#pragma once

#include "Scene.h"
#include <Arduino.h> 
#include <memory> 
#include "../../System/GameContext.h" 

#define PAW_OVERLAP_CHECK_TRIES 20
#define MAX_PAWS 8

// Forward Declarations
class Renderer; 
class EffectsManager; 

struct PawPosition {
    int x;
    int y;
};

struct BootSceneConfig {
    bool isWakingUp = false;
    int sleepDurationMinutes = 0;
};

class BootScene : public Scene {
public:
    BootScene();
    ~BootScene() override; 
    
    // This is a scene-specific init, not an override of the base class.
    void init(GameContext& context); 
    void onEnter() override;
    void onExit() override;
    void update(unsigned long deltaTime) override;
    void draw(Renderer& renderer) override; 

    void setConfig(const BootSceneConfig& config);

private:
    // Scene-specific context pointer
    GameContext* _gameContext = nullptr;

    std::unique_ptr<EffectsManager> _effectsManager; 

    bool checkOverlap(int newX, int newY);
    unsigned long tickCounter;
    unsigned long lastPawTick;
    int pawsDrawn;
    const int maxTicks = 60; 
    const int ticksPerPaw = 8;

    PawPosition pawPositions[MAX_PAWS]; 

    static const unsigned char pawBitmap[];
    static const int PAW_WIDTH = 13;
    static const int PAW_HEIGHT = 14;

    BootSceneConfig _sceneConfig; 
    bool _isWakingUp = false; 
    int _sleepDurationMinutes = 0; 

    const unsigned long _fadeOutDurationMs = 1000; 
    bool _transitionRequested = false; 
};