#pragma once

#include "Scene.h"
#include "GEM_u8g2.h"
#include "espasyncbutton.hpp"
#include <memory> // For unique_ptr
#include "../../DebugUtils.h"
#include "../../System/GameContext.h" // For GameContext

// Forward Declarations
class U8G2;

// Width for gauge bar characters (adjust as needed)
#define GAUGE_CHAR_WIDTH 10

class StatsScene : public Scene {
public:
    StatsScene();
    ~StatsScene() override;

    // This is a scene-specific init, not an override of the base class.
    void init(GameContext& context); 
    void onEnter() override;
    void onExit() override;
    void update(unsigned long deltaTime) override;
    void draw(Renderer& renderer) override;

    bool usesKeyQueue() const override { return true; }
    void processKeyPress(uint8_t keyCode) override;

    std::unique_ptr<GEM_u8g2> menu;

private:
    // Scene-specific context pointer
    GameContext* _gameContext = nullptr;

    GEMPage menuPageStats;

    // Buffers for formatted strings
    char ageBuffer[16];
    char healthBuffer[32];
    char happyBuffer[32];
    char dirtyBuffer[32];
    char sicknessBuffer[24];
    char fatigueBuffer[32];
    char hungerBuffer[32];
    char weightBuffer[16];
    char playTimeBuffer[24];
    char sleepBuffer[16];
    char poopBuffer[16]; 
    char exitHintBuffer[24];

    // GEMItems (Labels pointing to buffers)
    GEMItem menuItemAge;
    GEMItem menuItemHealth;
    GEMItem menuItemHappy;
    GEMItem menuItemDirty;
    GEMItem menuItemSickness;
    GEMItem menuItemFatigue;
    GEMItem menuItemHunger;
    GEMItem menuItemWeight;
    GEMItem menuItemPlayTime;
    GEMItem menuItemSleep;
    GEMItem menuItemPoop; 
    GEMItem menuItemExitHint;

    U8G2* u8g2_ptr = nullptr;
    uint8_t fontWidth = 4;
    uint8_t gaugeWidthChars = GAUGE_CHAR_WIDTH;

    // Button Callbacks
    void onButtonOkLongPress();

    // Helper to update buffer values
    void updateStatBuffers();
};