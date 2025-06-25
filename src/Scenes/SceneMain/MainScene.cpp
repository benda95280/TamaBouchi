#include "MainScene.h"
#include "../../System/GameContext.h"
#include "SceneManager.h"
#include "InputManager.h"
#include "Renderer.h"
#include <U8g2lib.h>
#include "../../Graphics.h"
#include "character/level0/CharacterGraphics_L0.h"
#include "../../GameStats.h"
#include "../../Animator.h"
#include "IconMenuManager.h"
#include "../../Weather/WeatherManager.h"
#include <Arduino.h>
#include <memory>
#include "../../SerialForwarder.h"
#include "character/CharacterManager.h"
#include <vector>
#include "FastNoiseLite.h"
#include <cmath>
#include "../../DialogBox/DialogBox.h"
#include "../../Helper/PathGenerator.h"
#include "IdleAnimationController.h" 
#include "../../DebugUtils.h"
#include "../../GlobalMappings.h"


MainScene::MainScene() : currentPhase(AnimationPhase::PHASE_IDLE),
                         showMenus(false),
                         _nextIdleAnimTime(0),
                         _isFirstEntry(true),
                         _topMenuYCurrent(0.0f),
                         _bottomMenuYCurrent(0.0f),
                         _topMenuYTarget(0.0f),
                         _bottomMenuYTarget(0.0f)
{
    _noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    _noise.SetFrequency(0.05);
    _noise.SetSeed(esp_random());
}
void MainScene::init(GameContext& context)
{
    Scene::init();
    _gameContext = &context;
    debugPrint("SCENES", "MainScene::init");
    
    if (!_gameContext) { 
        debugPrint("SCENES", "FATAL: MainScene::init - _gameContext is null!");
        return;
    }

    if (_gameContext->characterManager && _gameContext->gameStats)
    {
        _gameContext->characterManager->init(_gameContext->gameStats);
    }
    else
    {
        debugPrint("SCENES", "ERROR: CharacterManager or GameStats (via context) is null in MainScene init!");
    }

    _isFirstEntry = true;
    showMenus = false;
    currentAnimation.reset();
    _nextIdleAnimTime = 0;

    if (_gameContext->renderer && _gameContext->gameStats) {
        _iconMenuManager.reset(new IconMenuManager(*_gameContext->renderer, *_gameContext->gameStats));
        _dialogBox.reset(new DialogBox(*_gameContext->renderer));
        _idleAnimController.reset(new IdleAnimationController(*_gameContext->renderer, _gameContext->characterManager, _gameContext->pathGenerator));
    } else {
        debugPrint("SCENES", "ERROR: Renderer or GameStats (via context) null in MainScene init, cannot create UI managers.");
    }


    _gameContext->inputManager->registerButtonListener(EDGE_Button::LEFT, EDGE_Event::CLICK, this, [this](){ 
        this->onLeftClick(); 
    });
    _gameContext->inputManager->registerButtonListener(EDGE_Button::OK, EDGE_Event::CLICK, this, [this](){ 
        this->onSelectClick(); 
    });
    _gameContext->inputManager->registerButtonListener(EDGE_Button::RIGHT, EDGE_Event::CLICK, this, [this](){ 
        this->onRightClick(); 
    });

    debugPrint("SCENES", "MainScene Initialized - Ready for onEnter.");
}
void MainScene::onEnter()
{
    debugPrint("SCENES", "MainScene::onEnter");
    if (!_gameContext || !_gameContext->gameStats || !_gameContext->characterManager || !_gameContext->renderer) {
        debugPrint("SCENES", "ERROR: MainScene::onEnter - Critical context member is null!");
        return;
    }


    if (_gameContext->gameStats->isSleeping)
    {
        debugPrint("SCENES", "MainScene::onEnter - Game is already in sleeping state. Switching to SleepingScene.");
        _gameContext->sceneManager->requestSetCurrentScene("SLEEPING");
        return;
    }

    _gameContext->characterManager->updateLevel(_gameContext->gameStats->age);

    if (_iconMenuManager)
    {
        _iconMenuManager->setupLayout();
        _iconMenuManager->resetSelection();
    }
    else
    {
        debugPrint("SCENES", "ERROR: _iconMenuManager is null in onEnter!");
    }
    if (_gameContext->weatherManager) 
        _gameContext->weatherManager->init();
    else
    {
        debugPrint("SCENES", "ERROR: _weatherManager (via context) is null in onEnter!");
    }
    if (_idleAnimController)
        _idleAnimController->reset(); 
    else
    {
        debugPrint("SCENES", "ERROR: _idleAnimController is null in onEnter!");
    }

    currentAnimation.reset();
    if (_dialogBox)
        _dialogBox->close();

    GraphicAssetData staticAsset, downingAsset;
    bool staticOk = _gameContext->characterManager->getGraphicAssetData(GraphicType::STATIC_IDLE, staticAsset);
    bool downingOk = _gameContext->characterManager->getGraphicAssetData(GraphicType::DOWNING_SHEET, downingAsset);

    showMenus = true;
    debugPrintf("SCENES", "MainScene::onEnter - showMenus initially set to %s", showMenus ? "true" : "false");

    Renderer& renderer = *_gameContext->renderer;
    float onScreenTopY = static_cast<float>(renderer.getYOffset());
    float onScreenBottomY = static_cast<float>(renderer.getYOffset() + renderer.getHeight() - MENU_BAR_HEIGHT);
    float offScreenTopY = static_cast<float>(renderer.getYOffset() - MENU_BAR_HEIGHT - 1);
    float offScreenBottomY = static_cast<float>(renderer.getYOffset() + renderer.getHeight() + 1);

    if (_isFirstEntry)
    {
        debugPrint("SCENES", "MainScene: First entry, starting entry animation.");
        _isFirstEntry = false;
        currentPhase = AnimationPhase::PHASE_FALLING;
        showMenus = false;
        _topMenuYCurrent = offScreenTopY;
        _bottomMenuYCurrent = offScreenBottomY;
        _topMenuYTarget = offScreenTopY;
        _bottomMenuYTarget = offScreenBottomY;

        if (staticOk)
        {
            targetEggX = (renderer.getWidth() - staticAsset.width) / 2;
            targetEggY = (renderer.getHeight() - staticAsset.height) / 2;
            int startX = targetEggX;
            int startY = -staticAsset.height;
            currentAnimation.reset(new Animator(renderer, staticAsset.bitmap, staticAsset.width, staticAsset.height, startX, startY, targetEggX, targetEggY, FALLING_ANIMATION_DURATION_MS));
            debugPrint("SCENES", "MainScene::onEnter - FALLING animation created.");
            unsigned long downingTotalDur = downingOk && downingAsset.isSpritesheet() ? (downingAsset.frameDurationMs * downingAsset.frameCount) : 0;
            scheduleNextIdleAnimation(millis() + FALLING_ANIMATION_DURATION_MS + downingTotalDur + 1000);
        }
        else
        {
            debugPrintf("SCENES", "ERROR: staticBitmap (%p) is null/invalid in onEnter, cannot create FALLING animation.", staticAsset.bitmap);
            currentPhase = AnimationPhase::PHASE_IDLE;
            showMenus = !(_gameContext->gameStats && _gameContext->gameStats->isSleeping);
            if (_iconMenuManager && showMenus)
                _iconMenuManager->resetSelection();
            scheduleNextIdleAnimation(millis());
        }
    }
    else
    {
        debugPrint("SCENES", "MainScene: Subsequent entry, skipping entry animation.");
        currentPhase = AnimationPhase::PHASE_IDLE;
        scheduleNextIdleAnimation(millis());
        if (staticOk)
        {
            targetEggX = (renderer.getWidth() - staticAsset.width) / 2;
            targetEggY = (renderer.getHeight() - staticAsset.height) / 2;
        }
        else
        {
            targetEggX = (renderer.getWidth() - 32) / 2;
            targetEggY = (renderer.getHeight() - 32) / 2;
        }
        if (showMenus)
        { 
            if (_iconMenuManager)
                _iconMenuManager->resetSelection();
            _topMenuYCurrent = onScreenTopY;
            _bottomMenuYCurrent = onScreenBottomY;
        }
        else
        { 
            _topMenuYCurrent = offScreenTopY;
            _bottomMenuYCurrent = offScreenBottomY;
        }
        _topMenuYTarget = _topMenuYCurrent;
        _bottomMenuYTarget = _bottomMenuYCurrent;
    }
}
void MainScene::onExit()
{
    debugPrint("SCENES", "MainScene::onExit");
    currentAnimation.reset();
    if (_iconMenuManager)
        _iconMenuManager->resetSelection();
    if (_idleAnimController)
        _idleAnimController->reset(); 
    showMenus = false;
    currentPhase = AnimationPhase::PHASE_IDLE;
    if (_gameContext && _gameContext->inputManager) {
        _gameContext->inputManager->unregisterAllListenersForScene(this);
    }
}
void MainScene::scheduleNextIdleAnimation(unsigned long currentTime)
{
    unsigned long randomInterval = random(MIN_IDLE_ANIM_INTERVAL_MS, MAX_IDLE_ANIM_INTERVAL_MS + 1);
    _nextIdleAnimTime = currentTime + randomInterval;
    debugPrintf("SCENES", "MainScene: Next random idle animation scheduled for %lu ms from now (at %lu)", randomInterval, _nextIdleAnimTime);
}

bool MainScene::triggerSnoozeAnimation()
{
    if (!_gameContext || !_gameContext->gameStats || !_idleAnimController) {
        debugPrint("SCENES", "MainScene Error: Context/GameStats/IdleController null in triggerSnoozeAnimation.");
        return false;
    }
    if (currentAnimation || currentPhase != AnimationPhase::PHASE_IDLE || _gameContext->gameStats->isSleeping || showMenus)
    {
        debugPrint("SCENES", "MainScene: Cannot trigger Snooze: Main animation, sleep, menu active, or not idle.");
        return false;
    }
    bool success = _idleAnimController->triggerSnoozeAnimation(millis(), targetEggX, targetEggY);
    if (success)
        _nextIdleAnimTime = 0; 
    return success;
}
bool MainScene::triggerLeanAnimation()
{
    if (!_gameContext || !_gameContext->gameStats || !_idleAnimController) {
        debugPrint("SCENES", "MainScene Error: Context/GameStats/IdleController null in triggerLeanAnimation.");
        return false;
    }
    if (currentAnimation || currentPhase != AnimationPhase::PHASE_IDLE || _gameContext->gameStats->isSleeping || showMenus)
    {
        debugPrint("SCENES", "MainScene: Cannot trigger Lean: Main animation, sleep, menu active, or not idle.");
        return false;
    }
    bool success = _idleAnimController->triggerLeanAnimation(millis(), targetEggX, targetEggY);
    if (success)
        _nextIdleAnimTime = 0;
    return success;
}
bool MainScene::triggerPathAnimation()
{
    if (!_gameContext || !_gameContext->gameStats || !_idleAnimController) {
        debugPrint("SCENES", "MainScene Error: Context/GameStats/IdleController null in triggerPathAnimation.");
        return false;
    }
    if (currentAnimation || currentPhase != AnimationPhase::PHASE_IDLE || _gameContext->gameStats->isSleeping || showMenus)
    {
        debugPrint("SCENES", "MainScene: Cannot trigger Path: Main animation, sleep, menu active, or not idle.");
        return false;
    }
    bool success = _idleAnimController->triggerPathAnimation(millis());
    if (success)
        _nextIdleAnimTime = 0;
    return success;
}
void MainScene::updateMenuAnimations(float dt)
{
    if (!_gameContext || !_gameContext->renderer) return; 
    Renderer& renderer = *_gameContext->renderer;

    float onScreenTopY = static_cast<float>(renderer.getYOffset());
    float onScreenBottomY = static_cast<float>(renderer.getYOffset() + renderer.getHeight() - MENU_BAR_HEIGHT);
    float offScreenTopY = static_cast<float>(renderer.getYOffset() - MENU_BAR_HEIGHT - 1);
    float offScreenBottomY = static_cast<float>(renderer.getYOffset() + renderer.getHeight() + 1);

    if (showMenus)
    {
        _topMenuYTarget = onScreenTopY;
        _bottomMenuYTarget = onScreenBottomY;
    }
    else
    {
        _topMenuYTarget = offScreenTopY;
        _bottomMenuYTarget = offScreenBottomY;
    }

    if (std::abs(_topMenuYCurrent - _topMenuYTarget) > 0.5f) { _topMenuYCurrent += (_topMenuYTarget - _topMenuYCurrent) * MENU_ANIMATION_SPEED * dt; }
    else { _topMenuYCurrent = _topMenuYTarget; }

    if (std::abs(_bottomMenuYCurrent - _bottomMenuYTarget) > 0.5f) { _bottomMenuYCurrent += (_bottomMenuYTarget - _bottomMenuYCurrent) * MENU_ANIMATION_SPEED * dt; }
    else { _bottomMenuYCurrent = _bottomMenuYTarget; }
}
void MainScene::update(unsigned long deltaTime)
{
    unsigned long currentTime = millis();
    float dtSeconds = deltaTime / 1000.0f;

    if (!_gameContext || !_gameContext->gameStats || !_gameContext->characterManager || !_gameContext->sceneManager || !_gameContext->weatherManager) {
        debugPrint("SCENES", "MainScene FATAL: Critical context members are NULL in update!");
        return; 
    }


    if (_gameContext->gameStats->isSleeping && currentPhase != AnimationPhase::PHASE_FALLING && currentPhase != AnimationPhase::DOWNING)
    {
        debugPrint("SCENES", "MainScene::update - Game is sleeping. Switching to SleepingScene.");
        _gameContext->sceneManager->requestSetCurrentScene("SLEEPING");
        return;
    }

    _gameContext->weatherManager->update(currentTime);
    updateMenuAnimations(dtSeconds); 

    if (_gameContext->characterManager->getCurrentManagedLevel() != _gameContext->gameStats->age)
    {
        _gameContext->characterManager->updateLevel(_gameContext->gameStats->age);
        debugPrintf("SCENES", "MainScene::update - Character level updated to %u", _gameContext->gameStats->age);
    }

    if (_dialogBox && _dialogBox->isActive())
    {
        _dialogBox->update(currentTime);
    }

    if (currentAnimation)
    { 
        bool stillAnimating = currentAnimation->update();
        if (!stillAnimating)
        {
            GraphicAssetData downingAsset;
            bool downingOk = _gameContext->characterManager->getGraphicAssetData(GraphicType::DOWNING_SHEET, downingAsset);

            if (currentPhase == AnimationPhase::PHASE_FALLING)
            {
                currentPhase = AnimationPhase::DOWNING;
                currentAnimation.reset();
                if (_gameContext->renderer && downingOk && downingAsset.isSpritesheet())
                {
                    currentAnimation.reset(new Animator(*_gameContext->renderer, downingAsset.bitmap, downingAsset.width, downingAsset.height, downingAsset.frameCount, downingAsset.bytesPerFrame, targetEggX, targetEggY, downingAsset.frameDurationMs, 1));
                }
                else
                {
                    debugPrint("SCENES", "Error: Missing assets/renderer for DOWNING animation, skipping to IDLE.");
                    currentPhase = AnimationPhase::PHASE_IDLE;
                    showMenus = !(_gameContext->gameStats && _gameContext->gameStats->isSleeping);
                    if (_iconMenuManager && showMenus)
                        _iconMenuManager->resetSelection();
                    scheduleNextIdleAnimation(currentTime);
                }
            }
            else if (currentPhase == AnimationPhase::DOWNING)
            {
                currentPhase = AnimationPhase::PHASE_IDLE;
                currentAnimation.reset();
                showMenus = !(_gameContext->gameStats && _gameContext->gameStats->isSleeping);
                if (showMenus && _iconMenuManager)
                    _iconMenuManager->resetSelection();
                scheduleNextIdleAnimation(currentTime);
            }
            else
            { 
                currentAnimation.reset();
                currentPhase = AnimationPhase::PHASE_IDLE; 
                scheduleNextIdleAnimation(currentTime);
            }
        }
    }
    else if (currentPhase == AnimationPhase::PHASE_IDLE && !(_dialogBox && _dialogBox->isActive()))
    {
        if (_idleAnimController)
        {
            if (_idleAnimController->isAnimating())
            {
                _idleAnimController->update(currentTime);
                if (!_idleAnimController->isAnimating())
                { 
                    scheduleNextIdleAnimation(currentTime);
                }
            }
            else
            { 
                if (_nextIdleAnimTime > 0 && currentTime >= _nextIdleAnimTime && !showMenus)
                {
                    _idleAnimController->startRandomIdleAnimation(currentTime, targetEggX, targetEggY);
                    if (!_idleAnimController->isAnimating())
                    { 
                        scheduleNextIdleAnimation(currentTime);
                    }
                    else
                    {
                        _nextIdleAnimTime = 0; 
                    }
                }
            }
        }

        if (_iconMenuManager)
        {
            _iconMenuManager->update();
            if (_iconMenuManager->getSelectedRow() == IconMenuManager::ROW_NONE && showMenus)
            {
                showMenus = false;
            }
        }
    }
}
void MainScene::attemptToSleep()
{
    if (!_gameContext || !_gameContext->gameStats || !_dialogBox || !_gameContext->sceneManager)
    {
        debugPrint("SCENES", "Error: Critical context member null in attemptToSleep");
        return;
    }
    if (_gameContext->gameStats->isSleeping)
    {
        _dialogBox->showTemporary(loc(StringKey::DIALOG_ALREADY_SLEEPING));
        return;
    }
    if (_gameContext->gameStats->fatigue >= SLEEP_FATIGUE_THRESHOLD)
    {
        debugPrint("SCENES", "Command: Attempt sleep OK. Switching to SleepingScene.");
        _gameContext->gameStats->setIsSleeping(true);
        _gameContext->gameStats->save();

        _gameContext->sceneManager->requestSetCurrentScene("SLEEPING");

        currentAnimation.reset();
        if (_idleAnimController)
            _idleAnimController->reset(); 
        _nextIdleAnimTime = 0;
        showMenus = false;
    }
    else
    {
        debugPrint("SCENES", "Command: Attempt sleep FAILED (Not tired).");
        _dialogBox->showTemporary(loc(StringKey::DIALOG_NOT_TIRED_ENOUGH));
    }
}
void MainScene::drawSicknessOverlay(Renderer &renderer) 
{
    if (!_gameContext || !_gameContext->gameStats || !_gameContext->characterManager) return;
    
    if (!_gameContext->gameStats->isSick()) return;
    
    Sickness currentSickness = _gameContext->gameStats->sickness;
    GraphicAssetData overlayAsset;
    GraphicType sicknessGraphicType;
    switch (currentSickness)
    {
    case Sickness::COLD: sicknessGraphicType = GraphicType::SICKNESS_COLD; break;
    case Sickness::HOT: sicknessGraphicType = GraphicType::SICKNESS_HOT; break;
    case Sickness::DIARRHEA: sicknessGraphicType = GraphicType::SICKNESS_DIARRHEA; break;
    case Sickness::HEADACHE: sicknessGraphicType = GraphicType::SICKNESS_HEADACHE; break;
    default: return;
    }
    if (_gameContext->characterManager->getGraphicAssetData(sicknessGraphicType, overlayAsset) && overlayAsset.isValid())
    {
        U8G2 *u8g2 = renderer.getU8G2(); 
        if (!u8g2) return;
        GraphicAssetData baseAsset;
        int charW = 32; int charH = 32; 
        if (_gameContext->characterManager->getGraphicAssetData(GraphicType::STATIC_IDLE, baseAsset) && baseAsset.isValid())
        {
            charW = baseAsset.width; charH = baseAsset.height;
        }
        int drawX = targetEggX + (charW - overlayAsset.width) / 2;
        int drawY = targetEggY + (charH - overlayAsset.height) / 2 - 3;
        u8g2->setBitmapMode(1); u8g2->setDrawColor(1);
        u8g2->drawXBMP(renderer.getXOffset() + drawX, renderer.getYOffset() + drawY, overlayAsset.width, overlayAsset.height, overlayAsset.bitmap);
        u8g2->setBitmapMode(0);
    }
}
void MainScene::draw(Renderer& renderer) 
{
    if (!_gameContext || !_gameContext->characterManager || !_gameContext->renderer || !_gameContext->weatherManager) {
        debugPrint("SCENES", "ERROR: Critical context members null in MainScene::draw!");
        return;
    }
    U8G2* u8g2 = renderer.getU8G2(); 
    if (!u8g2) return;

    bool canDrawWeather = (currentPhase == AnimationPhase::PHASE_IDLE);

    _gameContext->weatherManager->drawBackground(canDrawWeather);

    if (currentAnimation && (currentPhase == AnimationPhase::PHASE_FALLING || currentPhase == AnimationPhase::DOWNING))
    {
        currentAnimation->draw();
    }
    else if (currentPhase == AnimationPhase::PHASE_IDLE)
    {
        if (_idleAnimController && _idleAnimController->isAnimating())
        {
            _idleAnimController->draw();
        }
        else
        { 
            GraphicAssetData baseAsset;
            if (_gameContext->characterManager->getGraphicAssetData(GraphicType::STATIC_IDLE, baseAsset) && baseAsset.isValid())
            {
                u8g2->drawXBMP(renderer.getXOffset() + targetEggX, renderer.getYOffset() + targetEggY, baseAsset.width, baseAsset.height, baseAsset.bitmap);
            }
        }
        drawSicknessOverlay(renderer);
    }
    
    uint8_t originalColor = u8g2->getDrawColor();
    u8g2->setDrawColor(2); 
    _gameContext->weatherManager->drawForeground(canDrawWeather);
    u8g2->setDrawColor(originalColor);

    if (_iconMenuManager)
    {
        int currentTopMenuDrawY = static_cast<int>(round(_topMenuYCurrent));
        if (currentTopMenuDrawY < renderer.getYOffset() + MENU_BAR_HEIGHT)
        {
            u8g2->setDrawColor(0); u8g2->drawBox(renderer.getXOffset(), currentTopMenuDrawY, renderer.getWidth(), MENU_BAR_HEIGHT);
            u8g2->setDrawColor(1); _iconMenuManager->drawRow(IconMenuManager::ROW_TOP, currentTopMenuDrawY);
        }
        int currentBottomMenuDrawY = static_cast<int>(round(_bottomMenuYCurrent));
        if (currentBottomMenuDrawY + MENU_BAR_HEIGHT > renderer.getYOffset())
        {
            u8g2->setDrawColor(0); u8g2->drawBox(renderer.getXOffset(), currentBottomMenuDrawY, renderer.getWidth(), MENU_BAR_HEIGHT);
            u8g2->setDrawColor(1); _iconMenuManager->drawRow(IconMenuManager::ROW_BOTTOM, currentBottomMenuDrawY);
        }
    }

    if (_dialogBox && _dialogBox->isActive())
    {
        _dialogBox->draw();
    }
}
void MainScene::handleMenuAction(IconAction action)
{
    if (!_gameContext || !_gameContext->sceneManager) {
        debugPrint("SCENES", "MainScene Error: Context or SceneManager null in handleMenuAction.");
        return;
    }
    bool requestedChange = false;
    switch (action)
    {
    case IconAction::GOTO_STATS: _gameContext->sceneManager->requestPushScene("STATS"); requestedChange = true; break;
    case IconAction::GOTO_PARAMS: _gameContext->sceneManager->requestPushScene("PARAMS"); requestedChange = true; break;
    case IconAction::GOTO_ACTION_MENU: _gameContext->sceneManager->requestPushScene("ACTIONS"); requestedChange = true; break;
    case IconAction::ACTION_SLEEP: attemptToSleep(); break;
    case IconAction::ACTION_ICON1: debugPrint("SCENES", "MainScene: Handling ACTION_ICON1."); showMenus = false; break;
    case IconAction::ACTION_ICON2: debugPrint("SCENES", "MainScene: Handling ACTION_ICON2."); showMenus = false; break;
    case IconAction::NONE: default: break;
    }
    
    if (requestedChange) {
        if (_iconMenuManager) _iconMenuManager->resetSelection();
        showMenus = false;
    }
}
void MainScene::onLeftClick()
{
    if (_dialogBox && _dialogBox->isTemporary()) { _dialogBox->close(); return; }
    if (_dialogBox && _dialogBox->isActive()) return;
    if (!showMenus) { showMenus = true; if (_iconMenuManager) _iconMenuManager->resetSelection(); }
    else if (currentPhase == AnimationPhase::PHASE_IDLE && (!_idleAnimController || !_idleAnimController->isAnimating()) && _iconMenuManager)
    {
        handleMenuAction(_iconMenuManager->handleInput(MenuInputKey::LEFT));
    }
}
void MainScene::onRightClick()
{
    if (_dialogBox && _dialogBox->isTemporary()) { _dialogBox->close(); return; }
    if (_dialogBox && _dialogBox->isActive()) return;
    if (!showMenus) { showMenus = true; if (_iconMenuManager) _iconMenuManager->resetSelection(); }
    else if (currentPhase == AnimationPhase::PHASE_IDLE && (!_idleAnimController || !_idleAnimController->isAnimating()) && _iconMenuManager)
    {
        handleMenuAction(_iconMenuManager->handleInput(MenuInputKey::RIGHT));
    }
}
void MainScene::onSelectClick()
{
    if (_dialogBox && _dialogBox->isTemporary()) { _dialogBox->close(); return; }
    if (_dialogBox && _dialogBox->isActive()) { if (_dialogBox->isAtBottom()) _dialogBox->close(); else _dialogBox->scrollDown(); return; }
    if (!showMenus) { showMenus = true; if (_iconMenuManager) _iconMenuManager->resetSelection(); return; }
    if (currentPhase == AnimationPhase::PHASE_IDLE && (!_idleAnimController || !_idleAnimController->isAnimating()) && showMenus && _iconMenuManager)
    {
        handleMenuAction(_iconMenuManager->handleInput(MenuInputKey::SELECT));
    }
}