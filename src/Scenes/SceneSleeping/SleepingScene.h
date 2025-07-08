#ifndef SLEEPING_SCENE_H
#define SLEEPING_SCENE_H

#include "Scene.h"
#include <memory> 
#include "espasyncbutton.hpp" 
#include "character/CharacterManager.h" 
#include "../../ParticleSystem.h"      
#include "../../DialogBox/DialogBox.h" 
#include "../../System/GameContext.h" 

// Forward Declarations
class Renderer;

class SleepingScene : public Scene {
public:
    SleepingScene();
    ~SleepingScene() override;

    DialogBox* getDialogBox() override { return _dialogBox.get(); } 

    void init(GameContext& context); 
    void onEnter() override;
    void onExit() override;
    void update(unsigned long deltaTime) override;
    void draw(Renderer& renderer) override; 

private:
    enum class Phase { SLEEPING, WAKING_UP };
    Phase _currentPhase = Phase::SLEEPING;

    static const uint8_t WAKE_FATIGUE_THRESHOLD = 50;
    static const int FATIGUE_BAR_WIDTH = 80;
    static const int FATIGUE_BAR_HEIGHT = 5;
    static const int FATIGUE_BAR_Y_OFFSET = 3; 
    static const int FATIGUE_BAR_CORNER_RADIUS = 1; 
    static const unsigned long SLEEP_INDICATOR_TOGGLE_MS = 700;
    static const unsigned long FLYING_Z_SPAWN_INTERVAL_MS = 500;
    static const unsigned long FLYING_Z_LIFETIME_MS = 2500;
    static constexpr float FLYING_Z_SPEED_Y = -0.4f;
    static constexpr float FLYING_Z_SPEED_X_VAR = 0.1f;
    static const unsigned long FATIGUE_BAR_ANIM_DURATION_MS = 750; 
    static const unsigned long FATIGUE_BAR_PATTERN_UPDATE_MS = 100; 
    static const int FATIGUE_BAR_PATTERN_STRIPE_WIDTH = 4; 
    static const int FATIGUE_BAR_PATTERN_SPACING = 4; 
    static const unsigned long WAKE_UP_ANIMATION_DURATION_MS = 1500;
    static constexpr float BAR_GRAVITY = 0.15f;


    GameContext* _gameContext = nullptr;

    std::unique_ptr<ParticleSystem> _particleSystem;
    std::unique_ptr<DialogBox> _dialogBox;

    int _targetEggX = 0;
    int _targetEggY = 0;

    bool _sleepIndicatorState = false;
    unsigned long _lastSleepIndicatorToggleTime = 0;
    unsigned long _lastFlyingZSpawnTime = 0;

    float _currentFatigueBarDisplayValue = 0.0f; 
    float _targetFatigueBarDisplayValue = 0.0f;
    float _fatigueBarAnimStartValue = 0.0f; 
    unsigned long _fatigueBarAnimStartTime = 0;
    bool _isFatigueBarAnimating = false;
    int _fatigueBarPatternOffset = 0; 
    unsigned long _lastFatigueBarPatternUpdateTime = 0;

    unsigned long _wakeUpAnimationStartTime = 0;
    float _fatigueBarY = 0.0f;
    float _fatigueBarVY = 0.0f;

    void updateSleepState(unsigned long currentTime, unsigned long deltaTime);
    void drawCharacter(Renderer& renderer);
    void drawSleepIndicator(Renderer& renderer);
    void drawFatigueBar(Renderer& renderer);
    void attemptToWakeUp();
    void updateFatigueBarAnimation(unsigned long currentTime);
    bool handleDialogKeyPress(uint8_t keyCode);

    void onWakeUpAttemptPress();
};

#endif // SLEEPING_SCENE_H