#include "StatsScene.h"
#include "SceneManager.h" 
#include "InputManager.h"
#include "Renderer.h"
#include "../../GameStats.h"        
#include "../../Localization.h"    
#include "GEM_u8g2.h"
#include <memory>
#include <stdio.h>
#include <cstring>
#include <algorithm>
#include "../../SerialForwarder.h" 
#include "../../character/CharacterManager.h" 
#include "../../DebugUtils.h"
#include "../../System/GameContext.h" 

StatsScene::StatsScene() :
    menuPageStats(""), 
    menuItemAge(ageBuffer),
    menuItemHealth(healthBuffer),
    menuItemHappy(happyBuffer),
    menuItemDirty(dirtyBuffer),
    menuItemSickness(sicknessBuffer),
    menuItemFatigue(fatigueBuffer),
    menuItemHunger(hungerBuffer),
    menuItemWeight(weightBuffer),
    menuItemPlayTime(playTimeBuffer),
    menuItemSleep(sleepBuffer),
    menuItemPoop(poopBuffer), 
    menuItemExitHint(exitHintBuffer)
{
    strcpy(ageBuffer, "Age: N/A");
    strcpy(healthBuffer, "Health: [          ]");
    strcpy(happyBuffer,  "Happy:  [          ]");
    strcpy(dirtyBuffer,  "Dirty:  [          ]");
    strcpy(sicknessBuffer, "Sick: None");
    strcpy(fatigueBuffer, "Tired:  [          ]");
    strcpy(hungerBuffer, "Hunger: [..........]");
    strcpy(weightBuffer, "Weight: N/A");
    strcpy(playTimeBuffer, "Play: N/A");
    strcpy(sleepBuffer, "Sleep: No");
    strcpy(poopBuffer, "Poop: 0"); 
    strcpy(exitHintBuffer, "(Long OK to exit)");
    managesOwnDrawing = true;
}

StatsScene::~StatsScene() { debugPrint("SCENES", "StatsScene destroyed."); }

void StatsScene::init(GameContext& context) { 
    Scene::init(); 
    _gameContext = &context;
    debugPrint("SCENES", "StatsScene::init");

    if (!_gameContext || !_gameContext->renderer || !_gameContext->display) {
        debugPrint("SCENES", "ERROR: StatsScene::init - Critical context members (renderer/display) are null!");
        return;
    }
    u8g2_ptr = _gameContext->display; 

    if (!menu) {
        debugPrint("SCENES", "StatsScene: Initializing GEM object.");
        menu.reset(new GEM_u8g2(*u8g2_ptr, GEM_POINTER_ROW, GEM_ITEMS_COUNT_AUTO));
        menu->setFontSmall(); fontWidth = 4; 
        GEMAppearance* appearance = menu->getCurrentAppearance();
        
        int availablePixels = u8g2_ptr->getDisplayWidth() - appearance->menuValuesLeftOffset - 3;
        gaugeWidthChars = (availablePixels > 0 && fontWidth > 0) ? (availablePixels / fontWidth) : 1;
        int maxGaugeChars = sizeof(healthBuffer) - 10; 
        gaugeWidthChars = std::min((int)gaugeWidthChars, maxGaugeChars);
        gaugeWidthChars = std::max(1, (int)gaugeWidthChars);
        debugPrintf("SCENES", "StatsScene: Calculated Gauge Width = %d chars", gaugeWidthChars);

        menuItemAge.setReadonly(); menuItemHealth.setReadonly(); menuItemHappy.setReadonly(); 
        menuItemDirty.setReadonly(); menuItemSickness.setReadonly(); menuItemFatigue.setReadonly(); 
        menuItemHunger.setReadonly(); menuItemWeight.setReadonly(); menuItemPlayTime.setReadonly(); 
        menuItemSleep.setReadonly(); menuItemPoop.setReadonly(); menuItemExitHint.setReadonly();
        
        menuPageStats.addMenuItem(menuItemAge); menuPageStats.addMenuItem(menuItemHealth); 
        menuPageStats.addMenuItem(menuItemHappy); menuPageStats.addMenuItem(menuItemDirty); 
        menuPageStats.addMenuItem(menuItemHunger); menuPageStats.addMenuItem(menuItemSickness); 
        menuPageStats.addMenuItem(menuItemFatigue); menuPageStats.addMenuItem(menuItemWeight); 
        menuPageStats.addMenuItem(menuItemPlayTime); menuPageStats.addMenuItem(menuItemSleep); 
        menuPageStats.addMenuItem(menuItemPoop); menuPageStats.addMenuItem(menuItemExitHint);
        
        menu->setMenuPageCurrent(menuPageStats); 
        menu->init();
    } else { 
        debugPrint("SCENES", "StatsScene: GEM object already initialized."); 
        menu->reInit(); 
    }

    updateStatBuffers();

    _gameContext->inputManager->registerButtonListener(EDGE_Button::OK, EDGE_Event::LONG_PRESS, this, [this](){ this->onButtonOkLongPress(); });
    debugPrint("SCENES", "StatsScene GEM Initialized");
}

void StatsScene::onEnter() {
    debugPrint("SCENES", "StatsScene::onEnter - Updating stats");
    updateStatBuffers();
    if (menu) { 
        menuPageStats.setTitle(loc(StringKey::STATS_TITLE)); 
        menu->drawMenu(); 
    }
    else { debugPrint("SCENES", "StatsScene::onEnter - Error: menu is null!"); }
}
void StatsScene::onExit() { 
    debugPrint("SCENES", "StatsScene::onExit"); 
    if (_gameContext && _gameContext->inputManager) {
        _gameContext->inputManager->unregisterAllListenersForScene(this);
    }
}

void StatsScene::update(unsigned long deltaTime) {
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate > 500) { 
        updateStatBuffers();
        lastUpdate = millis();
     }
}
void StatsScene::draw(Renderer& renderer) { if (menu) menu->drawMenu(); }

void StatsScene::processKeyPress(uint8_t keyCode) { 
    if (menu) { 
        menu->registerKeyPress(keyCode); 
    } else { 
        debugPrint("SCENES", "StatsScene::processKeyPress - Error: menu is null!"); 
    } 
}

void StatsScene::onButtonOkLongPress() { 
    debugPrint("SCENES", "StatsScene: Requesting exit back to Main Scene via Long OK."); 
    if (_gameContext && _gameContext->sceneManager) { 
        _gameContext->sceneManager->requestSetCurrentScene("MAIN");
    }
}

void StatsScene::updateStatBuffers() {
    if (!_gameContext || !_gameContext->gameStats || !_gameContext->characterManager) { 
        debugPrint("SCENES", "Error: Critical context members NULL in updateStatBuffers!");
        return; 
    }
    GameStats* gs = _gameContext->gameStats;
    CharacterManager* cm = _gameContext->characterManager;

    snprintf(ageBuffer, sizeof(ageBuffer), "%s: %u", loc(StringKey::STAT_AGE), gs->age);
    snprintf(weightBuffer, sizeof(weightBuffer), "%s: %u g", loc(StringKey::STAT_WEIGHT), gs->weight);
    snprintf(playTimeBuffer, sizeof(playTimeBuffer), "%s: %u min", loc(StringKey::STAT_PLAYTIME), gs->playingTimeMinutes);
    
    if (cm->isSicknessAvailable(gs->sickness)) {
        snprintf(sicknessBuffer, sizeof(sicknessBuffer), "%s: %s", loc(StringKey::STAT_SICKNESS), gs->getSicknessString());
    } else {
         snprintf(sicknessBuffer, sizeof(sicknessBuffer), "%s: %s", loc(StringKey::STAT_SICKNESS), loc(StringKey::SICKNESS_NONE)); 
    }
    
    snprintf(sleepBuffer, sizeof(sleepBuffer), "%s: %s", loc(StringKey::STAT_SLEEP), gs->isSleeping ? loc(StringKey::YES) : loc(StringKey::NO));
    
    if (cm->currentLevelCanPoop()) {
        snprintf(poopBuffer, sizeof(poopBuffer), "%s: %u", loc(StringKey::STAT_POOP), gs->poopCount); 
    } else {
        snprintf(poopBuffer, sizeof(poopBuffer), "%s: N/A", loc(StringKey::STAT_POOP));
    }

    char gaugeBar[gaugeWidthChars + 1];

    uint8_t modHappy = gs->getModifiedHappiness();
    uint8_t baseHealth = gs->health; 
    uint8_t baseFatigue = gs->fatigue; 
    uint8_t baseDirty = gs->dirty;     
    uint8_t baseHunger = gs->hunger;   

    int healthChars = (baseHealth * gaugeWidthChars) / 100; healthChars = std::max(0, std::min((int)gaugeWidthChars, healthChars)); memset(gaugeBar, '#', healthChars); memset(gaugeBar + healthChars, '.', gaugeWidthChars - healthChars); gaugeBar[gaugeWidthChars] = '\0'; snprintf(healthBuffer, sizeof(healthBuffer), "%s:[%s]", loc(StringKey::STAT_HEALTH), gaugeBar);
    int happyChars = (modHappy * gaugeWidthChars) / 100; happyChars = std::max(0, std::min((int)gaugeWidthChars, happyChars)); memset(gaugeBar, '#', happyChars); memset(gaugeBar + happyChars, '.', gaugeWidthChars - happyChars); gaugeBar[gaugeWidthChars] = '\0'; snprintf(happyBuffer, sizeof(happyBuffer), "%s: [%s]", loc(StringKey::STAT_HAPPY), gaugeBar);
    int dirtyChars = (baseDirty * gaugeWidthChars) / 100; dirtyChars = std::max(0, std::min((int)gaugeWidthChars, dirtyChars)); memset(gaugeBar, '#', dirtyChars); memset(gaugeBar + dirtyChars, '.', gaugeWidthChars - dirtyChars); gaugeBar[gaugeWidthChars] = '\0'; snprintf(dirtyBuffer, sizeof(dirtyBuffer), "%s: [%s]", loc(StringKey::STAT_DIRTY), gaugeBar);
    int energyChars = ((100 - baseFatigue) * gaugeWidthChars) / 100; energyChars = std::max(0, std::min((int)gaugeWidthChars, energyChars)); memset(gaugeBar, '#', energyChars); memset(gaugeBar + energyChars, '.', gaugeWidthChars - energyChars); gaugeBar[gaugeWidthChars] = '\0'; snprintf(fatigueBuffer, sizeof(fatigueBuffer), "%s:[%s]", loc(StringKey::STAT_TIRED), gaugeBar);
    
    if (cm->currentLevelCanBeHungry()) {
        int hungerChars = ((100 - baseHunger) * gaugeWidthChars) / 100; hungerChars = std::max(0, std::min((int)gaugeWidthChars, hungerChars)); memset(gaugeBar, '#', hungerChars); memset(gaugeBar + hungerChars, '.', gaugeWidthChars - hungerChars); gaugeBar[gaugeWidthChars] = '\0'; snprintf(hungerBuffer, sizeof(hungerBuffer), "%s:[%s]", loc(StringKey::STAT_HUNGER), gaugeBar);
    } else {
        snprintf(hungerBuffer, sizeof(hungerBuffer), "%s: N/A", loc(StringKey::STAT_HUNGER));
    }

    snprintf(exitHintBuffer, sizeof(exitHintBuffer), "%s", loc(StringKey::HINT_EXIT));
}