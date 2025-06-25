#include "_2CellularConglomerationScene.h"
#include "SceneManager.h"
#include "InputManager.h"
#include "Renderer.h"
#include "GameStats.h"
#include "SerialForwarder.h"
#include "Localization.h"
#include <cmath>
#include <vector>
#include <limits>
#include "../../Helper/EffectsManager.h"
#include "../../ParticleSystem.h"
#include "../../DebugUtils.h"
#include "GEM_u8g2.h"
#include "../../System/GameContext.h" 

extern String nextSceneName; 
extern bool replaceSceneStack;
extern bool sceneChangeRequested;

_2CellularConglomerationScene::_2CellularConglomerationScene()
{
    _noise.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
    _noise.SetFrequency(0.05f);
    _noise.SetCellularReturnType(FastNoiseLite::CellularReturnType_Distance2Div);
    _noise.SetSeed(random(0, 10000));
    _hazardNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    _hazardNoise.SetFrequency(0.08f);
    _hazardNoise.SetSeed(random(10000, 20000));
    debugPrint("SCENES", "_2CellularConglomerationScene constructor");
}

_2CellularConglomerationScene::~_2CellularConglomerationScene()
{
    debugPrint("SCENES", "_2CellularConglomerationScene destroyed");
}

void _2CellularConglomerationScene::init(GameContext &context)
{
    Scene::init();
    _gameContext = &context;
    debugPrint("SCENES", "_2CellularConglomerationScene::init");

    if (!_gameContext || !_gameContext->renderer || !_gameContext->display)
    {
        debugPrint("SCENES", "ERROR: _2CellularConglomerationScene::init - Critical context members are null!");
        return;
    }
    _dialogBox.reset(new DialogBox(*_gameContext->renderer));
    _particleSystem.reset(new ParticleSystem(*_gameContext->renderer, _gameContext->defaultFont)); 
    _effectsManager.reset(new EffectsManager(*_gameContext->renderer));
}

void _2CellularConglomerationScene::onEnter()
{
    debugPrint("SCENES", "_2CellularConglomerationScene::onEnter");
    _instructionsShown = false;
    _noiseOffsetX = random(0, 1000) / 10.0f;
    _noiseOffsetY = random(0, 1000) / 10.0f;
    setupBondingPhase();
    if (_dialogBox)
    {
        String instructions = String(loc(StringKey::PREQUEL_STAGE2_INST_BOND1)) + "\n" +
                              String(loc(StringKey::PREQUEL_STAGE2_INST_BOND2)) + "\n" +
                              String(loc(StringKey::PREQUEL_STAGE2_INST_BOND3)) + "\n" +
                              String(loc(StringKey::PREQUEL_STAGE2_INST_BOND4));
        _dialogBox->show(instructions.c_str());
    }
    else
    {
        _instructionsShown = true;
    }
}

void _2CellularConglomerationScene::onExit()
{
    debugPrint("SCENES", "_2CellularConglomerationScene::onExit");
    if (_dialogBox)
        _dialogBox->close();
    if (_gameContext && _gameContext->inputManager)
    {
        _gameContext->inputManager->unregisterAllListenersForScene(this);
    }
}

void _2CellularConglomerationScene::setupBondingPhase()
{
    debugPrint("SCENES", "Entering Bonding Phase");
    _currentPhase = Stage2Phase::BONDING_IDLE;
    _activeCellCount = 0;
    _bondedCellCountInLead = 0;
    _unbondedCellCount = 0;
    _clusterLeadCount = 0;
    _activeClusterId = -1;
    _nextClusterId = 0;
    _potentialClusters.clear();
    _bondingDifficultyLevel = 0;
    _enableHazards = false;
    _enableCellDrift = false;
    _currentMaxDriftSpeed = 0.0f;
    _currentHazardThreshold = 1.1f;
    _timingIndicatorSpeed = TIMING_SPEED_MIN;
    for (int i = 0; i < MAX_PROTO_CELLS; ++i)
    {
        _cellPool[i].active = false;
        _cellPool[i].clusterId = -1;
        _cellPool[i].isInHazard = false;
    }
    for (int i = 0; i < _clusterSizeGoal + 3; ++i)
    {
        spawnProtoCell();
    }
    updateCellCounts();
    if (_gameContext && _gameContext->inputManager)
    {
        _gameContext->inputManager->unregisterAllListenersForScene(this);
        _gameContext->inputManager->registerButtonListener(EDGE_Button::LEFT, EDGE_Event::CLICK, this, [this]()
                              { this->onButton1Click(); });
        _gameContext->inputManager->registerButtonListener(EDGE_Button::RIGHT, EDGE_Event::CLICK, this, [this]()
                              { this->onButton2Click(); });
        _gameContext->inputManager->registerButtonListener(EDGE_Button::OK, EDGE_Event::CLICK, this, [this]()
                              { this->onButton3Click(); });
        _gameContext->inputManager->registerButtonListener(EDGE_Button::OK, EDGE_Event::LONG_PRESS, this, [this]()
                              {
            if (handleDialogKeyPress(GEM_KEY_CANCEL)) { return; }
            if (_dialogBox && _dialogBox->isActive()) { _dialogBox->close(); _instructionsShown = true; } });
    }
}

void _2CellularConglomerationScene::setupSpecializationPhase()
{
    debugPrint("SCENES", "Entering Specialization Phase");
    _currentPhase = Stage2Phase::SPECIALIZATION;
    _selectedCellIndexInCluster = 0;
    _selectedSpecType = CellSpecialization::TYPE_A;
    _specTypeACount = 0;
    _specTypeBCount = 0;
    _specTypeCCount = 0;
    _enableHazards = false;
    _enableCellDrift = false;
    _activeClusterIndices.clear();
    for (int i = 0; i < MAX_PROTO_CELLS; ++i)
    {
        if (_cellPool[i].active && _cellPool[i].clusterId == _activeClusterId)
        {
            _activeClusterIndices.push_back(i);
            _cellPool[i].isSelectedForSpecialization = false;
        }
        else
        {
            _cellPool[i].active = false;
        }
    }
    if (!_activeClusterIndices.empty())
    {
        _cellPool[_activeClusterIndices[0]].isSelectedForSpecialization = true;
    }
    else
    {
        debugPrint("SCENES", "ERROR: Entered Specialization Phase with no active cluster members!");
        completeStage();
        return;
    }
    if (_gameContext && _gameContext->inputManager)
    {
        _gameContext->inputManager->unregisterAllListenersForScene(this);
        _gameContext->inputManager->registerButtonListener(EDGE_Button::LEFT, EDGE_Event::CLICK, this, [this]()
                              { this->onButton1Click(); });
        _gameContext->inputManager->registerButtonListener(EDGE_Button::RIGHT, EDGE_Event::CLICK, this, [this]()
                              { this->onButton2Click(); });
        _gameContext->inputManager->registerButtonListener(EDGE_Button::OK, EDGE_Event::CLICK, this, [this]()
                              { this->onButton3Click(); });
        _gameContext->inputManager->registerButtonListener(EDGE_Button::OK, EDGE_Event::LONG_PRESS, this, [this]()
                              {
            if (handleDialogKeyPress(GEM_KEY_CANCEL)) { return; }
            if (_dialogBox && _dialogBox->isActive()) { _dialogBox->close(); _instructionsShown = true; } });
    }
    if (_dialogBox)
    {
        char goalBuffer[30];
        snprintf(goalBuffer, sizeof(goalBuffer), "A:%d B:%d C:%d", _specGoalA, _specGoalB, _specGoalC);
        String instructions = String(loc(StringKey::PREQUEL_STAGE2_INST_SPEC1)) + " (" + String(goalBuffer) + ")" + "\n" +
                              String(loc(StringKey::PREQUEL_STAGE2_INST_SPEC2)) + "\n" +
                              String(loc(StringKey::PREQUEL_STAGE2_INST_SPEC3)) + "\n" +
                              String(loc(StringKey::PREQUEL_STAGE2_INST_SPEC4));
        _dialogBox->show(instructions.c_str());
        _instructionsShown = false;
    }
    else
    {
        _instructionsShown = true;
    }
}

void _2CellularConglomerationScene::spawnProtoCell()
{
    if (!_gameContext || !_gameContext->renderer || _activeCellCount >= MAX_PROTO_CELLS)
        return;
    Renderer &r = *_gameContext->renderer;
    for (int i = 0; i < MAX_PROTO_CELLS; ++i)
    {
        if (!_cellPool[i].active)
        {
            _cellPool[i].active = true;
            _cellPool[i].x = random(5, r.getWidth() - 5);
            _cellPool[i].y = random(5, r.getHeight() - 5);
            if (_enableCellDrift && _currentPhase != Stage2Phase::SPECIALIZATION)
            {
                _cellPool[i].vx = ((random(0, 101) / 100.0f) - 0.5f) * _currentMaxDriftSpeed;
                _cellPool[i].vy = ((random(0, 101) / 100.0f) - 0.5f) * _currentMaxDriftSpeed;
            }
            else
            {
                _cellPool[i].vx = 0.0f;
                _cellPool[i].vy = 0.0f;
            }
            _cellPool[i].clusterId = -1;
            _cellPool[i].isClusterLead = false;
            _cellPool[i].specialization = CellSpecialization::NONE;
            _cellPool[i].isSelectedForSpecialization = false;
            _cellPool[i].isInHazard = false;
            break;
        }
    }
    updateCellCounts();
}

void _2CellularConglomerationScene::updateCellCounts()
{
    _activeCellCount = 0;
    _unbondedCellCount = 0;
    _clusterLeadCount = 0;
    for (int i = 0; i < MAX_PROTO_CELLS; ++i)
    {
        if (_cellPool[i].active)
        {
            _activeCellCount++;
            if (_cellPool[i].clusterId == -1)
            {
                _unbondedCellCount++;
            }
            else if (_cellPool[i].isClusterLead)
            {
                _clusterLeadCount++;
            }
        }
    }
}

void _2CellularConglomerationScene::updateProtoCells(unsigned long deltaTime)
{
    if (!_gameContext || !_gameContext->renderer)
        return;
    Renderer &r = *_gameContext->renderer;
    float dt_factor = deltaTime / 33.0f;
    for (int i = 0; i < MAX_PROTO_CELLS; ++i)
    {
        if (_cellPool[i].active)
        {
            if (_currentPhase != Stage2Phase::SPECIALIZATION || _cellPool[i].clusterId != _activeClusterId)
            {
                bool currentlyInHazard = false;
                if ((_currentPhase == Stage2Phase::BONDING_IDLE || _currentPhase == Stage2Phase::BONDING_TIMING_ACTIVE) && _cellPool[i].clusterId == -1 && _enableHazards)
                {
                    float hazardNoiseVal = _hazardNoise.GetNoise(_cellPool[i].x, _cellPool[i].y);
                    currentlyInHazard = (hazardNoiseVal > _currentHazardThreshold);
                    if (currentlyInHazard && !_cellPool[i].isInHazard)
                    {
                        _cellPool[i].isInHazard = true;
                        _cellPool[i].hazardEnterTime = millis();
                        float repelBase = 1.5f + _currentMaxDriftSpeed * 5.0f;
                        _cellPool[i].vx = -_cellPool[i].vx * repelBase - ((_cellPool[i].x - r.getWidth() / 2.0f) * 0.02f);
                        _cellPool[i].vy = -_cellPool[i].vy * repelBase - ((_cellPool[i].y - r.getHeight() / 2.0f) * 0.02f);
                        if (_effectsManager)
                            _effectsManager->triggerScreenShake(1, 150);
                    }
                    else if (!currentlyInHazard && _cellPool[i].isInHazard)
                    {
                        _cellPool[i].isInHazard = false;
                    }
                    else if (currentlyInHazard && _cellPool[i].isInHazard)
                    {
                        float repelFactor = HAZARD_REPEL_STRENGTH;
                        _cellPool[i].vx -= ((_cellPool[i].x - r.getWidth() / 2.0f) * 0.002f) * repelFactor * dt_factor;
                        _cellPool[i].vy -= ((_cellPool[i].y - r.getHeight() / 2.0f) * 0.002f) * repelFactor * dt_factor;
                    }
                }
                else
                {
                    _cellPool[i].isInHazard = false;
                }
                if (_enableCellDrift || _cellPool[i].isInHazard || abs(_cellPool[i].vx) > 0.001f || abs(_cellPool[i].vy) > 0.001f)
                {
                    _cellPool[i].x += _cellPool[i].vx * dt_factor;
                    _cellPool[i].y += _cellPool[i].vy * dt_factor;
                }
                if (abs(_cellPool[i].vx) > 0.001f || abs(_cellPool[i].vy) > 0.001f)
                {
                    _cellPool[i].vx *= 0.97f;
                    _cellPool[i].vy *= 0.97f;
                }
                if (abs(_cellPool[i].vx) < 0.005f)
                    _cellPool[i].vx = 0.0f;
                if (abs(_cellPool[i].vy) < 0.005f)
                    _cellPool[i].vy = 0.0f;
                if (_cellPool[i].x < 2)
                {
                    _cellPool[i].x = 2;
                    _cellPool[i].vx *= -0.7f;
                }
                if (_cellPool[i].x > r.getWidth() - 3)
                {
                    _cellPool[i].x = r.getWidth() - 3;
                    _cellPool[i].vx *= -0.7f;
                }
                if (_cellPool[i].y < 2)
                {
                    _cellPool[i].y = 2;
                    _cellPool[i].vy *= -0.7f;
                }
                if (_cellPool[i].y > r.getHeight() - 3)
                {
                    _cellPool[i].y = r.getHeight() - 3;
                    _cellPool[i].vy *= -0.7f;
                }
            }
        }
    }
}

void _2CellularConglomerationScene::updateBondingTimingGame(unsigned long deltaTime)
{
    if (_currentPhase != Stage2Phase::BONDING_TIMING_ACTIVE)
        return;
    float dt_secs = deltaTime / 1000.0f;
    float moveDelta = _timingIndicatorSpeed * dt_secs;
    if (_timingIndicatorDir)
    {
        _timingIndicatorPos += moveDelta;
        if (_timingIndicatorPos >= 1.0f)
        {
            _timingIndicatorPos = 1.0f;
            _timingIndicatorDir = false;
        }
    }
    else
    {
        _timingIndicatorPos -= moveDelta;
        if (_timingIndicatorPos <= 0.0f)
        {
            _timingIndicatorPos = 0.0f;
            _timingIndicatorDir = true;
        }
    }
}

void _2CellularConglomerationScene::startBondingTimingGame(int idx1, int idx2)
{
    debugPrintf("SCENES", "Starting Bond Timing Game for cells %d and %d", idx1, idx2);
    _currentPhase = Stage2Phase::BONDING_TIMING_ACTIVE;
    _timingBondCellIdx1 = idx1;
    _timingBondCellIdx2 = idx2;
    _timingIndicatorPos = 0.0f;
    _timingIndicatorDir = true;
    _lastBondAttemptTime = millis();
}

void _2CellularConglomerationScene::attemptBond(bool success, int idx1, int idx2)
{
    _currentPhase = Stage2Phase::BONDING_IDLE;
    if (idx1 < 0 || idx1 >= MAX_PROTO_CELLS || idx2 < 0 || idx2 >= MAX_PROTO_CELLS ||
        !_cellPool[idx1].active || !_cellPool[idx2].active)
    {
        debugPrint("SCENES", "Bond attempt invalid - cells no longer active.");
        return;
    }
    if (success)
    {
        debugPrintf("SCENES", "Bond Success between %d and %d.", idx1, idx2);
        int cluster1Id = _cellPool[idx1].clusterId;
        int cluster2Id = _cellPool[idx2].clusterId;
        if (cluster1Id == -1 && cluster2Id == -1)
        {
            _cellPool[idx1].clusterId = _nextClusterId;
            _cellPool[idx1].isClusterLead = true;
            _cellPool[idx2].clusterId = _nextClusterId;
            _cellPool[idx2].isClusterLead = false;
            debugPrintf("SCENES", "New cluster %d formed.", _nextClusterId);
            _nextClusterId++;
        }
        else if (cluster1Id != -1 && cluster2Id == -1)
        {
            _cellPool[idx2].clusterId = cluster1Id;
            _cellPool[idx2].isClusterLead = false;
            debugPrintf("SCENES", "Cell %d joined cluster %d.", idx2, cluster1Id);
        }
        else if (cluster1Id == -1 && cluster2Id != -1)
        {
            _cellPool[idx1].clusterId = cluster2Id;
            _cellPool[idx1].isClusterLead = false;
            debugPrintf("SCENES", "Cell %d joined cluster %d.", idx1, cluster2Id);
        }
        else if (cluster1Id != cluster2Id)
        {
            int targetClusterId = cluster1Id;
            int sourceClusterId = cluster2Id;
            for (int k = 0; k < MAX_PROTO_CELLS; ++k)
            {
                if (_cellPool[k].active && _cellPool[k].clusterId == sourceClusterId && _cellPool[k].isClusterLead)
                {
                    _cellPool[k].isClusterLead = false;
                    break;
                }
            }
            debugPrintf("SCENES", "Merging cluster %d into %d.", sourceClusterId, targetClusterId);
            for (int k = 0; k < MAX_PROTO_CELLS; ++k)
            {
                if (_cellPool[k].active && _cellPool[k].clusterId == sourceClusterId)
                {
                    _cellPool[k].clusterId = targetClusterId;
                }
            }
        }
    }
    else
    {
        debugPrint("SCENES", "Bond Failed! (Timing missed)");
        if (_effectsManager)
            _effectsManager->triggerScreenShake(1, 100);
        float dx = _cellPool[idx1].x - _cellPool[idx2].x;
        float dy = _cellPool[idx1].y - _cellPool[idx2].y;
        float dist = sqrt(dx * dx + dy * dy);
        if (dist > 0.1f)
        {
            _cellPool[idx1].vx += (dx / dist) * 0.3f;
            _cellPool[idx1].vy += (dy / dist) * 0.3f;
            _cellPool[idx2].vx -= (dx / dist) * 0.3f;
            _cellPool[idx2].vy -= (dy / dist) * 0.3f;
        }
    }
    updateDifficultyAndCheckPhase();
}

void _2CellularConglomerationScene::updateDifficultyAndCheckPhase()
{
    updateCellCounts();
    int leadClusterId = -1;
    _bondedCellCountInLead = 0;
    for (int i = 0; i < MAX_PROTO_CELLS; ++i)
    {
        if (_cellPool[i].active && _cellPool[i].isClusterLead)
        {
            leadClusterId = _cellPool[i].clusterId;
            break;
        }
    }
    if (leadClusterId != -1)
    {
        for (int i = 0; i < MAX_PROTO_CELLS; ++i)
        {
            if (_cellPool[i].active && _cellPool[i].clusterId == leadClusterId)
            {
                _bondedCellCountInLead++;
            }
        }
    }
    else
    {
        _bondedCellCountInLead = 0;
        int maxCount = 0;
        if (_nextClusterId > 0)
        {
            std::vector<int> clusterCounts(_nextClusterId, 0);
            for (int i = 0; i < MAX_PROTO_CELLS; ++i)
            {
                if (_cellPool[i].active && _cellPool[i].clusterId != -1)
                {
                    if (_cellPool[i].clusterId >= 0 && _cellPool[i].clusterId < _nextClusterId)
                    {
                        clusterCounts[_cellPool[i].clusterId]++;
                        if (clusterCounts[_cellPool[i].clusterId] > maxCount)
                        {
                            maxCount = clusterCounts[_cellPool[i].clusterId];
                        }
                    }
                }
            }
        }
        _bondedCellCountInLead = maxCount > 1 ? maxCount : 0;
    }
    float bondingProgress = (_clusterSizeGoal > 0) ? (float)_bondedCellCountInLead / (float)_clusterSizeGoal : 0.0f;
    bondingProgress = std::min(1.0f, bondingProgress);
    int currentDifficultyLevel = (int)(bondingProgress * 5.0f);
    currentDifficultyLevel = std::min(4, std::max(0, currentDifficultyLevel));
    if (currentDifficultyLevel != _bondingDifficultyLevel)
    {
        _bondingDifficultyLevel = currentDifficultyLevel;
        debugPrintf("SCENES", "Bonding Difficulty Level updated to %d (%d / %d cells in main cluster)", _bondingDifficultyLevel, _bondedCellCountInLead, _clusterSizeGoal);
        bool shouldDrift = (_bondingDifficultyLevel >= 1);
        if (shouldDrift != _enableCellDrift)
        {
            _enableCellDrift = shouldDrift;
            if (_enableCellDrift)
            {
                _currentMaxDriftSpeed = 0.03f + 0.07f * ((float)_bondingDifficultyLevel / 4.0f);
                debugPrintf("SCENES", " -> Cell drift ENABLED (Max Speed: %.2f)", _currentMaxDriftSpeed);
                for (int i = 0; i < MAX_PROTO_CELLS; ++i)
                {
                    if (_cellPool[i].active && _cellPool[i].clusterId == -1)
                    {
                        _cellPool[i].vx = ((random(0, 101) / 100.0f) - 0.5f) * _currentMaxDriftSpeed;
                        _cellPool[i].vy = ((random(0, 101) / 100.0f) - 0.5f) * _currentMaxDriftSpeed;
                    }
                }
            }
            else
            {
                _currentMaxDriftSpeed = 0.0f;
                for (int i = 0; i < MAX_PROTO_CELLS; ++i)
                {
                    if (_cellPool[i].clusterId == -1)
                    {
                        _cellPool[i].vx = 0;
                        _cellPool[i].vy = 0;
                    }
                }
                debugPrint("SCENES", " -> Cell drift DISABLED");
            }
        }
        else if (_enableCellDrift)
        {
            _currentMaxDriftSpeed = 0.03f + 0.07f * ((float)_bondingDifficultyLevel / 4.0f);
        }
        bool shouldHaveHazards = (_bondingDifficultyLevel >= 2 && _bondingDifficultyLevel < 4);
        if (shouldHaveHazards != _enableHazards)
        {
            _enableHazards = shouldHaveHazards;
            if (_enableHazards)
            {
                _currentHazardThreshold = HAZARD_BASE_THRESHOLD - (0.1f * (_bondingDifficultyLevel - 1));
                _currentHazardThreshold = std::max(0.35f, _currentHazardThreshold);
                debugPrintf("SCENES", " -> Hazards ENABLED (Threshold: %.2f)", _currentHazardThreshold);
            }
            else
            {
                _currentHazardThreshold = 1.1f;
                debugPrintf("SCENES", " -> Hazards DISABLED (Level %d)", _bondingDifficultyLevel);
            }
        }
        else if (_enableHazards)
        {
            _currentHazardThreshold = HAZARD_BASE_THRESHOLD - (0.1f * (_bondingDifficultyLevel - 1));
            _currentHazardThreshold = std::max(0.35f, _currentHazardThreshold);
        }
    }
    if ((_currentPhase == Stage2Phase::BONDING_IDLE || _currentPhase == Stage2Phase::BONDING_TIMING_ACTIVE) &&
        _bondedCellCountInLead >= _clusterSizeGoal && leadClusterId != -1)
    {
        _activeClusterId = leadClusterId;
        setupSpecializationPhase();
    }
}

void _2CellularConglomerationScene::update(unsigned long deltaTime)
{
    unsigned long currentTime = millis();
    if (handleDialogKeyPress(GEM_KEY_OK))
    {
        if (_dialogBox && !_dialogBox->isActive() && !_instructionsShown)
        {
            _instructionsShown = true;
        }
        return;
    }
    if (!_instructionsShown)
        return;
    if (_effectsManager)
        _effectsManager->update(currentTime);
    _noiseOffsetX += 0.01f;
    updateProtoCells(deltaTime);
    updateCellCounts();
    if (_currentPhase == Stage2Phase::BONDING_IDLE)
    {
        if (_bondedCellCountInLead < _clusterSizeGoal && _activeCellCount < MAX_PROTO_CELLS &&
            (_unbondedCellCount + _clusterLeadCount < 2) && random(100) < 25)
        {
            debugPrintf("SCENES", "Low potential partners (%d unbonded, %d leads), spawning cell.", _unbondedCellCount, _clusterLeadCount);
            spawnProtoCell();
        }
        updateDifficultyAndCheckPhase();
    }
    else if (_currentPhase == Stage2Phase::BONDING_TIMING_ACTIVE)
    {
        updateBondingTimingGame(deltaTime);
        updateDifficultyAndCheckPhase();
    }
    else
    {
        if (_specTypeACount >= _specGoalA && _specTypeBCount >= _specGoalB && _specTypeCCount >= _specGoalC)
        {
            completeStage();
        }
    }
}

float _2CellularConglomerationScene::calculateOverallProgress() {
    float bondingProg = 0.0f; // Initialize
    if (_clusterSizeGoal > 0) { // Prevent division by zero
        bondingProg = static_cast<float>(_bondedCellCountInLead) / static_cast<float>(_clusterSizeGoal);
    }
    bondingProg = std::min(1.0f, bondingProg); // Now bondingProgress is definitely a float

    float specProg = 0.0f;
    int totalSpecGoal = _specGoalA + _specGoalB + _specGoalC;
    if (totalSpecGoal > 0) {
        specProg = static_cast<float>(_specTypeACount + _specTypeBCount + _specTypeCCount) / static_cast<float>(totalSpecGoal);
    }
    specProg = std::min(1.0f, specProg); // specProg is also float

    return (bondingProg * 0.6f) + (specProg * 0.4f);
}

void _2CellularConglomerationScene::draw(Renderer &renderer)
{
    if (!_gameContext || !_gameContext->display)
    {
        debugPrint("SCENES", "ERROR: _2CellularConglomerationScene::draw - Critical context member (display) null!");
        return;
    }
    U8G2 *u8g2 = _gameContext->display;
    int rOffsetX = renderer.getXOffset();
    int rOffsetY = renderer.getYOffset();
    if (_effectsManager)
        _effectsManager->applyEffectOffset(rOffsetX, rOffsetY);
    for (int y = 0; y < renderer.getHeight(); y += 4)
    {
        for (int x = 0; x < renderer.getWidth(); x += 4)
        {
            float noiseVal = _noise.GetNoise((float)x + _noiseOffsetX * 10, (float)y + _noiseOffsetY * 10);
            if (noiseVal > 0.5f)
                u8g2->drawPixel(rOffsetX + x, rOffsetY + y);
            else if (noiseVal < -0.5f)
                u8g2->drawPixel(rOffsetX + x + 1, rOffsetY + 1);
        }
    }
    if (_currentPhase == Stage2Phase::BONDING_IDLE || _currentPhase == Stage2Phase::BONDING_TIMING_ACTIVE)
    {
        if (_enableHazards)
        {
            drawHazards(renderer);
        }
        drawBondingPhase(renderer);
    }
    else
    {
        drawSpecializationPhase(renderer);
    }
    if (_dialogBox && _dialogBox->isActive())
    {
        _dialogBox->draw();
    }
}

void _2CellularConglomerationScene::drawHazards(Renderer &renderer)
{
    if (!_gameContext || !_gameContext->display)
        return;
    U8G2 *u8g2 = _gameContext->display;
    int rOffsetX = renderer.getXOffset();
    int rOffsetY = renderer.getYOffset();
    if (_effectsManager)
        _effectsManager->applyEffectOffset(rOffsetX, rOffsetY);
    u8g2->setDrawColor(1);
    for (int y = 0; y < renderer.getHeight(); y += 3)
    {
        for (int x = 0; x < renderer.getWidth(); x += 3)
        {
            float hazardVal = _hazardNoise.GetNoise((float)x, (float)y);
            if (hazardVal > _currentHazardThreshold)
            {
                u8g2->drawBox(rOffsetX + x, rOffsetY + y, 2, 2);
                if (hazardVal > _currentHazardThreshold + 0.1f)
                {
                    u8g2->drawPixel(rOffsetX + x + 1, rOffsetY + y);
                    u8g2->drawPixel(rOffsetX + x, rOffsetY + y + 1);
                }
            }
        }
    }
}

void _2CellularConglomerationScene::drawBondingPhase(Renderer &renderer)
{
    if (!_gameContext || !_gameContext->display)
        return;
    U8G2 *u8g2 = _gameContext->display;
    int rOffsetX = renderer.getXOffset();
    int rOffsetY = renderer.getYOffset();
    if (_effectsManager)
        _effectsManager->applyEffectOffset(rOffsetX, rOffsetY);
    for (int i = 0; i < MAX_PROTO_CELLS; ++i)
    {
        if (_cellPool[i].active)
        {
            int drawX = rOffsetX + (int)_cellPool[i].x;
            int drawY = rOffsetY + (int)_cellPool[i].y;
            if (_cellPool[i].clusterId != -1)
            {
                u8g2->drawDisc(drawX, drawY, 3);
                if (_cellPool[i].isClusterLead)
                {
                    u8g2->drawCircle(drawX, drawY, 4);
                }
            }
            else
            {
                u8g2->drawCircle(drawX, drawY, 2);
            }
        }
    }
    int barWidth = renderer.getWidth() / 2;
    int barHeight = 5;
    int barX = renderer.getXOffset() + (renderer.getWidth() - barWidth) / 2;
    int barY = renderer.getYOffset() + renderer.getHeight() - barHeight - 3;
    int fillWidth = 0;
    if (_clusterSizeGoal > 0)
    {
        fillWidth = (_bondedCellCountInLead * (barWidth - 2)) / _clusterSizeGoal;
        fillWidth = std::max(0, std::min(fillWidth, barWidth - 2));
    }
    u8g2->drawFrame(barX, barY, barWidth, barHeight);
    if (fillWidth > 0)
    {
        u8g2->drawBox(barX + 1, barY + 1, fillWidth, barHeight - 2);
    }
    if (_currentPhase == Stage2Phase::BONDING_TIMING_ACTIVE)
    {
        drawBondingTimingGame(renderer);
    }
}

void _2CellularConglomerationScene::drawBondingTimingGame(Renderer &renderer)
{
    if (!_gameContext || !_gameContext->display)
        return;
    U8G2 *u8g2 = _gameContext->display;
    int barX = renderer.getXOffset() + (renderer.getWidth() - TIMING_BAR_WIDTH) / 2;
    int barY = renderer.getYOffset() + (renderer.getHeight() / 2) - (TIMING_BAR_HEIGHT / 2) - 10;
    u8g2->setDrawColor(0);
    u8g2->drawBox(barX - 1, barY - 1, TIMING_BAR_WIDTH + 2, TIMING_BAR_HEIGHT + 2);
    u8g2->setDrawColor(1);
    u8g2->drawFrame(barX, barY, TIMING_BAR_WIDTH, TIMING_BAR_HEIGHT);
    int targetX = barX + (int)(_timingTargetZoneStart * TIMING_BAR_WIDTH);
    int targetW = (int)((_timingTargetZoneEnd - _timingTargetZoneStart) * TIMING_BAR_WIDTH);
    targetW = std::max(1, targetW);
    u8g2->drawBox(targetX, barY + 1, targetW, TIMING_BAR_HEIGHT - 2);
    int indicatorWidth = 2;
    int indicatorX = barX + (int)(_timingIndicatorPos * (TIMING_BAR_WIDTH - indicatorWidth));
    indicatorX = std::max(barX, std::min(barX + TIMING_BAR_WIDTH - indicatorWidth, indicatorX));
    bool overlaps = (indicatorX < targetX + targetW) && (indicatorX + indicatorWidth > targetX);
    if (overlaps)
    {
        u8g2->setDrawColor(0);
    }
    else
    {
        u8g2->setDrawColor(1);
    }
    u8g2->drawBox(indicatorX, barY + 1, indicatorWidth, TIMING_BAR_HEIGHT - 2);
    u8g2->setDrawColor(1);
}

void _2CellularConglomerationScene::drawSpecializationPhase(Renderer &renderer)
{
    if (!_gameContext || !_gameContext->display || !_gameContext->renderer)
        return;
    U8G2 *u8g2 = _gameContext->display;
    Renderer &r_ref = *_gameContext->renderer;
    int rOffsetX = renderer.getXOffset();
    int rOffsetY = renderer.getYOffset();
    if (_effectsManager)
        _effectsManager->applyEffectOffset(rOffsetX, rOffsetY);
    int screenCenterX = renderer.getXOffset() + r_ref.getWidth() / 2;
    int screenCenterY = renderer.getYOffset() + r_ref.getHeight() / 2;
    int clusterRadius = 15;
    for (size_t i = 0; i < _activeClusterIndices.size(); ++i)
    {
        int cellPoolIndex = _activeClusterIndices[i];
        float angle = (2.0f * PI / _activeClusterIndices.size()) * i;
        int cellX = rOffsetX + screenCenterX + (int)(cos(angle) * clusterRadius);
        int cellY = rOffsetY + screenCenterY + (int)(sin(angle) * clusterRadius);
        switch (_cellPool[cellPoolIndex].specialization)
        {
        case CellSpecialization::TYPE_A:
            u8g2->drawCircle(cellX, cellY, 3);
            break;
        case CellSpecialization::TYPE_B:
            u8g2->drawDisc(cellX, cellY, 3);
            break;
        case CellSpecialization::TYPE_C:
            u8g2->drawFrame(cellX - 2, cellY - 2, 5, 5);
            break;
        case CellSpecialization::NONE:
        default:
            u8g2->drawCircle(cellX, cellY, 2);
            break;
        }
        if (i == _selectedCellIndexInCluster)
        {
            u8g2->drawFrame(cellX - 5, cellY - 5, 10, 10);
        }
    }
    int indicatorX = renderer.getXOffset() + 3;
    int indicatorY = renderer.getYOffset() + 3;
    switch (_selectedSpecType)
    {
    case CellSpecialization::TYPE_A:
        u8g2->drawCircle(indicatorX + 1, indicatorY + 1, 1);
        break;
    case CellSpecialization::TYPE_B:
        u8g2->drawDisc(indicatorX + 1, indicatorY + 1, 1);
        break;
    case CellSpecialization::TYPE_C:
        u8g2->drawFrame(indicatorX, indicatorY, 3, 3);
        break;
    default:
        break;
    }
    char buffer[20];
    snprintf(buffer, sizeof(buffer), "A:%d/%d B:%d/%d C:%d/%d", _specTypeACount, _specGoalA, _specTypeBCount, _specGoalB, _specTypeCCount, _specGoalC);
    u8g2->setFont(u8g2_font_4x6_tf);
    u8g2->drawStr(renderer.getXOffset() + 2, renderer.getYOffset() + renderer.getHeight() - 7, buffer);
    u8g2->setFont(u8g2_font_5x7_tf);
}

void _2CellularConglomerationScene::onButton1Click()
{
    if (handleDialogKeyPress(GEM_KEY_UP))
    {
        return;
    }
    if (!_instructionsShown)
        return;
    if (_currentPhase == Stage2Phase::BONDING_IDLE)
    {
        debugPrint("SCENES", "Stage2: Nudge Left");
        for (int i = 0; i < MAX_PROTO_CELLS; ++i)
        {
            if (_cellPool[i].active && _cellPool[i].clusterId == -1)
            {
                _cellPool[i].vx -= NUDGE_STRENGTH;
            }
        }
    }
    else if (_currentPhase == Stage2Phase::SPECIALIZATION)
    {
        _selectedSpecType = static_cast<CellSpecialization>(((int)_selectedSpecType + 1) % 4);
        if (_selectedSpecType == CellSpecialization::NONE)
            _selectedSpecType = CellSpecialization::TYPE_A;
        debugPrintf("SCENES", "Stage2: Spec Type selected: %d", (int)_selectedSpecType);
    }
}

void _2CellularConglomerationScene::onButton2Click()
{
    if (handleDialogKeyPress(GEM_KEY_DOWN))
    {
        return;
    }
    if (!_instructionsShown)
        return;
    if (_currentPhase == Stage2Phase::BONDING_IDLE)
    {
        debugPrint("SCENES", "Stage2: Nudge Right");
        for (int i = 0; i < MAX_PROTO_CELLS; ++i)
        {
            if (_cellPool[i].active && _cellPool[i].clusterId == -1)
            {
                _cellPool[i].vx += NUDGE_STRENGTH;
            }
        }
    }
    else if (_currentPhase == Stage2Phase::SPECIALIZATION)
    {
        if (!_activeClusterIndices.empty())
        {
            _cellPool[_activeClusterIndices[_selectedCellIndexInCluster]].isSelectedForSpecialization = false;
            _selectedCellIndexInCluster = (_selectedCellIndexInCluster + 1) % _activeClusterIndices.size();
            _cellPool[_activeClusterIndices[_selectedCellIndexInCluster]].isSelectedForSpecialization = true;
            debugPrintf("SCENES", "Stage2: Cell %d selected in cluster.", _activeClusterIndices[_selectedCellIndexInCluster]);
        }
    }
}

void _2CellularConglomerationScene::onButton3Click()
{
    if (handleDialogKeyPress(GEM_KEY_OK))
    {
        if (_dialogBox && !_dialogBox->isActive() && !_instructionsShown)
        {
            _instructionsShown = true;
        }
        return;
    }
    if (!_instructionsShown)
        return;
    if (_currentPhase == Stage2Phase::BONDING_IDLE)
    {
        if (millis() - _lastBondAttemptTime < BOND_COOLDOWN_MS)
            return;
        int target1 = -1;
        int target2 = -1;
        bool targetFound = false;
        float minDistSq = BOND_DISTANCE * BOND_DISTANCE;
        int leadClusterId = -1;
        for (int i = 0; i < MAX_PROTO_CELLS; ++i)
            if (_cellPool[i].active && _cellPool[i].isClusterLead)
            {
                leadClusterId = _cellPool[i].clusterId;
                break;
            }
        if (leadClusterId != -1)
        {
            float closestDistSqToCluster = std::numeric_limits<float>::max();
            for (int i = 0; i < MAX_PROTO_CELLS; ++i)
            {
                if (!_cellPool[i].active || _cellPool[i].clusterId != leadClusterId)
                    continue;
                for (int j = 0; j < MAX_PROTO_CELLS; ++j)
                {
                    if (!_cellPool[j].active || _cellPool[j].clusterId != -1)
                        continue;
                    float dx = _cellPool[i].x - _cellPool[j].x;
                    float dy = _cellPool[i].y - _cellPool[j].y;
                    float distSq = dx * dx + dy * dy;
                    if (distSq < minDistSq && distSq < closestDistSqToCluster)
                    {
                        closestDistSqToCluster = distSq;
                        target1 = i;
                        target2 = j;
                        targetFound = true;
                    }
                }
            }
        }
        if (!targetFound)
        {
            float closestLeadDistSq = std::numeric_limits<float>::max();
            for (int i = 0; i < MAX_PROTO_CELLS; ++i)
            {
                if (!_cellPool[i].active || !_cellPool[i].isClusterLead)
                    continue;
                for (int j = i + 1; j < MAX_PROTO_CELLS; ++j)
                {
                    if (!_cellPool[j].active || !_cellPool[j].isClusterLead)
                        continue;
                    float dx = _cellPool[i].x - _cellPool[j].x;
                    float dy = _cellPool[i].y - _cellPool[j].y;
                    float distSq = dx * dx + dy * dy;
                    if (distSq < minDistSq && distSq < closestLeadDistSq)
                    {
                        closestLeadDistSq = distSq;
                        target1 = i;
                        target2 = j;
                        targetFound = true;
                    }
                }
            }
        }
        if (!targetFound)
        {
            float closestUnbondedDistSq = std::numeric_limits<float>::max();
            for (int i = 0; i < MAX_PROTO_CELLS; ++i)
            {
                if (!_cellPool[i].active || _cellPool[i].clusterId != -1)
                    continue;
                for (int j = i + 1; j < MAX_PROTO_CELLS; ++j)
                {
                    if (!_cellPool[j].active || _cellPool[j].clusterId != -1)
                        continue;
                    float dx = _cellPool[i].x - _cellPool[j].x;
                    float dy = _cellPool[i].y - _cellPool[j].y;
                    float distSq = dx * dx + dy * dy;
                    if (distSq < minDistSq && distSq < closestUnbondedDistSq)
                    {
                        closestUnbondedDistSq = distSq;
                        target1 = i;
                        target2 = j;
                        targetFound = true;
                    }
                }
            }
        }
        if (targetFound)
        {
            startBondingTimingGame(target1, target2);
        }
        else
        {
            debugPrint("SCENES", "Stage2: OK Press - No bond targets found, performing Recall Pulse.");
            recallUnbondedCells();
        }
    }
    else if (_currentPhase == Stage2Phase::BONDING_TIMING_ACTIVE)
    {
        bool success = (_timingIndicatorPos >= _timingTargetZoneStart && _timingIndicatorPos <= _timingTargetZoneEnd);
        attemptBond(success, _timingBondCellIdx1, _timingBondCellIdx2);
        _timingBondCellIdx1 = -1;
        _timingBondCellIdx2 = -1;
    }
    else
    {
        if (!_activeClusterIndices.empty() && _selectedCellIndexInCluster < _activeClusterIndices.size())
        {
            int targetCellPoolIndex = _activeClusterIndices[_selectedCellIndexInCluster];
            if (_cellPool[targetCellPoolIndex].specialization == CellSpecialization::NONE)
            {
                _cellPool[targetCellPoolIndex].specialization = _selectedSpecType;
                debugPrintf("SCENES", "Stage2: Applied Spec %d to cell index %d.", (int)_selectedSpecType, targetCellPoolIndex);
                if (_selectedSpecType == CellSpecialization::TYPE_A)
                    _specTypeACount++;
                else if (_selectedSpecType == CellSpecialization::TYPE_B)
                    _specTypeBCount++;
                else if (_selectedSpecType == CellSpecialization::TYPE_C)
                    _specTypeCCount++;
                if (_specTypeACount >= _specGoalA && _specTypeBCount >= _specGoalB && _specTypeCCount >= _specGoalC)
                {
                    completeStage();
                    return;
                }
            }
            else
            {
                debugPrintf("SCENES", "Stage2: Cell %d already specialized.", targetCellPoolIndex);
            }
        }
    }
}

void _2CellularConglomerationScene::recallUnbondedCells()
{
    if (!_gameContext || !_gameContext->renderer)
        return;
    Renderer &r = *_gameContext->renderer;
    float centerX = r.getWidth() / 2.0f;
    float centerY = r.getHeight() / 2.0f;
    debugPrint("SCENES", "Applying Recall Pulse to unbonded cells.");
    for (int i = 0; i < MAX_PROTO_CELLS; ++i)
    {
        if (_cellPool[i].active && _cellPool[i].clusterId == -1)
        {
            float dx = centerX - _cellPool[i].x;
            float dy = centerY - _cellPool[i].y;
            float dist = sqrt(dx * dx + dy * dy);
            if (dist > 0.1f)
            {
                _cellPool[i].vx += (dx / dist) * RECALL_PULSE_STRENGTH * (1.0f + dist * 0.01f);
                _cellPool[i].vy += (dy / dist) * RECALL_PULSE_STRENGTH * (1.0f + dist * 0.01f);
            }
        }
    }
    if (_effectsManager)
        _effectsManager->triggerScreenShake(1, 100);
}

void _2CellularConglomerationScene::completeStage()
{
    debugPrint("SCENES", "_2CellularConglomerationScene: Stage 2 Complete!");
    if (_gameContext && _gameContext->gameStats && _gameContext->sceneManager)
    {
        _gameContext->gameStats->setCompletedPrequelStage(PrequelStage::STAGE_2_CONGLOMERATE_COMPLETE);
        _gameContext->gameStats->addPoints(100);
        _gameContext->gameStats->save();
        _gameContext->sceneManager->requestSetCurrentScene("PREQUEL_STAGE_3");
    }
    else
    {
        debugPrint("SCENES", "Error: Critical context members null in completeStage.");
    }
}

void _2CellularConglomerationScene::triggerAbsorbEffect(float x, float y, int size)
{
    if (_particleSystem)
    {
        _particleSystem->spawnAbsorbEffect(x, y, x, y, 6, size);
    }
}

bool _2CellularConglomerationScene::handleDialogKeyPress(uint8_t keyCode) {
    if (_dialogBox && _dialogBox->isActive()) {
        if (keyCode == GEM_KEY_OK) {
            _dialogBox->close();
            _instructionsShown = true;
            return true;
        }
        return true; 
    }
    return false;
}