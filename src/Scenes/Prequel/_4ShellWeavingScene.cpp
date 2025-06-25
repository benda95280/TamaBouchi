#include "_4ShellWeavingScene.h"
#include "SceneManager.h"
#include "InputManager.h"
#include "Renderer.h"
#include "GameStats.h"
#include "SerialForwarder.h"
#include "Localization.h"
#include "../../Helper/EffectsManager.h"
#include "../../DebugUtils.h"
#include "GEM_u8g2.h"
#include "../../System/GameContext.h"

_4ShellWeavingScene::_4ShellWeavingScene()
{
    debugPrint("SCENES", "_4ShellWeavingScene constructor");
    for (int i = 0; i < MAX_FRAGMENTS; ++i)
    {
        _fragments[i].active = false;
    }
}

_4ShellWeavingScene::~_4ShellWeavingScene()
{
    debugPrint("SCENES", "_4ShellWeavingScene destroyed");
}

void _4ShellWeavingScene::init(GameContext &context)
{
    Scene::init();
    _gameContext = &context;
    debugPrint("SCENES", "_4ShellWeavingScene::init");

    if (!_gameContext || !_gameContext->renderer)
    {
        debugPrint("SCENES", "ERROR: _4ShellWeavingScene::init - Critical context member (renderer) is null!");
        return;
    }
    _dialogBox.reset(new DialogBox(*_gameContext->renderer));
    _effectsManager.reset(new EffectsManager(*_gameContext->renderer));

    _gameContext->inputManager->registerButtonListener(EDGE_Button::LEFT, EDGE_Event::PRESS, this, [this]()
                          { 
        if (handleDialogKeyPress(GEM_KEY_UP)) { return; } this->onButtonLeftPress(); });
    _gameContext->inputManager->registerButtonListener(EDGE_Button::RIGHT, EDGE_Event::PRESS, this, [this]()
                          { 
        if (handleDialogKeyPress(GEM_KEY_DOWN)) { return; } this->onButtonRightPress(); });
    _gameContext->inputManager->registerButtonListener(EDGE_Button::OK, EDGE_Event::CLICK, this, [this]()
                          { 
        if (handleDialogKeyPress(GEM_KEY_OK)) { 
            if (_dialogBox && !_dialogBox->isActive() && !_instructionsShown) _instructionsShown = true;
            return; 
        }
        this->onButtonOkClick(); });
    _gameContext->inputManager->registerButtonListener(EDGE_Button::OK, EDGE_Event::LONG_PRESS, this, [this]()
                          {
        if (handleDialogKeyPress(GEM_KEY_CANCEL)) { return; }
        if (_dialogBox && _dialogBox->isActive()) { _dialogBox->close(); _instructionsShown = true; } });
}

void _4ShellWeavingScene::onEnter()
{
    debugPrint("SCENES", "_4ShellWeavingScene::onEnter");
    _instructionsShown = false;
    _segmentsWoven = 0;
    _successfulWeaveTime = 0;

    if (_gameContext && _gameContext->renderer)
    {
        _collectorX = _gameContext->renderer->getWidth() / 2;
    }
    else
    {
        _collectorX = 128 / 2;
        debugPrint("SCENES", "Warning: _4ShellWeavingScene::onEnter - GameContext or Renderer is null.");
    }

    for (int i = 0; i < MAX_FRAGMENTS; ++i)
    {
        _fragments[i].active = false;
    }
    _lastFragmentSpawnTime = millis();
    _nextFragmentSpawnInterval = 1500 + random(0, 1000);
    setupNewPattern();
    resetSlots();
    if (_dialogBox)
    {
        String instructions = String(loc(StringKey::PREQUEL_STAGE4_TITLE)) + "\n" +
                              String(loc(StringKey::PREQUEL_STAGE4_INST_LINE1)) + "\n" +
                              String(loc(StringKey::PREQUEL_STAGE4_INST_LINE2)) + "\n" +
                              String(loc(StringKey::PREQUEL_STAGE4_INST_LINE3));
        _dialogBox->show(instructions.c_str());
    }
    else
    {
        _instructionsShown = true;
    }
}

void _4ShellWeavingScene::onExit()
{
    debugPrint("SCENES", "_4ShellWeavingScene::onExit");
    if (_dialogBox)
        _dialogBox->close();
    if (_gameContext && _gameContext->inputManager)
    {
        _gameContext->inputManager->unregisterAllListenersForScene(this);
    }
}

void _4ShellWeavingScene::update(unsigned long deltaTime)
{
    unsigned long currentTime = millis();
    if (handleDialogKeyPress(GEM_KEY_OK))
    {
        if (_dialogBox && !_dialogBox->isActive() && !_instructionsShown)
        {
            _instructionsShown = true;
            _stageStartTime = currentTime;
        }
        return;
    }
    if (!_instructionsShown)
        return;
    if (_effectsManager)
        _effectsManager->update(currentTime);
    if (_successfulWeaveTime > 0 && currentTime - _successfulWeaveTime < WEAVE_SUCCESS_DISPLAY_MS)
    {
        return;
    }
    else if (_successfulWeaveTime > 0)
    {
        _successfulWeaveTime = 0;
        if (_segmentsWoven >= SEGMENTS_TO_WIN)
        {
            completeStage();
            return;
        }
        setupNewPattern();
        resetSlots();
    }
    updateFragments(currentTime);
    if (currentTime - _lastFragmentSpawnTime >= _nextFragmentSpawnInterval)
    {
        spawnFragment();
        _lastFragmentSpawnTime = currentTime;
        _nextFragmentSpawnInterval = 1200 + random(0, 1600);
    }
}

void _4ShellWeavingScene::draw(Renderer &renderer)
{ 
    if (!_gameContext || !_gameContext->display)
        return;                         
    U8G2 *u8g2 = _gameContext->display; 
    int rOffsetX = renderer.getXOffset();
    int rOffsetY = renderer.getYOffset();
    if (_effectsManager)
        _effectsManager->applyEffectOffset(rOffsetX, rOffsetY);
    for (int i = 0; i < renderer.getWidth(); i += 8)
    {
        for (int j = 0; j < renderer.getHeight(); j += 8)
        {
            if ((i / 8 + j / 8) % 2 == 0)
                u8g2->drawPixel(rOffsetX + i, rOffsetY + j);
        }
    }
    drawFragments(renderer);
    drawCollector(renderer);
    drawWeavingUI(renderer);
    drawShellProgress(renderer);
    if (_successfulWeaveTime > 0 && millis() - _successfulWeaveTime < WEAVE_SUCCESS_DISPLAY_MS)
    {
        u8g2->setFont(u8g2_font_6x10_tf);
        const char *successMsg = "Segment Woven!";
        int msgWidth = u8g2->getStrWidth(successMsg);
        u8g2->setDrawColor(0);
        u8g2->drawBox(renderer.getXOffset() + (renderer.getWidth() - msgWidth) / 2 - 2, renderer.getYOffset() + renderer.getHeight() / 2 - 7, msgWidth + 4, 12);
        u8g2->setDrawColor(1);
        u8g2->drawStr(renderer.getXOffset() + (renderer.getWidth() - msgWidth) / 2, renderer.getYOffset() + renderer.getHeight() / 2 - 5, successMsg);
    }
    if (_dialogBox && _dialogBox->isActive())
    {
        _dialogBox->draw();
    }
}

void _4ShellWeavingScene::onButtonLeftPress()
{
    if (!_instructionsShown || _successfulWeaveTime > 0)
        return;
    _collectorX -= COLLECTOR_SPEED;
    if (_gameContext && _gameContext->renderer && _collectorX < 0)
        _collectorX = 0;
}
void _4ShellWeavingScene::onButtonRightPress()
{
    if (!_instructionsShown || _successfulWeaveTime > 0)
        return;
    _collectorX += COLLECTOR_SPEED;
    if (_gameContext && _gameContext->renderer && _collectorX > _gameContext->renderer->getWidth() - COLLECTOR_WIDTH)
    {
        _collectorX = _gameContext->renderer->getWidth() - COLLECTOR_WIDTH;
    }
}
void _4ShellWeavingScene::onButtonOkClick()
{
    if (_instructionsShown && _successfulWeaveTime == 0)
    {
        checkCollection();
    }
}

void _4ShellWeavingScene::spawnFragment()
{
    if (!_gameContext || !_gameContext->renderer)
        return;
    Renderer &r = *_gameContext->renderer;
    int availableSlot = -1;
    for (int i = 0; i < MAX_FRAGMENTS; ++i)
    {
        if (!_fragments[i].active)
        {
            availableSlot = i;
            break;
        }
    }
    if (availableSlot == -1)
        return;
    ShellFragment &frag = _fragments[availableSlot];
    frag.active = true;
    frag.x = random(0, r.getWidth() - 5);
    frag.y = 0;
    frag.vy = 0.3f + (float)random(0, 41) / 100.0f;
    frag.type = static_cast<ShellFragmentType>(random(1, (int)ShellFragmentType::_TYPE_COUNT));
    frag.displayChar = getCharForFragmentType(frag.type);
}
void _4ShellWeavingScene::updateFragments(unsigned long currentTime)
{
    if (!_gameContext || !_gameContext->renderer)
        return;
    int screenHeight = _gameContext->renderer->getHeight();
    for (int i = 0; i < MAX_FRAGMENTS; ++i)
    {
        if (_fragments[i].active)
        {
            _fragments[i].y += _fragments[i].vy;
            if (_fragments[i].y > screenHeight)
            {
                _fragments[i].active = false;
            }
        }
    }
}
void _4ShellWeavingScene::checkCollection()
{
    if (!_gameContext || !_gameContext->renderer || _successfulWeaveTime > 0)
        return;
    int collectorCenterX = _collectorX + COLLECTOR_WIDTH / 2;
    int collectorCatchY = _gameContext->renderer->getHeight() - COLLECTOR_Y_OFFSET;
    for (int i = 0; i < MAX_FRAGMENTS; ++i)
    {
        if (_fragments[i].active)
        {
            bool xMatch = (_fragments[i].x >= _collectorX - 2 && _fragments[i].x <= _collectorX + COLLECTOR_WIDTH + 2);
            bool yMatch = (_fragments[i].y >= collectorCatchY - COLLECTION_RADIUS && _fragments[i].y <= collectorCatchY + COLLECTION_RADIUS / 2.0f);
            if (xMatch && yMatch)
            {
                if (_currentSlots[_currentSlotIndex] == ShellFragmentType::NONE)
                {
                    if (_fragments[i].type == _targetPattern[_currentSlotIndex])
                    {
                        _currentSlots[_currentSlotIndex] = _fragments[i].type;
                        _fragments[i].active = false;
                        debugPrintf("SCENES", "Collected correct fragment %c for slot %d", getCharForFragmentType(_fragments[i].type), _currentSlotIndex);
                        if (_effectsManager)
                            _effectsManager->triggerScreenShake(1, 100);
                        _currentSlotIndex++;
                        if (_currentSlotIndex >= PATTERN_LENGTH)
                        {
                            _segmentsWoven++;
                            _successfulWeaveTime = millis();
                            debugPrintf("SCENES", "Segment %d woven!", _segmentsWoven);
                        }
                    }
                    else
                    {
                        debugPrintf("SCENES", "Incorrect fragment %c for slot %d (Needed %c). Fragment lost.", getCharForFragmentType(_fragments[i].type), _currentSlotIndex, getCharForFragmentType(_targetPattern[_currentSlotIndex]));
                        _fragments[i].active = false;
                        if (_effectsManager)
                            _effectsManager->triggerScreenShake(2, 150);
                    }
                    return;
                }
            }
        }
    }
}
void _4ShellWeavingScene::setupNewPattern()
{
    String patternStr = "New target pattern: ";
    for (int i = 0; i < PATTERN_LENGTH; ++i)
    {
        _targetPattern[i] = static_cast<ShellFragmentType>(random(1, (int)ShellFragmentType::_TYPE_COUNT));
        patternStr += getCharForFragmentType(_targetPattern[i]);
    }
    debugPrint("SCENES", patternStr.c_str());
}
void _4ShellWeavingScene::resetSlots()
{
    for (int i = 0; i < PATTERN_LENGTH; ++i)
    {
        _currentSlots[i] = ShellFragmentType::NONE;
    }
    _currentSlotIndex = 0;
}
char _4ShellWeavingScene::getCharForFragmentType(ShellFragmentType type)
{
    switch (type)
    {
    case ShellFragmentType::TYPE_A:
        return 'A';
    case ShellFragmentType::TYPE_B:
        return 'B';
    case ShellFragmentType::TYPE_C:
        return 'C';
    default:
        return ' ';
    }
}
void _4ShellWeavingScene::drawFragments(Renderer &renderer)
{
    if (!_gameContext || !_gameContext->display)
        return;
    U8G2 *u8g2 = _gameContext->display;
    u8g2->setFont(u8g2_font_5x7_tf);
    for (int i = 0; i < MAX_FRAGMENTS; ++i)
    {
        if (_fragments[i].active)
        {
            char str[2] = {_fragments[i].displayChar, '\0'};
            u8g2->drawStr(renderer.getXOffset() + (int)_fragments[i].x, renderer.getYOffset() + (int)_fragments[i].y, str);
        }
    }
}
void _4ShellWeavingScene::drawCollector(Renderer &renderer)
{
    if (!_gameContext || !_gameContext->display || !_gameContext->renderer)
        return;
    U8G2 *u8g2 = _gameContext->display;
    Renderer &r_ref = *_gameContext->renderer;
    int collectorY = r_ref.getHeight() - COLLECTOR_Y_OFFSET - COLLECTOR_HEIGHT;
    u8g2->drawFrame(renderer.getXOffset() + _collectorX, renderer.getYOffset() + collectorY, COLLECTOR_WIDTH, COLLECTOR_HEIGHT);
    u8g2->drawPixel(renderer.getXOffset() + _collectorX + COLLECTOR_WIDTH / 2, renderer.getYOffset() + collectorY - 2);
    u8g2->drawPixel(renderer.getXOffset() + _collectorX + COLLECTOR_WIDTH / 2 - 1, renderer.getYOffset() + collectorY - 1);
    u8g2->drawPixel(renderer.getXOffset() + _collectorX + COLLECTOR_WIDTH / 2 + 1, renderer.getYOffset() + collectorY - 1);
}
void _4ShellWeavingScene::drawWeavingUI(Renderer &renderer)
{
    if (!_gameContext || !_gameContext->display || !_gameContext->renderer)
        return;
    U8G2 *u8g2 = _gameContext->display;
    Renderer &r_ref = *_gameContext->renderer;
    int uiY = renderer.getYOffset() + 5;
    int slotBoxSize = 10;
    int slotSpacing = 4;
    int totalSlotsWidth = PATTERN_LENGTH * slotBoxSize + (PATTERN_LENGTH - 1) * slotSpacing;
    int startX = renderer.getXOffset() + (r_ref.getWidth() - totalSlotsWidth) / 2;
    u8g2->setFont(u8g2_font_6x10_tf);
    for (int i = 0; i < PATTERN_LENGTH; ++i)
    {
        char targetChar[2] = {getCharForFragmentType(_targetPattern[i]), '\0'};
        int charWidth = u8g2->getStrWidth(targetChar);
        int charX = startX + i * (slotBoxSize + slotSpacing) + (slotBoxSize - charWidth) / 2;
        u8g2->drawStr(charX, uiY - 12, targetChar);
    }
    for (int i = 0; i < PATTERN_LENGTH; ++i)
    {
        int currentSlotX = startX + i * (slotBoxSize + slotSpacing);
        u8g2->drawFrame(currentSlotX, uiY, slotBoxSize, slotBoxSize);
        if (_currentSlots[i] != ShellFragmentType::NONE)
        {
            char slotChar[2] = {getCharForFragmentType(_currentSlots[i]), '\0'};
            int charWidth = u8g2->getStrWidth(slotChar);
            u8g2->drawStr(currentSlotX + (slotBoxSize - charWidth) / 2, uiY + 1, slotChar);
        }
        if (i == _currentSlotIndex)
        {
            u8g2->drawFrame(currentSlotX - 1, uiY - 1, slotBoxSize + 2, slotBoxSize + 2);
        }
    }
}
void _4ShellWeavingScene::drawShellProgress(Renderer &renderer)
{
    if (!_gameContext || !_gameContext->display || !_gameContext->renderer)
        return;
    U8G2 *u8g2 = _gameContext->display;
    Renderer &r_ref = *_gameContext->renderer;
    int barWidth = 50;
    int barHeight = 6;
    int barX = renderer.getXOffset() + (r_ref.getWidth() - barWidth) / 2;
    int barY = renderer.getYOffset() + r_ref.getHeight() - COLLECTOR_Y_OFFSET - COLLECTOR_HEIGHT - barHeight - 5;
    u8g2->drawFrame(barX, barY, barWidth, barHeight);
    if (_segmentsWoven > 0)
    {
        int fillWidth = (barWidth - 2) * _segmentsWoven / SEGMENTS_TO_WIN;
        fillWidth = std::max(0, std::min(fillWidth, barWidth - 2));
        u8g2->drawBox(barX + 1, barY + 1, fillWidth, barHeight - 2);
    }
}

void _4ShellWeavingScene::completeStage()
{
    debugPrint("SCENES", "_4ShellWeavingScene: Stage 4 Complete!");
    if (_gameContext && _gameContext->gameStats && _gameContext->sceneManager)
    {
        _gameContext->gameStats->setCompletedPrequelStage(PrequelStage::STAGE_4_SHELLWEAVE_COMPLETE);
        _gameContext->gameStats->setCompletedPrequelStage(PrequelStage::PREQUEL_FINISHED);
        _gameContext->gameStats->addPoints(250);
        _gameContext->gameStats->save();
        _gameContext->sceneManager->requestSetCurrentScene("MAIN");
    }
    else
    {
        debugPrint("SCENES", "Error: Critical context members null in completeStage.");
    }
}

bool _4ShellWeavingScene::handleDialogKeyPress(uint8_t keyCode) {
    if (_dialogBox && _dialogBox->isActive()) {
        if (keyCode == GEM_KEY_OK) {
            _dialogBox->close();
            _instructionsShown = true;
            _stageStartTime = millis();
            return true;
        }
        return true; 
    }
    return false;
}