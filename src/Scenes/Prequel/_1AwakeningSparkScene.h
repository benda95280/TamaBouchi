#ifndef AWAKENING_SPARK_SCENE_H
#define AWAKENING_SPARK_SCENE_H

#include "Scene.h"
#include "FastNoiseLite.h" 
#include "../../DialogBox/DialogBox.h" 
#include <memory> 
#include <vector> 
#include "../../ParticleSystem.h" 
#include "../../Helper/EffectsManager.h" 
#include "../../System/GameContext.h" 

// Forward Declarations
class Renderer;

class _1AwakeningSparkScene : public Scene {
public:
    _1AwakeningSparkScene();
    ~_1AwakeningSparkScene() override;

    SceneType getSceneType() const override { return SceneType::PREQUEL_STAGE_1; }

    // This is a scene-specific init method, not an override of the base class.
    void init(GameContext& context); 
    void onEnter() override;
    void onExit() override;
    void update(unsigned long deltaTime) override;
    void draw(Renderer& renderer) override; // Renderer& stays for drawing

    DialogBox* getDialogBox() override { return _dialogBox.get(); }


private:
    FastNoiseLite _noise;
    FastNoiseLite _pulseNoise;
    FastNoiseLite _badQuantaWaveNoise;
    float _noiseOffsetX = 0.0f;
    float _noiseOffsetY = 0.0f;
    float _baseBackgroundScrollSpeedX = 0.003f;
    float _baseBackgroundScrollSpeedY = 0.0015f;
    unsigned long _startTime = 0;
    bool _sparkIsInNebula = false;
    int _currentDifficultyLevel = 0;

    float _sparkX, _sparkY;
    int _sparkSize = 2;
    float _sparkEnergy = 0.0f;
    static constexpr float ENERGY_TO_COLLECT = 400.0f; 
    bool _isPulsing = false;
    unsigned long _pulseAnimStartTime = 0;
    static const unsigned long PULSE_ANIM_DURATION_MS = 250;
    static const int PULSE_MAX_RADIUS = 20;
    static constexpr float PULSE_ENERGY_COST = 5.0f;
    int _sparkCoreAnimCounter = 0;
    bool _anyQuantaUnderSpark = false;

    struct Quanta {
        float x, y; float vx = 0.0f, vy = 0.0f; unsigned long nextMoveTime = 0;
        bool active = false; int size = 1; float actualRadius = 0.4f;
        unsigned long spawnTime; unsigned long lastPulseTime = 0;
        bool pulseIncreasing = true; float currentPulseSize = 0.0f;
        bool isInvertedDraw = false; int clusterId = -1; bool isClusterLead = false;
        float targetClusterOffsetX = 0; float targetClusterOffsetY = 0;
    };
    static const int MAX_QUANTA = 15;
    Quanta _quantaPool[MAX_QUANTA];
    unsigned long _lastQuantaSpawnTime = 0;
    static const unsigned long QUANTA_SPAWN_INTERVAL_MS = 700; 
    static const unsigned long QUANTA_LIFETIME_MS = 12000;
    static const unsigned long QUANTA_PULSE_INTERVAL_MS = 750;
    static constexpr float QUANTA_PULSE_MAX_ADDITIONAL_SIZE = 0.2f;
    static const unsigned long QUANTA_MIN_MOVE_INTERVAL_MS = 3000;
    static const unsigned long QUANTA_MAX_MOVE_INTERVAL_MS = 6000;
    static constexpr float QUANTA_MAX_SPEED = 0.05f;
    static constexpr float ENERGY_PER_QUANTA_SIZE_1 = 1.5f; 
    static constexpr float ENERGY_PER_QUANTA_SIZE_2 = 2.5f; 
    static constexpr float CLUSTER_BONUS_MULTIPLIER = 0.40f; 

    struct BadQuanta {
        float x, y; float vx = 0.0f, vy = 0.0f; unsigned long lastRandomMoveTime = 0;
        bool active = false; int health = 1; float radius = 2.0f;
        unsigned long spawnTime; unsigned long lifetimeMs = 45000;
        float waveAnimationTime = 0.0f;
    };
    static const int MAX_BAD_QUANTA = 3;
    BadQuanta _badQuantaPool[MAX_BAD_QUANTA];
    unsigned long _lastBadQuantaSpawnTime = 0;
    static const unsigned long BAD_QUANTA_BASE_MIN_SPAWN_INTERVAL_MS = 10000;
    static const unsigned long BAD_QUANTA_BASE_MAX_SPAWN_INTERVAL_MS = 18000;
    static constexpr float BAD_QUANTA_AUTO_ATTRACT_RADIUS = 35.0f;
    static constexpr float BAD_QUANTA_BASE_ATTRACT_STRENGTH = 0.25f;
    static constexpr float BAD_QUANTA_ENERGY_DRAIN = 15.0f; 
    static const unsigned long BAD_QUANTA_RANDOM_MOVE_INTERVAL_MS = 1500;
    static constexpr float BAD_QUANTA_BASE_WANDER_SPEED = 0.15f;

    int _nextClusterId = 0;
    static const unsigned long CLUSTER_STICK_TIME_MS = 1000;
    struct PotentialCluster { int q1_idx; int q2_idx; unsigned long overlapStartTime; };
    std::vector<PotentialCluster> _potentialClusters;

    bool _isAttracting = false;
    unsigned long _attractStartTime = 0;
    static const unsigned long ATTRACT_MAX_DURATION_MS = 1500;
    static constexpr float ATTRACT_RADIUS = 30.0f;
    static constexpr float ATTRACT_STRENGTH = 0.40f;

    std::unique_ptr<DialogBox> _dialogBox;
    bool _instructionsShown = false;

    std::unique_ptr<ParticleSystem> _particleSystem;
    std::unique_ptr<EffectsManager> _effectsManager; 

    // Scene-specific context pointer
    GameContext* _gameContext = nullptr;

    void onButton1Press();
    void onButton1Release();
    void onButton2Click();
    void onButton3Click();

    void spawnQuanta();
    void updateQuanta(unsigned long currentTime);
    void updateClustering(unsigned long currentTime);
    void checkQuantaCollection();
    void completeStage();
    void applyPulseToBackground();
    bool handleDialogKeyPress(uint8_t keyCode);

    void spawnBadQuanta();
    void updateBadQuanta(unsigned long currentTime);
    void drawBadQuanta(Renderer& renderer);
    void triggerScreenShake(); 
    void triggerAbsorbEffect(float x, float y, int size); 
};

#endif // AWAKENING_SPARK_SCENE_H