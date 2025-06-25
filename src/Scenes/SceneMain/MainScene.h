#pragma once
#include "Scene.h"
#include <vector>
#include <memory>
#include "espasyncbutton.hpp"
#include "../../Animator.h"
#include "character/CharacterManager.h"
#include "../../Weather/WeatherManager.h"
#include "FastNoiseLite.h"
#include "IconMenuManager.h"
#include "../../DialogBox/DialogBox.h"
#include "../../Helper/PathGenerator.h"
#include "IdleAnimationController.h" 
#include "../../System/GameContext.h" 

// Forward Declarations
class Renderer;

class MainScene : public Scene {
public:
    enum class AnimationPhase
    {
        PHASE_FALLING,
        DOWNING,
        PHASE_IDLE
    };

    MainScene();

    // This is a scene-specific init, not an override of the base class.
    void init(GameContext& context); 
    void update(unsigned long deltaTime) override;
    void draw(Renderer& renderer) override;
    void onEnter() override;
    void onExit() override;
    SceneType getSceneType() const override { return SceneType::MAIN; }

    bool triggerSnoozeAnimation();
    bool triggerLeanAnimation();
    bool triggerPathAnimation();

    DialogBox *getDialogBox() override { return _dialogBox.get(); } 

private:
    static const unsigned long FALLING_ANIMATION_DURATION_MS = 500;
    static const unsigned long MIN_IDLE_ANIM_INTERVAL_MS = 20000;
    static const unsigned long MAX_IDLE_ANIM_INTERVAL_MS = 60000;
    static const int MENU_BAR_HEIGHT = IconMenuManager::ICON_HEIGHT + 2 * IconMenuManager::ICON_SPACING;
    static constexpr float MENU_ANIMATION_SPEED = 8.0f; 

    std::unique_ptr<Animator> currentAnimation; 
    AnimationPhase currentPhase = AnimationPhase::PHASE_IDLE;
    int targetEggX = 0;
    int targetEggY = 0;

    unsigned long _nextIdleAnimTime = 0;
    std::unique_ptr<IdleAnimationController> _idleAnimController; 

    std::unique_ptr<IconMenuManager> _iconMenuManager;
    bool showMenus = false;
    float _topMenuYCurrent;
    float _bottomMenuYCurrent;
    float _topMenuYTarget;
    float _bottomMenuYTarget;

    std::unique_ptr<DialogBox> _dialogBox;
    bool _isFirstEntry = true;

    FastNoiseLite _noise;
    
    // Scene-specific context pointer
    GameContext* _gameContext = nullptr;

    void scheduleNextIdleAnimation(unsigned long currentTime);
    void handleMenuAction(IconAction action);
    void drawSicknessOverlay(Renderer &renderer);
    void attemptToSleep();
    void updateMenuAnimations(float dt); 

    void onLeftClick();
    void onRightClick();
    void onSelectClick();
};