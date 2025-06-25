#include "BootScene.h"
#include "Renderer.h"
#include <U8g2lib.h>
#include <Arduino.h> 
#include "SerialForwarder.h"
#include "GameStats.h" 
#include "Localization.h" 
#include "../../Helper/EffectsManager.h" 
#include "../../DebugUtils.h"
#include "../../System/GameContext.h" 
#include "SceneManager.h" 


extern String nextSceneName; 
extern bool replaceSceneStack;
extern bool sceneChangeRequested;

const unsigned char BootScene::pawBitmap[] = {
	0x10, 0x01, 0xb8, 0x03, 0xb8, 0x03, 0xb8, 0x03, 0x12, 0x09, 0x07, 0x1c, 0xe7, 0x1c, 0xf3, 0x19,
	0xf8, 0x03, 0xfc, 0x07, 0xfe, 0x0f, 0xfe, 0x0f, 0xfc, 0x07, 0xb8, 0x03 };


BootScene::BootScene() :
    tickCounter(0),
    lastPawTick(0),
    pawsDrawn(0),
    _isWakingUp(false), 
    _sleepDurationMinutes(0),
    _transitionRequested(false) 
{}

BootScene::~BootScene() {
    debugPrint("SCENES", "BootScene destroyed.");
}

void BootScene::setConfig(const BootSceneConfig& config) {
    _sceneConfig = config;
    _isWakingUp = _sceneConfig.isWakingUp; 
    _sleepDurationMinutes = _sceneConfig.sleepDurationMinutes; 
    debugPrintf("SCENES", "BootScene: Config set - WakeUpMode: %s, SleepDuration: %d mins",
               _isWakingUp ? "Waking" : "Booting", _sleepDurationMinutes);
}

bool BootScene::checkOverlap(int newX, int newY) {
    for (int i = 0; i < pawsDrawn; ++i) {
        int overlapBias = 2;
        bool xOverlap = (newX < pawPositions[i].x + PAW_WIDTH + overlapBias) && (newX + PAW_WIDTH + overlapBias > pawPositions[i].x);
        bool yOverlap = (newY < pawPositions[i].y + PAW_HEIGHT+ overlapBias) && (newY + PAW_HEIGHT + overlapBias > pawPositions[i].y);
        if (xOverlap && yOverlap) return true;
    }
    return false;
}

void BootScene::init(GameContext& context) { 
    Scene::init(); 
    _gameContext = &context;
    debugPrint("SCENES", "BootScene::init");
    
    if (!_gameContext || !_gameContext->renderer) {
        debugPrint("SCENES", "ERROR: BootScene::init - Critical context members (renderer) are null!");
        return;
    }
    _effectsManager.reset(new EffectsManager(*_gameContext->renderer)); 
    if (!_effectsManager) {
        debugPrint("SCENES", "ERROR: BootScene::init - Failed to create EffectsManager!");
    }

    _isWakingUp = _sceneConfig.isWakingUp;
    _sleepDurationMinutes = _sceneConfig.sleepDurationMinutes;

    tickCounter = 0;
    lastPawTick = 0;
    pawsDrawn = 0;
    _transitionRequested = false; 
    memset(pawPositions, 0, sizeof(pawPositions));

    randomSeed(esp_random());
    debugPrint("SCENES", "BootScene: Random seed set using esp_random().");
}

void BootScene::onEnter() {
    _isWakingUp = _sceneConfig.isWakingUp; 
    _sleepDurationMinutes = _sceneConfig.sleepDurationMinutes;
    debugPrintf("SCENES", "BootScene::onEnter - WakeUpMode: %s, SleepDuration: %d mins",
               _isWakingUp ? "Waking" : "Booting", _sleepDurationMinutes);
    tickCounter = 0;
    lastPawTick = 0;
    pawsDrawn = 0;
    _transitionRequested = false; 
    if (_effectsManager) {
        _effectsManager->resetFadeOut(); 
    } else {
        debugPrint("SCENES", "ERROR: BootScene::onEnter - _effectsManager is NULL!");
    }
}

void BootScene::onExit() {
    debugPrint("SCENES", "BootScene::onExit.");
    _transitionRequested = false; 
}


void BootScene::update(unsigned long deltaTime) {
    if (_transitionRequested) return; 

    if (!_gameContext || !_gameContext->renderer || !_effectsManager || !_gameContext->sceneManager) { 
        debugPrint("SCENES", "ERROR: BootScene::update - Critical context members are NULL!");
        return;
    }

    unsigned long currentTime = millis();
    _effectsManager->update(currentTime); 

    if (_effectsManager->isFadeOutToBlackCompleted()) { 
        debugPrint("SCENES", "BootScene: Fade out reported complete by EffectsManager. Requesting POST_BOOT_TRANSITION.");
        _gameContext->sceneManager->requestSetCurrentScene("POST_BOOT_TRANSITION"); 
        _transitionRequested = true; 
        return; 
    }

    if (_effectsManager->isFadingOutToBlack()) { 
        return; 
    }
    
    tickCounter++;

    if (tickCounter >= maxTicks) {
        if (!_effectsManager->isFadingOutToBlack()) { 
            debugPrint("SCENES", "BootScene: Paw animation complete. Triggering fade out via EffectsManager.");
            _effectsManager->triggerFadeOutToBlack(_fadeOutDurationMs);
        }
    } else { 
        if (pawsDrawn < MAX_PAWS && tickCounter >= lastPawTick + ticksPerPaw) {
             int screenWidth = _gameContext->renderer->getWidth(); 
             int screenHeight = _gameContext->renderer->getHeight(); 
            int x = 0, y = 0;
            bool positionFound = false;
            for(int tries = 0; tries < PAW_OVERLAP_CHECK_TRIES; ++tries) {
                x = random(0, screenWidth - PAW_WIDTH);
                y = random(0, screenHeight - PAW_HEIGHT - 16); 
                if (!checkOverlap(x, y)) { positionFound = true; break; }
            }
            pawPositions[pawsDrawn].x = x;
            pawPositions[pawsDrawn].y = y;
            pawsDrawn++;
            lastPawTick = tickCounter;
        }
    }
}

void BootScene::draw(Renderer& renderer) { 
    if (!_gameContext || !_gameContext->display || !_effectsManager) { 
        debugPrint("SCENES", "ERROR: BootScene::draw - Critical context members are NULL!");
        return;
    }
    U8G2* u8g2 = _gameContext->display; 

    u8g2->setDrawColor(1);

    for (int i = 0; i < pawsDrawn; ++i) {
        u8g2->drawXBMP(renderer.getXOffset() + pawPositions[i].x,
                        renderer.getYOffset() + pawPositions[i].y,
                        PAW_WIDTH, PAW_HEIGHT,
                        pawBitmap);
    }

    const char* mainDisplayText = _isWakingUp ? loc(StringKey::BOOT_WAKING_UP) : loc(StringKey::BOOT_BOOTING);
    u8g2->setFont(u8g2_font_5x7_tf); 
    int screenWidth = renderer.getWidth();
    int screenHeight = renderer.getHeight();

    u8g2_uint_t mainTextWidth = u8g2->getStrWidth(mainDisplayText);
    u8g2_uint_t mainTextX = (screenWidth - mainTextWidth) / 2;
    u8g2_uint_t fontHeight = u8g2->getMaxCharHeight(); 
    u8g2_uint_t textBottomMargin = 2; 
    u8g2_uint_t mainTextY;

    if (_isWakingUp && _sleepDurationMinutes > 0) {
        mainTextY = screenHeight - (2 * fontHeight + 1 + textBottomMargin); 
    } else {
        mainTextY = screenHeight - (fontHeight + textBottomMargin); 
    }
    
    u8g2->drawStr(renderer.getXOffset() + mainTextX, renderer.getYOffset() + mainTextY, mainDisplayText);

    if (_isWakingUp && _sleepDurationMinutes > 0) {
        char durationStr[40]; 
        if (_sleepDurationMinutes < 60) {
            snprintf(durationStr, sizeof(durationStr), "%s: %d min", loc(StringKey::BOOT_SLEPT_FOR), _sleepDurationMinutes);
        } else {
            int hours = _sleepDurationMinutes / 60;
            int minutes = _sleepDurationMinutes % 60;
            snprintf(durationStr, sizeof(durationStr), "%s: %dh %dm", loc(StringKey::BOOT_SLEPT_FOR), hours, minutes);
        }
        u8g2_uint_t durationTextWidth = u8g2->getStrWidth(durationStr);
        u8g2_uint_t durationTextX = (screenWidth - durationTextWidth) / 2;
        u8g2_uint_t durationTextY = mainTextY + fontHeight + 1; 
        u8g2->drawStr(renderer.getXOffset() + durationTextX, renderer.getYOffset() + durationTextY, durationStr);
    }

    _effectsManager->drawFadeOutOverlay();
}