#ifndef JOURNEY_WITHIN_SCENE_H
#define JOURNEY_WITHIN_SCENE_H

#include "Scene.h"
#include "FastNoiseLite.h"
#include "../../DialogBox/DialogBox.h"
#include "../../Helper/EffectsManager.h"
#include "../../ParticleSystem.h"
#include <memory>
#include <vector>
#include "../../System/GameContext.h"

// Forward Declarations
class Renderer;

enum class JourneyPhase
{
    CALM_FLOW,
    THICKET,
    RAPID_CURRENT,
    AT_GATE_QTE,
    GATE_OPENING,
    STAGE_COMPLETE_ANIM
};
enum class QTEButton
{
    NONE,
    LEFT,
    RIGHT,
    OK
};

class _3JourneyWithinScene : public Scene
{
public:
    _3JourneyWithinScene();
    ~_3JourneyWithinScene() override;

    // This is a scene-specific init, not an override of the base class.
    void init(GameContext &context);
    void onEnter() override;
    void onExit() override;
    void update(unsigned long deltaTime) override;
    void draw(Renderer &renderer) override; 

    DialogBox *getDialogBox() override { return _dialogBox.get(); }

private:
    // --- Constants ---
    static constexpr float PLAYER_RADIUS = 3.0f;
    static constexpr float TARGET_PROGRESS = 1000.0f;
    static constexpr float PROGRESS_PER_SECOND_BASE = 10.0f;
    static constexpr float DRAG_FACTOR = 0.96f;
    static constexpr float CALM_FORCE_MAGNITUDE = 0.025f;
    static constexpr float CALM_STEER_FORCE = 0.06f;
    static constexpr float THICKET_MOVE_SPEED = 0.4f;
    static constexpr float THICKET_ROTATE_SPEED_DEG = 3.5f;
    static constexpr float THICKET_DRAG = 0.88f;
    static constexpr float RAPID_FORCE_MAGNITUDE = 0.11f;
    static constexpr float RAPID_RESIST_FORCE = 0.05f;
    static constexpr float RAPID_COLLISION_VELOCITY_THRESHOLD = 0.2f;
    static constexpr unsigned long BOOST_DURATION_MS = 350;
    static constexpr unsigned long BOOST_COOLDOWN_MS = 1200;
    static constexpr float BOOST_FORCE_MAGNITUDE = 0.18f;
    static constexpr float BOOST_NUDGE_THICKET = 1.2f;
    static constexpr int GATE_COUNT = 2;
    static const float _gatePositions[GATE_COUNT]; 

    static constexpr int QTE_SEQUENCE_LENGTH = 3;
    static constexpr unsigned long QTE_STEP_TIME_LIMIT_MS = 2000;
    static constexpr float GATE_PROGRESS_PENALTY = 20.0f;
    static constexpr unsigned long GATE_OPEN_ANIM_DURATION_MS = 500;
    static constexpr unsigned long PHASE_TRANSITION_GRACE_MS = 750;

    // --- Zone Thresholds ---
    static constexpr float THICKET_NOISE_THRESHOLD = -0.25f;
    static constexpr float RAPID_NOISE_THRESHOLD = 0.35f;   

    // --- State Variables ---
    FastNoiseLite _zoneNoise;
    FastNoiseLite _currentNoise;
    FastNoiseLite _thicketElementNoise;
    float _noiseOffsetX = 0.0f;
    float _noiseOffsetY = 0.0f;
    float _backgroundScrollX = 0.0f;
    float _backgroundScrollY = 0.0f;

    float _playerX = 0.0f;
    float _playerY = 0.0f;
    float _playerAngle = 0.0f;
    float _playerVX = 0.0f;
    float _playerVY = 0.0f;

    JourneyPhase _currentPhase = JourneyPhase::CALM_FLOW;
    JourneyPhase _phaseBeforeGate = JourneyPhase::CALM_FLOW;
    unsigned long _phaseTransitionGraceTime = 0;
    float _journeyProgress = 0.0f;
    unsigned long _lastUpdateTime = 0;

    bool _isBoosting = false;
    unsigned long _boostEndTime = 0;
    unsigned long _boostCooldownTime = 0;

    bool _gateActive[GATE_COUNT] = {false, false};
    bool _gatePassed[GATE_COUNT] = {false, false};
    QTEButton _qteSequence[QTE_SEQUENCE_LENGTH];
    int _qteCurrentStep = 0;
    unsigned long _qteStepStartTime = 0;
    unsigned long _gateAnimEndTime = 0;

    std::unique_ptr<DialogBox> _dialogBox;
    std::unique_ptr<EffectsManager> _effectsManager;
    std::unique_ptr<ParticleSystem> _particleSystem;
    bool _instructionsShown = false;

    // Scene-specific context pointer
    GameContext* _gameContext = nullptr;

    void updateCurrentRiding(unsigned long currentTime, float dt);
    void updateThicketNavigation(unsigned long currentTime, float dt);
    void updateRapidCurrent(unsigned long currentTime, float dt);
    void updateGateQTE(unsigned long currentTime, float dt);
    void applyPhysics(float dt);
    void checkZoneTransition();
    void drawPlayer(Renderer &renderer, int rOffsetX, int rOffsetY);
    void drawBackgroundAndZones(Renderer &renderer, int rOffsetX, int rOffsetY);
    void drawProgressBar(Renderer &renderer);
    void drawGate(Renderer &renderer, int rOffsetX, int rOffsetY, int gateIndex);
    void drawQTEUI(Renderer &renderer);
    void completeStage();
    void triggerGate(int gateIndex, unsigned long currentTime);
    void generateQTESequence();
    void handleQTEInput(QTEButton pressedButton);
    bool handleDialogKeyPress(uint8_t keyCode);

    void handleButton1(unsigned long currentTime);
    void handleButton2(unsigned long currentTime);
    void handleButton3(unsigned long currentTime);
};

#endif // JOURNEY_WITHIN_SCENE_H