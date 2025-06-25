#include "SleepingScene.h"
#include "SceneManager.h" 
#include "InputManager.h"
#include "Renderer.h"
#include "GameStats.h"
#include "Localization.h"
#include <U8g2lib.h>
#include "SerialForwarder.h"
#include <cmath> 
#include "../../DebugUtils.h"
#include "GEM_u8g2.h" 
#include "../../System/GameContext.h" 

SleepingScene::SleepingScene() :
    _sleepIndicatorState(false),
    _lastSleepIndicatorToggleTime(0),
    _lastFlyingZSpawnTime(0),
    _currentFatigueBarDisplayValue(0.0f),
    _targetFatigueBarDisplayValue(0.0f),
    _fatigueBarAnimStartTime(0),
    _isFatigueBarAnimating(false),
    _fatigueBarPatternOffset(0),
    _lastFatigueBarPatternUpdateTime(0)
{
}

SleepingScene::~SleepingScene() {
    debugPrint("SCENES", "SleepingScene destroyed.");
}

void SleepingScene::init(GameContext& context) { 
    Scene::init(); 
    _gameContext = &context;
    debugPrint("SCENES", "SleepingScene::init");

    if (!_gameContext || !_gameContext->renderer || !_gameContext->characterManager) {
        debugPrint("SCENES", "ERROR: SleepingScene::init - Critical context members are null!");
        return;
    }

    _dialogBox.reset(new DialogBox(*_gameContext->renderer)); 
    _particleSystem.reset(new ParticleSystem(*_gameContext->renderer, _gameContext->defaultFont)); 
    
    _gameContext->inputManager->registerButtonListener(EDGE_Button::OK, EDGE_Event::CLICK, this, [this](){ 
        if (handleDialogKeyPress(GEM_KEY_OK)) { return; }
        this->onWakeUpAttemptPress(); 
    });
}

void SleepingScene::onEnter() {
    debugPrint("SCENES", "SleepingScene::onEnter");

    if (!_gameContext || !_gameContext->characterManager || !_gameContext->gameStats || !_gameContext->renderer) {
        debugPrint("SCENES", "ERROR: SleepingScene::onEnter - Critical context members are null!");
        return;
    }

    Renderer& renderer = *_gameContext->renderer;
    GraphicAssetData staticAsset;
    int charW = 32; 
    int charH = 32; 
    if (_gameContext->characterManager->getGraphicAssetData(GraphicType::STATIC_IDLE, staticAsset) && staticAsset.isValid()) {
        charW = staticAsset.width;
        charH = staticAsset.height;
    }
    _targetEggX = (renderer.getWidth() - charW) / 2;
    _targetEggY = (renderer.getHeight() - charH) / 2;
    
    _lastSleepIndicatorToggleTime = millis();
    _lastFlyingZSpawnTime = millis();
    if (_particleSystem) _particleSystem->reset();
    if (_dialogBox) _dialogBox->close();

    _targetFatigueBarDisplayValue = 100.0f - (float)_gameContext->gameStats->fatigue; 
    _fatigueBarAnimStartTime = millis();
    _isFatigueBarAnimating = true;
    _fatigueBarPatternOffset = 0; 
    _lastFatigueBarPatternUpdateTime = millis();
    debugPrintf("SCENES", "SleepingScene: Fatigue bar animation target: %.1f", _targetFatigueBarDisplayValue);
}

void SleepingScene::onExit() {
    debugPrint("SCENES", "SleepingScene::onExit");
    if (_particleSystem) _particleSystem->reset();
    if (_dialogBox) _dialogBox->close();
    _isFatigueBarAnimating = false; 
    if (_gameContext && _gameContext->inputManager) {
        _gameContext->inputManager->unregisterAllListenersForScene(this);
    }
}

void SleepingScene::update(unsigned long deltaTime) {
    unsigned long currentTime = millis();

    if (!_gameContext || !_gameContext->gameStats) return; // Guard
    GameStats* gameStats = _gameContext->gameStats; // Convenience

    updateSleepState(currentTime, deltaTime);
    updateFatigueBarAnimation(currentTime);

    if (currentTime - _lastFatigueBarPatternUpdateTime >= FATIGUE_BAR_PATTERN_UPDATE_MS) {
        _lastFatigueBarPatternUpdateTime = currentTime;
        _fatigueBarPatternOffset = (_fatigueBarPatternOffset + 1) % (FATIGUE_BAR_PATTERN_WIDTH + FATIGUE_BAR_PATTERN_SPACING);
    }

    if (_dialogBox && _dialogBox->isActive()) {
        _dialogBox->update(currentTime);
        if (_dialogBox->isTemporary()) { /* Allow background updates */ }
        else { return; } 
    }

    if (!gameStats->isSleeping) {
        debugPrint("SCENES", "SleepingScene: Detected character is no longer sleeping (auto-wake). Switching to MainScene.");
        if (_gameContext->sceneManager) { // Use context
            _gameContext->sceneManager->requestSetCurrentScene("MAIN");
        }
    }
}

void SleepingScene::updateFatigueBarAnimation(unsigned long currentTime) {
    if (!_isFatigueBarAnimating) return;
    unsigned long elapsedTime = currentTime - _fatigueBarAnimStartTime;
    float progress = 1.0f; 
    if (FATIGUE_BAR_ANIM_DURATION_MS > 0 && elapsedTime < FATIGUE_BAR_ANIM_DURATION_MS) {
        progress = (float)elapsedTime / (float)FATIGUE_BAR_ANIM_DURATION_MS;
    }
    _currentFatigueBarDisplayValue = 0.0f + (_targetFatigueBarDisplayValue - 0.0f) * progress;
    if (progress >= 1.0f) {
        _currentFatigueBarDisplayValue = _targetFatigueBarDisplayValue; 
        _isFatigueBarAnimating = false;
    }
}

void SleepingScene::updateSleepState(unsigned long currentTime, unsigned long deltaTime) {
    if (currentTime - _lastSleepIndicatorToggleTime >= SLEEP_INDICATOR_TOGGLE_MS) {
        _sleepIndicatorState = !_sleepIndicatorState;
        _lastSleepIndicatorToggleTime = currentTime;
    }

    if (_particleSystem) {
        if (currentTime - _lastFlyingZSpawnTime >= FLYING_Z_SPAWN_INTERVAL_MS) {
            _lastFlyingZSpawnTime = currentTime;
            if (!_gameContext || !_gameContext->characterManager) return; // Guard

            GraphicAssetData baseAsset;
            int charW = 32;
            if (_gameContext->characterManager->getGraphicAssetData(GraphicType::STATIC_IDLE, baseAsset) && baseAsset.isValid()) {
                charW = baseAsset.width;
            }
            float spawnX = _targetEggX + random(charW / 4, charW * 3 / 4);
            float spawnY = _targetEggY - 5; 
            float driftX = (float)random(-10, 11) / 100.0f * FLYING_Z_SPEED_X_VAR;

            _particleSystem->spawnParticle(
                spawnX, spawnY, driftX, FLYING_Z_SPEED_Y,
                FLYING_Z_LIFETIME_MS, 'z', u8g2_font_4x6_tf, 1
            );
        }
        _particleSystem->update(currentTime, deltaTime);
    }
}

void SleepingScene::draw(Renderer& renderer) { 
    if (!_gameContext || !_gameContext->display) return; 
    U8G2* u8g2 = _gameContext->display;

    u8g2->setDrawColor(1); 
    drawCharacter(renderer);
    drawSleepIndicator(renderer); 
    drawFatigueBar(renderer);
    if (_dialogBox && _dialogBox->isActive()) { _dialogBox->draw(); }
}

void SleepingScene::drawCharacter(Renderer& renderer) {
    if (!_gameContext || !_gameContext->display || !_gameContext->characterManager) return; 
    U8G2* u8g2 = _gameContext->display;
    CharacterManager* characterManager = _gameContext->characterManager;

    GraphicAssetData baseAsset;
    const unsigned char* bitmapToDraw = nullptr;
    if (characterManager->getGraphicAssetData(GraphicType::STATIC_IDLE, baseAsset) && baseAsset.isValid()) {
        bitmapToDraw = baseAsset.bitmap;
    } else {
        debugPrint("SCENES", "SleepingScene Warning: STATIC_IDLE character bitmap not available!");
        return; 
    }
    u8g2->setDrawColor(1); u8g2->setBitmapMode(0);   
    u8g2->drawXBMP(renderer.getXOffset() + _targetEggX, renderer.getYOffset() + _targetEggY, baseAsset.width, baseAsset.height, bitmapToDraw);
}

void SleepingScene::drawSleepIndicator(Renderer& renderer) {
    if (!_gameContext || !_gameContext->display || !_gameContext->characterManager) return; 
    U8G2* u8g2 = _gameContext->display;
    CharacterManager* characterManager = _gameContext->characterManager;

    GraphicAssetData baseAsset;
    if (!characterManager->getGraphicAssetData(GraphicType::STATIC_IDLE, baseAsset) || !baseAsset.isValid()) {
        debugPrint("SCENES", "SleepingScene: Could not get base asset dimensions for sleep indicator.");
        return;
    }
    const char* zChar1 = _sleepIndicatorState ? "z" : "Z";
    const char* zChar2 = _sleepIndicatorState ? "Z" : "z";
    int zX1 = _targetEggX - 6; int zY1 = _targetEggY + 3;
    int zX2 = _targetEggX + baseAsset.width - 2; int zY2 = _targetEggY + 6;
    uint8_t originalDrawColor = u8g2->getDrawColor(); 
    u8g2->setFont(u8g2_font_5x7_tf); u8g2->setDrawColor(1); 
    u8g2->drawStr(renderer.getXOffset() + zX1, renderer.getYOffset() + zY1, zChar1);
    u8g2->drawStr(renderer.getXOffset() + zX2, renderer.getYOffset() + zY2, zChar2);
    if (_particleSystem) { _particleSystem->draw(); }
    u8g2->setDrawColor(originalDrawColor); 
}

void SleepingScene::drawFatigueBar(Renderer& renderer) {
    if (!_gameContext || !_gameContext->display || !_gameContext->gameStats) return; 
    U8G2* u8g2 = _gameContext->display;
    
    int barX = renderer.getXOffset() + (renderer.getWidth() - FATIGUE_BAR_WIDTH) / 2;
    int barY = renderer.getYOffset() + renderer.getHeight() - FATIGUE_BAR_HEIGHT - FATIGUE_BAR_Y_OFFSET;
    int fillW = map(static_cast<long>(_currentFatigueBarDisplayValue), 0, 100, 0, FATIGUE_BAR_WIDTH - (2 * FATIGUE_BAR_CORNER_RADIUS));
    fillW = std::max(0, std::min(FATIGUE_BAR_WIDTH - (2 * FATIGUE_BAR_CORNER_RADIUS), fillW));
    uint8_t originalDrawColor = u8g2->getDrawColor();
    u8g2->setDrawColor(1); 
    u8g2->drawRFrame(barX, barY, FATIGUE_BAR_WIDTH, FATIGUE_BAR_HEIGHT, FATIGUE_BAR_CORNER_RADIUS);
    if (fillW > 0) {
        u8g2->setDrawColor(1);
        u8g2->drawRBox(barX + 1, barY + 1, fillW, FATIGUE_BAR_HEIGHT - 2, FATIGUE_BAR_CORNER_RADIUS > 0 ? FATIGUE_BAR_CORNER_RADIUS -1 : 0);
    }
    if (fillW > 0) {
        u8g2->setClipWindow(barX + 1, barY + 1, barX + 1 + fillW, barY + 1 + FATIGUE_BAR_HEIGHT - 2);
        u8g2->setDrawColor(0); 
        for (int x_offset = -FATIGUE_BAR_PATTERN_WIDTH - FATIGUE_BAR_PATTERN_SPACING + _fatigueBarPatternOffset; x_offset < fillW; x_offset += (FATIGUE_BAR_PATTERN_WIDTH + FATIGUE_BAR_PATTERN_SPACING)) {
            for (int i = 0; i < FATIGUE_BAR_PATTERN_WIDTH; ++i) {
                int lineX = barX + 1 + x_offset + i; 
                u8g2->drawVLine(lineX, barY + 1, FATIGUE_BAR_HEIGHT - 2);
            }
        }
        u8g2->setMaxClipWindow(); 
    }
    u8g2->setDrawColor(originalDrawColor); 
}

void SleepingScene::onWakeUpAttemptPress() {
    attemptToWakeUp();
}

void SleepingScene::attemptToWakeUp() {
    if (!_gameContext || !_gameContext->gameStats || !_dialogBox || !_gameContext->sceneManager) { 
        debugPrint("SCENES", "Error: Critical context members null in SleepingScene::attemptToWakeUp");
        return;
    }
    GameStats* gameStats = _gameContext->gameStats; 

    if (!gameStats->isSleeping) return; 

    if (gameStats->fatigue > WAKE_FATIGUE_THRESHOLD) {
        debugPrintf("SCENES", "SleepingScene: Attempt wake FAILED (Fatigue %d > %d).",
                   gameStats->fatigue, WAKE_FATIGUE_THRESHOLD);
        _dialogBox->showTemporary(loc(StringKey::DIALOG_STILL_TOO_TIRED));
    } else {
        debugPrint("SCENES", "SleepingScene: Attempt wake OK. Switching to MainScene.");
        gameStats->setIsSleeping(false);
        gameStats->save();
        _gameContext->sceneManager->requestSetCurrentScene("MAIN");
    }
}

bool SleepingScene::handleDialogKeyPress(uint8_t keyCode) {
    if (_dialogBox && _dialogBox->isActive()) {
        if (keyCode == GEM_KEY_OK) {
            _dialogBox->close();
            return true;
        }
    }
    return false;
}