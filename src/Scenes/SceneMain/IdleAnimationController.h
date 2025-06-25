#ifndef IDLE_ANIMATION_CONTROLLER_H
#define IDLE_ANIMATION_CONTROLLER_H
#include <memory>
#include "../../Animator.h"                   // Needs Animator definition
#include "../../character/CharacterManager.h" // Needs CharacterManager definition
#include "../../Helper/PathGenerator.h"       // Needs PathGenerator definition
#include "Renderer.h"                         // Needs Renderer definition
#include "../../DebugUtils.h"
class IdleAnimationController
{
public:
    enum class CurrentIdleAnimType
    {
        NONE,
        SNOOZE,
        LEAN,
        PATH
    };
    // Re-define SnoozeState here as it's specific to this controller now
    enum class SnoozeStateInternal
    {
        NONE,
        SNOOZE_1,
        SNOOZE_2
    };

    IdleAnimationController(Renderer &renderer, CharacterManager *charMgr, PathGenerator *pathGen);
    ~IdleAnimationController();

    void startRandomIdleAnimation(unsigned long currentTime, int targetX, int targetY);
    bool triggerSnoozeAnimation(unsigned long currentTime, int targetX, int targetY);
    bool triggerLeanAnimation(unsigned long currentTime, int targetX, int targetY);
    bool triggerPathAnimation(unsigned long currentTime); // Path anim typically doesn't need targetX/Y for its own start

    bool update(unsigned long currentTime); // Returns true if still animating
    void draw();
    void reset();
    bool isAnimating() const;
    CurrentIdleAnimType getActiveAnimationType() const { return _activeAnimType; }

private:
    Renderer &_renderer;
    CharacterManager *_characterManager;
    PathGenerator *_pathGenerator; // Pointer, as MainScene owns the instance
    std::unique_ptr<Animator> _currentAnimator;

    SnoozeStateInternal _snoozeState; // Use internal SnoozeState
    unsigned long _snoozeFrameEndTime;
    const unsigned char *_snoozeBitmap1;
    const unsigned char *_snoozeBitmap2;

    CurrentIdleAnimType _activeAnimType;

    // Constants for idle animations
    static const unsigned long SNOOZE_FRAME_DURATION_MS = 750;
    static const unsigned long LEAN_DURATION_MS = 400;
    static const unsigned long LEAN_REVERSE_DELAY_MS = 200;
    static constexpr float MAX_LEAN_ANGLE = 10.0f;
    static const int MAX_LEAN_OFFSET_X = 4;
    // Path animation durations are determined by PathGenerator
};
#endif // IDLE_ANIMATION_CONTROLLER_H
