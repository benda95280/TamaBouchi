#ifndef CELLULAR_CONGLOMERATION_SCENE_H
#define CELLULAR_CONGLOMERATION_SCENE_H

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

enum class CellSpecialization : uint8_t {
    NONE,
    TYPE_A,
    TYPE_B,
    TYPE_C
};

enum class Stage2Phase {
    BONDING_IDLE,
    BONDING_TIMING_ACTIVE,
    SPECIALIZATION
};

class _2CellularConglomerationScene : public Scene {
private:
    struct ProtoCell {
        float x, y; float vx, vy; bool active = false; int clusterId = -1;
        bool isClusterLead = false; CellSpecialization specialization = CellSpecialization::NONE;
        bool isSelectedForSpecialization = false; bool isInHazard = false;
        unsigned long hazardEnterTime = 0;
    };

    static const int MAX_PROTO_CELLS = 10;
    ProtoCell _cellPool[MAX_PROTO_CELLS];
    int _activeCellCount = 0; int _unbondedCellCount = 0; int _clusterLeadCount = 0;
    int _activeClusterId = -1; int _clusterSizeGoal = 5; int _bondedCellCountInLead = 0;

    unsigned long _lastBondAttemptTime = 0;
    static const unsigned long BOND_COOLDOWN_MS = 300;
    static constexpr float BOND_DISTANCE = 10.0f;
    static constexpr float NUDGE_STRENGTH = 0.6f;
    static constexpr float RECALL_PULSE_STRENGTH = 0.20f;

    float _timingIndicatorPos = 0.0f; float _timingIndicatorSpeed = TIMING_SPEED_MIN;
    static constexpr float TIMING_SPEED_MIN = 1.8f;
    bool _timingIndicatorDir = true; float _timingTargetZoneStart = 0.35f;
    float _timingTargetZoneEnd = 0.65f; int _timingBondCellIdx1 = -1; int _timingBondCellIdx2 = -1;
    static const int TIMING_BAR_WIDTH = 40; static const int TIMING_BAR_HEIGHT = 5;

    CellSpecialization _selectedSpecType = CellSpecialization::TYPE_A;
    int _selectedCellIndexInCluster = 0; std::vector<int> _activeClusterIndices;
    int _specTypeACount = 0; int _specTypeBCount = 0; int _specTypeCCount = 0;
    int _specGoalA = 2; int _specGoalB = 2; int _specGoalC = 1;

    Stage2Phase _currentPhase = Stage2Phase::BONDING_IDLE;
    FastNoiseLite _noise; FastNoiseLite _hazardNoise;
    float _noiseOffsetX = 0.0f; float _noiseOffsetY = 0.0f;
    static constexpr float HAZARD_BASE_THRESHOLD = 0.65f;
    float _currentHazardThreshold = 1.1f;
    static constexpr float HAZARD_REPEL_STRENGTH = 0.15f;

    int _bondingDifficultyLevel = 0; bool _enableHazards = false;
    bool _enableCellDrift = false; float _currentMaxDriftSpeed = 0.0f;
    static constexpr float MAX_DRIFT_SPEED_AT_100_PERCENT = 0.10f;

    std::unique_ptr<DialogBox> _dialogBox; bool _instructionsShown = false;

    int _nextClusterId = 0; static const unsigned long CLUSTER_STICK_TIME_MS = 1000;
    struct PotentialCluster { int q1_idx; int q2_idx; unsigned long overlapStartTime; };
    std::vector<PotentialCluster> _potentialClusters;

    std::unique_ptr<ParticleSystem> _particleSystem;
    std::unique_ptr<EffectsManager> _effectsManager; 

    // Scene-specific context pointer
    GameContext* _gameContext = nullptr;

    void onButton1Click(); void onButton2Click(); void onButton3Click();

    void setupBondingPhase(); void setupSpecializationPhase();
    void spawnProtoCell(); void updateProtoCells(unsigned long deltaTime);
    void updateBondingTimingGame(unsigned long deltaTime);
    void startBondingTimingGame(int idx1, int idx2);
    void attemptBond(bool success, int idx1, int idx2);
    void updateDifficultyAndCheckPhase(); float calculateOverallProgress();
    void drawBondingPhase(Renderer& renderer); void drawSpecializationPhase(Renderer& renderer);
    void drawHazards(Renderer& renderer); void drawBondingTimingGame(Renderer& renderer);
    void completeStage(); void triggerAbsorbEffect(float x, float y, int size); 
    void updateCellCounts(); void recallUnbondedCells();
    bool handleDialogKeyPress(uint8_t keyCode);

public:
    _2CellularConglomerationScene();
    ~_2CellularConglomerationScene() override;

    SceneType getSceneType() const override { return SceneType::PREQUEL_STAGE_2; }

    // This is a scene-specific init, not an override
    void init(GameContext& context); 
    void onEnter() override;
    void onExit() override;
    void update(unsigned long deltaTime) override;
    void draw(Renderer& renderer) override; // Renderer& stays for drawing

    DialogBox* getDialogBox() override { return _dialogBox.get(); }
};

#endif // CELLULAR_CONGLOMERATION_SCENE_H