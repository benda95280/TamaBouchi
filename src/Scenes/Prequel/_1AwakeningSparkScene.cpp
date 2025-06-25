#include "_1AwakeningSparkScene.h"
#include "SceneManager.h"
#include "InputManager.h" // For registerInputCallback
#include "Renderer.h"
#include "GameStats.h"
#include "SerialForwarder.h"
#include "Localization.h"
#include <cmath> 
#include "../../Helper/EffectsManager.h" 
#include "../../DebugUtils.h"
#include "GEM_u8g2.h" 
#include "../../System/GameContext.h"

// --- Remove extern GameStats*, SerialForwarder* ---
extern String nextSceneName; 
extern bool replaceSceneStack;
extern bool sceneChangeRequested;

_1AwakeningSparkScene::_1AwakeningSparkScene() {
    _noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2S);
    _noise.SetFrequency(0.035f);
    _noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    _noise.SetFractalOctaves(2);
    _noise.SetFractalLacunarity(2.0f);
    _noise.SetFractalGain(0.5f);
    _pulseNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    _pulseNoise.SetFrequency(0.2f);
    _pulseNoise.SetSeed(random(0,10000));
    _badQuantaWaveNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    _badQuantaWaveNoise.SetFrequency(0.3f);
    _badQuantaWaveNoise.SetSeed(random(0,5000));
    debugPrint("SCENES", "_1AwakeningSparkScene constructor");
}

_1AwakeningSparkScene::~_1AwakeningSparkScene() {
    debugPrint("SCENES", "_1AwakeningSparkScene destroyed");
}

void _1AwakeningSparkScene::init(GameContext& context) { 
    Scene::init(); 
    _gameContext = &context;
    debugPrint("SCENES", "_1AwakeningSparkScene::init");

    if (!_gameContext || !_gameContext->renderer || !_gameContext->display) {
        debugPrint("SCENES", "ERROR: _1AwakeningSparkScene::init - Critical context members are null!");
        return;
    }
    _dialogBox.reset(new DialogBox(*_gameContext->renderer));
    _particleSystem.reset(new ParticleSystem(*_gameContext->renderer, _gameContext->defaultFont)); 
    _effectsManager.reset(new EffectsManager(*_gameContext->renderer)); 

    _gameContext->inputManager->registerButtonListener(EDGE_Button::LEFT, EDGE_Event::PRESS, this, [this](){ this->onButton1Press(); });
    _gameContext->inputManager->registerButtonListener(EDGE_Button::LEFT, EDGE_Event::RELEASE, this, [this](){ this->onButton1Release(); });
    _gameContext->inputManager->registerButtonListener(EDGE_Button::RIGHT, EDGE_Event::CLICK, this, [this](){ this->onButton2Click(); });
    _gameContext->inputManager->registerButtonListener(EDGE_Button::OK, EDGE_Event::CLICK, this, [this](){ this->onButton3Click(); });
    _gameContext->inputManager->registerButtonListener(EDGE_Button::OK, EDGE_Event::LONG_PRESS, this, [this]() { 
        if (handleDialogKeyPress(GEM_KEY_CANCEL)) { return; }
        if (_dialogBox && _dialogBox->isActive()) { _dialogBox->close(); _instructionsShown = true; }
    });
}

void _1AwakeningSparkScene::onEnter() {
    debugPrint("SCENES", "_1AwakeningSparkScene::onEnter");
    _startTime = millis();
    _instructionsShown = false;
    _sparkIsInNebula = false;
    _currentDifficultyLevel = 0;

    if (_gameContext && _gameContext->renderer) { // Use context
        Renderer& r = *_gameContext->renderer;
        _sparkX = r.getWidth() / 2.0f;
        _sparkY = r.getHeight() / 2.0f;
    } else {
        _sparkX = 128 / 2.0f; _sparkY = 64 / 2.0f;
        debugPrint("SCENES", "Warning: _1AwakeningSparkScene::onEnter - GameContext or Renderer is null.");
    }
    _sparkEnergy = 0.0f; _isAttracting = false; _isPulsing = false;
    _sparkCoreAnimCounter = 0; _anyQuantaUnderSpark = false;
    _noiseOffsetX = random(0, 1000) / 10.0f; _noiseOffsetY = random(0, 1000) / 10.0f;

    for(int i=0; i < MAX_QUANTA; ++i) { _quantaPool[i].active = false; _quantaPool[i].clusterId = -1; _quantaPool[i].isClusterLead = false; }
    _potentialClusters.clear(); _nextClusterId = 0;
    _lastQuantaSpawnTime = millis(); for(int i=0; i<5; ++i) spawnQuanta();
    for(int i=0; i < MAX_BAD_QUANTA; ++i) { _badQuantaPool[i].active = false; }
    _lastBadQuantaSpawnTime = millis() + BAD_QUANTA_BASE_MIN_SPAWN_INTERVAL_MS;
    if (_particleSystem) _particleSystem->reset();
    if (_dialogBox) {
        String fullInstruction = String(loc(StringKey::PREQUEL_STAGE1_INST_LINE1)) + "\n" +
                                 String(loc(StringKey::PREQUEL_STAGE1_INST_LINE2)) + "\n" +
                                 String(loc(StringKey::PREQUEL_STAGE1_INST_LINE3)) + "\n" +
                                 String(loc(StringKey::PREQUEL_STAGE1_INST_LINE4));
        _dialogBox->show(fullInstruction.c_str());
    } else { _instructionsShown = true; }
}

void _1AwakeningSparkScene::onExit() {
    debugPrint("SCENES", "_1AwakeningSparkScene::onExit");
    if (_dialogBox) _dialogBox->close();
    if (_particleSystem) _particleSystem->reset();
    if (_gameContext && _gameContext->inputManager) {
        _gameContext->inputManager->unregisterAllListenersForScene(this);
    }
}

void _1AwakeningSparkScene::spawnQuanta() {
    if (!_gameContext || !_gameContext->renderer) return; // Use context
    Renderer& r = *_gameContext->renderer;
    for (int i = 0; i < MAX_QUANTA; ++i) {
        if (!_quantaPool[i].active) {
            _quantaPool[i].active = true;
            _quantaPool[i].x = random(0, r.getWidth());
            _quantaPool[i].y = random(0, r.getHeight());
            _quantaPool[i].size = random(1, 3);
            if (_quantaPool[i].size == 1) _quantaPool[i].actualRadius = 0.30f;
            else _quantaPool[i].actualRadius = 0.60f;
            float dx_check = _sparkX - _quantaPool[i].x;
            float dy_check = _sparkY - _quantaPool[i].y;
            if (sqrt(dx_check*dx_check + dy_check*dy_check) < _sparkSize + _quantaPool[i].actualRadius + 5.0f) {
                _quantaPool[i].active = false; continue;
            }
            _quantaPool[i].spawnTime = millis();
            _quantaPool[i].lastPulseTime = millis() + random(0, (int)(QUANTA_PULSE_INTERVAL_MS / 2.0f));
            _quantaPool[i].pulseIncreasing = true; _quantaPool[i].currentPulseSize = 0.0f;
            _quantaPool[i].isInvertedDraw = false; _quantaPool[i].clusterId = -1; _quantaPool[i].isClusterLead = false;
            _quantaPool[i].vx = 0; _quantaPool[i].vy = 0;
            _quantaPool[i].nextMoveTime = millis() + random(QUANTA_MIN_MOVE_INTERVAL_MS, QUANTA_MAX_MOVE_INTERVAL_MS + 1);
            break;
        }
    }
}

void _1AwakeningSparkScene::updateQuanta(unsigned long currentTime) {
    _anyQuantaUnderSpark = false;
    for (int i = 0; i < MAX_QUANTA; ++i) {
        if (_quantaPool[i].active) {
            if (currentTime - _quantaPool[i].spawnTime > QUANTA_LIFETIME_MS) { _quantaPool[i].active = false; continue; }
            if (currentTime >= _quantaPool[i].nextMoveTime) {
                float angle = random(0, 360) * PI / 180.0f;
                float speed = QUANTA_MAX_SPEED * (random(50, 101) / 100.0f);
                _quantaPool[i].vx = cos(angle) * speed; _quantaPool[i].vy = sin(angle) * speed;
                _quantaPool[i].nextMoveTime = currentTime + random(QUANTA_MIN_MOVE_INTERVAL_MS, QUANTA_MAX_MOVE_INTERVAL_MS + 1);
            }
            _quantaPool[i].x += _quantaPool[i].vx; _quantaPool[i].y += _quantaPool[i].vy;
            if(_gameContext && _gameContext->renderer){ // Use context
                Renderer& r = *_gameContext->renderer;
                if(_quantaPool[i].x < 0) {_quantaPool[i].x = 0; _quantaPool[i].vx *= -0.5;}
                if(_quantaPool[i].x >= r.getWidth()) {_quantaPool[i].x = r.getWidth()-1; _quantaPool[i].vx *= -0.5;}
                if(_quantaPool[i].y < 0) {_quantaPool[i].y = 0; _quantaPool[i].vy *= -0.5;}
                if(_quantaPool[i].y >= r.getHeight()) {_quantaPool[i].y = r.getHeight()-1; _quantaPool[i].vy *= -0.5;}
            }
            if (currentTime - _quantaPool[i].lastPulseTime > QUANTA_PULSE_INTERVAL_MS / 2) {
                _quantaPool[i].lastPulseTime = currentTime; _quantaPool[i].pulseIncreasing = !_quantaPool[i].pulseIncreasing;
            }
            float pulseCycleProgress = (float)(currentTime - _quantaPool[i].lastPulseTime) / (QUANTA_PULSE_INTERVAL_MS/2.0f);
            pulseCycleProgress = std::max(0.0f, std::min(1.0f, pulseCycleProgress));
            if (_quantaPool[i].pulseIncreasing) { _quantaPool[i].currentPulseSize = QUANTA_PULSE_MAX_ADDITIONAL_SIZE * pulseCycleProgress; } 
            else { _quantaPool[i].currentPulseSize = QUANTA_PULSE_MAX_ADDITIONAL_SIZE * (1.0f - pulseCycleProgress); }
            float dx_to_spark = _sparkX - _quantaPool[i].x; float dy_to_spark = _sparkY - _quantaPool[i].y;
            float dist_to_spark_sq = dx_to_spark * dx_to_spark + dy_to_spark * dy_to_spark;
            float overlap_radius_sq = (_sparkSize + _quantaPool[i].actualRadius + 0.5f) * (_sparkSize + _quantaPool[i].actualRadius + 0.5f);
            _quantaPool[i].isInvertedDraw = (dist_to_spark_sq < overlap_radius_sq);
            if(_quantaPool[i].isInvertedDraw) _anyQuantaUnderSpark = true;
            if (_isAttracting) {
                float dx_att = _sparkX - _quantaPool[i].x; float dy_att = _sparkY - _quantaPool[i].y;
                float dist_att = sqrt(dx_att * dx_att + dy_att * dy_att);
                if (dist_att < ATTRACT_RADIUS && dist_att > 0.1f) {
                    if (_quantaPool[i].clusterId == -1 || _quantaPool[i].isClusterLead) {
                        float moveX = (dx_att / dist_att) * ATTRACT_STRENGTH; float moveY = (dy_att / dist_att) * ATTRACT_STRENGTH;
                        _quantaPool[i].x += moveX; _quantaPool[i].y += moveY;
                        if (_quantaPool[i].isClusterLead) {
                            for (int j = 0; j < MAX_QUANTA; ++j) {
                                if (i != j && _quantaPool[j].active && _quantaPool[j].clusterId == _quantaPool[i].clusterId) {
                                    _quantaPool[j].x += moveX; _quantaPool[j].y += moveY;
                                }
                            }
                        }
                    }
                }
            }
            else if (_quantaPool[i].clusterId != -1 && !_quantaPool[i].isClusterLead) {
                for (int j = 0; j < MAX_QUANTA; ++j) {
                    if (_quantaPool[j].active && _quantaPool[j].isClusterLead && _quantaPool[j].clusterId == _quantaPool[i].clusterId) {
                        float targetX = _quantaPool[j].x + _quantaPool[i].targetClusterOffsetX;
                        float targetY = _quantaPool[j].y + _quantaPool[i].targetClusterOffsetY;
                        _quantaPool[i].x += (targetX - _quantaPool[i].x) * 0.1f;
                        _quantaPool[i].y += (targetY - _quantaPool[i].y) * 0.1f;
                        break;
                    }
                }
            }
        }
    }
}

void _1AwakeningSparkScene::updateClustering(unsigned long currentTime) {
    for (int i = 0; i < MAX_QUANTA; ++i) {
        if (!_quantaPool[i].active || _quantaPool[i].clusterId != -1) continue;
        for (int j = i + 1; j < MAX_QUANTA; ++j) {
            if (!_quantaPool[j].active || _quantaPool[j].clusterId != -1) continue;
            float dx = _quantaPool[i].x - _quantaPool[j].x; float dy = _quantaPool[i].y - _quantaPool[j].y;
            float distSq = dx * dx + dy * dy; float touchRadius = _quantaPool[i].actualRadius + _quantaPool[j].actualRadius + 1.0f;
            bool alreadyTracking = false;
            for (const auto& pc : _potentialClusters) { if ((pc.q1_idx == i && pc.q2_idx == j) || (pc.q1_idx == j && pc.q2_idx == i)) { alreadyTracking = true; break; } }
            if (distSq < touchRadius * touchRadius && !alreadyTracking) { _potentialClusters.push_back({i, j, currentTime}); }
        }
    }
    for (auto it = _potentialClusters.begin(); it != _potentialClusters.end(); ) {
        int q1_idx = it->q1_idx; int q2_idx = it->q2_idx;
        if (!_quantaPool[q1_idx].active || !_quantaPool[q2_idx].active || _quantaPool[q1_idx].clusterId != -1 || _quantaPool[q2_idx].clusterId != -1) {
            it = _potentialClusters.erase(it); continue;
        }
        float dx = _quantaPool[q1_idx].x - _quantaPool[q2_idx].x; float dy = _quantaPool[q1_idx].y - _quantaPool[q2_idx].y;
        float distSq = dx*dx + dy*dy; float touchRadius = _quantaPool[q1_idx].actualRadius + _quantaPool[q2_idx].actualRadius + 1.0f;
        if (distSq >= touchRadius * touchRadius) { it = _potentialClusters.erase(it); continue; }
        if (currentTime - it->overlapStartTime >= CLUSTER_STICK_TIME_MS) {
            _quantaPool[q1_idx].clusterId = _nextClusterId; _quantaPool[q1_idx].isClusterLead = true;
            _quantaPool[q2_idx].clusterId = _nextClusterId; _quantaPool[q2_idx].isClusterLead = false;
            _quantaPool[q2_idx].targetClusterOffsetX = _quantaPool[q2_idx].x - _quantaPool[q1_idx].x;
            _quantaPool[q2_idx].targetClusterOffsetY = _quantaPool[q2_idx].y - _quantaPool[q1_idx].y;
            debugPrintf("SCENES", "Cluster formed (ID %d) between Quanta %d and %d", _nextClusterId, q1_idx, q2_idx);
            _nextClusterId++; it = _potentialClusters.erase(it);
        } else { ++it; }
    }
}

void _1AwakeningSparkScene::checkQuantaCollection() {
    for (int i = 0; i < MAX_QUANTA; ++i) {
        if (_quantaPool[i].active) {
            float dx = _sparkX - _quantaPool[i].x; float dy = _sparkY - _quantaPool[i].y;
            float distSq = dx * dx + dy * dy; float collectionRadius = _sparkSize + _quantaPool[i].actualRadius + 0.5f;
            if (distSq < (collectionRadius * collectionRadius)) {
                float energyGain = 0;
                if(_quantaPool[i].size == 1) energyGain = ENERGY_PER_QUANTA_SIZE_1; else energyGain = ENERGY_PER_QUANTA_SIZE_2;
                float absorbX = _quantaPool[i].x; float absorbY = _quantaPool[i].y; int absorbSize = _quantaPool[i].size;
                if (_quantaPool[i].clusterId != -1 && _quantaPool[i].isClusterLead) {
                    int clusterMembersAbsorbed = 1; _quantaPool[i].active = false;
                    for (int j = 0; j < MAX_QUANTA; ++j) {
                        if (i != j && _quantaPool[j].active && _quantaPool[j].clusterId == _quantaPool[i].clusterId) {
                            if(_quantaPool[j].size == 1) energyGain += ENERGY_PER_QUANTA_SIZE_1; else energyGain += ENERGY_PER_QUANTA_SIZE_2;
                             triggerAbsorbEffect(_quantaPool[j].x, _quantaPool[j].y, _quantaPool[j].size);
                            _quantaPool[j].active = false; clusterMembersAbsorbed++;
                        }
                    }
                    energyGain *= (1.0f + (clusterMembersAbsorbed -1) * CLUSTER_BONUS_MULTIPLIER); 
                    debugPrintf("SCENES", "Cluster (ID %d) absorbed, %d members, total energy gain: %.1f", _quantaPool[i].clusterId, clusterMembersAbsorbed, energyGain);
                } else if (_quantaPool[i].clusterId != -1 && !_quantaPool[i].isClusterLead) { continue; } 
                else { _quantaPool[i].active = false; }
                _sparkEnergy += energyGain; triggerAbsorbEffect(absorbX, absorbY, absorbSize);
                int newDifficultyLevel = (int)(_sparkEnergy / (ENERGY_TO_COLLECT / 5.0f));
                newDifficultyLevel = std::min(4, newDifficultyLevel);
                if (newDifficultyLevel > _currentDifficultyLevel) {
                    _currentDifficultyLevel = newDifficultyLevel;
                    debugPrintf("SCENES", "Difficulty Increased to Level %d (%.1f / %.1f)", _currentDifficultyLevel, _sparkEnergy, ENERGY_TO_COLLECT);
                }
                debugPrintf("SCENES", "Energy collected! Current: %.1f / %.1f", _sparkEnergy, ENERGY_TO_COLLECT);
                if (_sparkEnergy >= ENERGY_TO_COLLECT) { completeStage(); return; }
            }
        }
    }
}

void _1AwakeningSparkScene::update(unsigned long deltaTime) {
    unsigned long currentTime = millis();
    if (handleDialogKeyPress(GEM_KEY_OK)) { if (_dialogBox && !_dialogBox->isActive() && !_instructionsShown) { _instructionsShown = true; } return; }
    if (!_instructionsShown) return;
    if (_effectsManager) _effectsManager->update(currentTime); 
    float currentBgScrollX = _baseBackgroundScrollSpeedX; float currentBgScrollY = _baseBackgroundScrollSpeedY;
    _sparkIsInNebula = (_noise.GetNoise(_sparkX + _noiseOffsetX * 50, _sparkY + _noiseOffsetY * 50) > 0.45f);
    if (_sparkIsInNebula) { currentBgScrollX *= 0.4f; currentBgScrollY *= 0.4f; }
    _noiseOffsetX += currentBgScrollX; _noiseOffsetY += currentBgScrollY;
    if (_gameContext && _gameContext->renderer) { // Use context
        Renderer& r = *_gameContext->renderer;
        _sparkX += ((random(0, 101) / 100.0f) - 0.5f) * 0.05f; _sparkY += ((random(0, 101) / 100.0f) - 0.5f) * 0.05f;
        _sparkX = std::max((float)_sparkSize, std::min((float)r.getWidth() - _sparkSize -1, _sparkX));
        _sparkY = std::max((float)_sparkSize, std::min((float)r.getHeight() - _sparkSize -1 - 8, _sparkY));
    }
    if (currentTime - _lastQuantaSpawnTime > QUANTA_SPAWN_INTERVAL_MS) { spawnQuanta(); _lastQuantaSpawnTime = currentTime; }
    updateQuanta(currentTime); updateClustering(currentTime); updateBadQuanta(currentTime);
    unsigned long current_min_bad_spawn = BAD_QUANTA_BASE_MIN_SPAWN_INTERVAL_MS - (_currentDifficultyLevel * 1100); 
    unsigned long current_max_bad_spawn = BAD_QUANTA_BASE_MAX_SPAWN_INTERVAL_MS - (_currentDifficultyLevel * 1800); 
    current_min_bad_spawn = std::max(2500UL, current_min_bad_spawn); 
    current_max_bad_spawn = std::max(current_min_bad_spawn + 1500UL, current_max_bad_spawn); 
    if (currentTime - _lastBadQuantaSpawnTime > random(current_min_bad_spawn, current_max_bad_spawn + 1)) {
        spawnBadQuanta(); _lastBadQuantaSpawnTime = currentTime;
    }
    if (_isAttracting && (currentTime - _attractStartTime > ATTRACT_MAX_DURATION_MS)) { _isAttracting = false; debugPrint("SCENES", "Attract timed out (max hold duration)."); }
    if (_isPulsing && currentTime - _pulseAnimStartTime >= PULSE_ANIM_DURATION_MS) { _isPulsing = false; }
    if (_isAttracting || _anyQuantaUnderSpark) { _sparkCoreAnimCounter++; }
    if (_particleSystem) { _particleSystem->update(currentTime, 33); }
}

void _1AwakeningSparkScene::draw(Renderer& renderer) {
    if (!_gameContext || !_gameContext->display) { debugPrint("SCENES", "ERROR: _1AwakeningSparkScene::draw - Critical context members (display) null!"); return; }
    U8G2* u8g2 = _gameContext->display; // Use context
    int renderOffsetX = renderer.getXOffset(); int renderOffsetY = renderer.getYOffset();
    if (_effectsManager) { _effectsManager->applyEffectOffset(renderOffsetX, renderOffsetY); }
    for (int y = 0; y < renderer.getHeight(); y += 2) {
        for (int x = 0; x < renderer.getWidth(); x += 2) {
            float noiseVal = _noise.GetNoise((float)x + _noiseOffsetX * 50, (float)y + _noiseOffsetY * 50);
            if (noiseVal > 0.55f) { u8g2->drawPixel(renderOffsetX + x, renderOffsetY + y); }
        }
    }
    for (int i = 0; i < MAX_QUANTA; ++i) {
        if (_quantaPool[i].active) {
            float displayRadius = _quantaPool[i].actualRadius + _quantaPool[i].currentPulseSize;
            displayRadius = std::max(0.3f, displayRadius);
            int qx = renderOffsetX + (int)_quantaPool[i].x; int qy = renderOffsetY + (int)_quantaPool[i].y;
            if (_quantaPool[i].isInvertedDraw) {
                u8g2->setDrawColor(0);
                if (displayRadius < 0.6f) u8g2->drawPixel(qx, qy); else u8g2->drawDisc(qx, qy, (int)round(displayRadius));
                u8g2->setDrawColor(1);
            } else {
                if (displayRadius < 0.6f) u8g2->drawPixel(qx, qy); else u8g2->drawDisc(qx, qy, (int)round(displayRadius));
            }
            if (_quantaPool[i].clusterId != -1 && _quantaPool[i].isClusterLead) {
                for (int j = 0; j < MAX_QUANTA; ++j) {
                    if (i != j && _quantaPool[j].active && _quantaPool[j].clusterId == _quantaPool[i].clusterId) {
                        u8g2->drawLine(qx, qy, renderOffsetX + (int)_quantaPool[j].x, renderOffsetY + (int)_quantaPool[j].y);
                    }
                }
            }
        }
    }
    drawBadQuanta(renderer);
    int currentSparkDisplaySize = _sparkSize + (int)(_sparkEnergy / (ENERGY_TO_COLLECT / 2.0f));
    currentSparkDisplaySize = std::min(currentSparkDisplaySize, 6); int finalSparkDrawRadius = currentSparkDisplaySize;
    u8g2->drawDisc(renderOffsetX + (int)_sparkX, renderOffsetY + (int)_sparkY, finalSparkDrawRadius);
    if (_anyQuantaUnderSpark || _isAttracting) {
        u8g2->setDrawColor(0);
        for (int k = 0; k < 2 + (_isAttracting ? 1 : 0) ; ++k) {
            int randXOffset = random(-finalSparkDrawRadius / 2, finalSparkDrawRadius / 2 + 1) + (_sparkCoreAnimCounter % 3 -1);
            int randYOffset = random(-finalSparkDrawRadius / 2, finalSparkDrawRadius / 2 + 1) + ((_sparkCoreAnimCounter+1) % 3 -1);
            if (sqrt(randXOffset*randXOffset + randYOffset*randYOffset) < finalSparkDrawRadius * 0.65) {
                 u8g2->drawPixel(renderOffsetX + (int)_sparkX + randXOffset, renderOffsetY + (int)_sparkY + randYOffset);
            }
        }
        u8g2->setDrawColor(1);
        if (_isAttracting) {
            float pulseProgress = (float)(millis() % 400) / 400.0f;
            int pulseSize = (int)(sin(pulseProgress * PI * 2) * 1.0f);
            u8g2->drawCircle(renderOffsetX + (int)_sparkX, renderOffsetY + (int)_sparkY, finalSparkDrawRadius + pulseSize + 2);
        }
    }
    if (_isPulsing) {
        unsigned long pulseElapsedTime = millis() - _pulseAnimStartTime; float pulseProgress = (float)pulseElapsedTime / PULSE_ANIM_DURATION_MS;
        if (pulseProgress <= 1.0f) {
            int basePulseRadius = finalSparkDrawRadius + (int)(PULSE_MAX_RADIUS * pulseProgress); u8g2->setDrawColor(1);
            const int waveSegments = 16; float lastX = -999, lastY = -999;
            for (int k = 0; k <= waveSegments; ++k) {
                float angle = (2.0f * PI / waveSegments) * k;
                float noiseInputX = cos(angle) * 2.5f + (float)pulseElapsedTime * 0.025f;
                float noiseInputY = sin(angle) * 2.5f + (float)pulseElapsedTime * 0.025f;
                float waveOffset = _pulseNoise.GetNoise(noiseInputX, noiseInputY) * 3.0f * (1.0f - pulseProgress);
                float currentPulseRadius = basePulseRadius + waveOffset;
                currentPulseRadius = std::max((float)finalSparkDrawRadius + 1.0f, currentPulseRadius);
                float x = renderOffsetX + _sparkX + cos(angle) * currentPulseRadius;
                float y = renderOffsetY + _sparkY + sin(angle) * currentPulseRadius;
                if (k > 0) { u8g2->drawLine((int)round(lastX), (int)round(lastY), (int)round(x), (int)round(y)); }
                lastX = x; lastY = y;
            }
        }
    }
    if (_particleSystem) { _particleSystem->draw(); }
    int barWidth = renderer.getWidth() / 2; int barHeight = 5;
    int barX = renderer.getXOffset() + (renderer.getWidth() - barWidth) / 2; 
    int barY = renderer.getYOffset() + renderer.getHeight() - barHeight - 3; 
    int fillWidth = (int)((_sparkEnergy / ENERGY_TO_COLLECT) * (barWidth - 2));
    fillWidth = std::max(0, std::min(fillWidth, barWidth - 2));
    u8g2->drawFrame(barX, barY, barWidth, barHeight);
    if (fillWidth > 0) { u8g2->drawBox(barX + 1, barY + 1, fillWidth, barHeight - 2); }
    if (_dialogBox && _dialogBox->isActive()) { _dialogBox->draw(); }
}

void _1AwakeningSparkScene::applyPulseToBackground() {
    float angle = random(0, 360) * PI / 180.0f;
    float currentSpeed = sqrt(_baseBackgroundScrollSpeedX * _baseBackgroundScrollSpeedX + _baseBackgroundScrollSpeedY * _baseBackgroundScrollSpeedY);
    if (currentSpeed < 0.001f) currentSpeed = 0.0025f;
    float speedMultiplier = random(70, 131) / 100.0f;
    _baseBackgroundScrollSpeedX = cos(angle) * currentSpeed * speedMultiplier;
    _baseBackgroundScrollSpeedY = sin(angle) * currentSpeed * speedMultiplier;
    _baseBackgroundScrollSpeedX = std::max(-0.008f, std::min(0.008f, _baseBackgroundScrollSpeedX));
    _baseBackgroundScrollSpeedY = std::max(-0.008f, std::min(0.008f, _baseBackgroundScrollSpeedY));
    debugPrintf("SCENES", "Pulse changed bg scroll vector: new base dx:%.4f, dy:%.4f", _baseBackgroundScrollSpeedX, _baseBackgroundScrollSpeedY);
}

void _1AwakeningSparkScene::onButton1Press() {
    if (handleDialogKeyPress(GEM_KEY_UP)) { return; } if (!_instructionsShown) return;
    debugPrint("SCENES", "PrequelS1: Button 1 (Attract) PRESSED.");
    _isAttracting = true; _attractStartTime = millis(); _sparkCoreAnimCounter = 0;
}
void _1AwakeningSparkScene::onButton1Release() {
    if (!_instructionsShown && !(_dialogBox && _dialogBox->isActive())) return;
    debugPrint("SCENES", "PrequelS1: Button 1 (Attract) RELEASED.");
    _isAttracting = false;
}
void _1AwakeningSparkScene::onButton2Click() {
    if (handleDialogKeyPress(GEM_KEY_DOWN)) { return; } if (!_instructionsShown) return;
    debugPrint("SCENES", "PrequelS1: Button 2 (Absorb) CLICKED."); checkQuantaCollection();
}
void _1AwakeningSparkScene::onButton3Click() {
    if (handleDialogKeyPress(GEM_KEY_OK)) { if (_dialogBox && !_dialogBox->isActive() && !_instructionsShown) { _instructionsShown = true; } return; }
    if (!_instructionsShown) return;
    if (_sparkIsInNebula) { debugPrint("SCENES", "PrequelS1: Pulse disabled in nebula!"); triggerScreenShake(); return; }
    if (_sparkEnergy < PULSE_ENERGY_COST) { debugPrint("SCENES", "PrequelS1: Not enough energy to pulse!"); triggerScreenShake(); return; }
    _sparkEnergy -= PULSE_ENERGY_COST;
    debugPrintf("SCENES", "PrequelS1: Button 3 (Pulse) CLICKED. Energy: %.1f (-%.1f)", _sparkEnergy, PULSE_ENERGY_COST);
    if (!_isPulsing) { _isPulsing = true; _pulseAnimStartTime = millis(); _pulseNoise.SetSeed(random(0,10000)); applyPulseToBackground(); }
    const float PULSE_PUSH_RADIUS = PULSE_MAX_RADIUS * 0.85f; const float PULSE_STRENGTH = 6.5f;
    float sparkNebulaVal = _noise.GetNoise(_sparkX + _noiseOffsetX * 50, _sparkY + _noiseOffsetY * 50);
    if (sparkNebulaVal > 0.55f && random(100) < 40) {
        int numToSpawn = random(1, 3); debugPrintf("SCENES", "Pulse in nebula! Spawning %d quanta.", numToSpawn);
        for(int n=0; n<numToSpawn; ++n) spawnQuanta();
    }
    for (int k_loop = 0; k_loop < MAX_QUANTA; ++k_loop) {
        if (_quantaPool[k_loop].active) {
            float dx = _quantaPool[k_loop].x - _sparkX; float dy = _quantaPool[k_loop].y - _sparkY; float dist = sqrt(dx * dx + dy * dy);
            if (dist < PULSE_PUSH_RADIUS && dist > 0.1f) {
                if (_quantaPool[k_loop].clusterId != -1) {
                    int CId = _quantaPool[k_loop].clusterId; debugPrintf("SCENES", "Pulse broke cluster ID %d involving quanta %d", CId, k_loop);
                    for (int j = 0; j < MAX_QUANTA; ++j) {
                        if (_quantaPool[j].clusterId == CId) {
                            _quantaPool[j].clusterId = -1; _quantaPool[j].isClusterLead = false;
                             _quantaPool[j].x += (dx / dist) * (PULSE_STRENGTH * 0.6f) + random(-25,26)/10.0f;
                             _quantaPool[j].y += (dy / dist) * (PULSE_STRENGTH * 0.6f) + random(-25,26)/10.0f;
                        }
                    }
                } else { _quantaPool[k_loop].x += (dx / dist) * PULSE_STRENGTH; _quantaPool[k_loop].y += (dy / dist) * PULSE_STRENGTH; }
                _quantaPool[k_loop].x += random(-15, 16) / 10.0f; _quantaPool[k_loop].y += random(-15, 16) / 10.0f;
                if(_gameContext && _gameContext->renderer){ // Use context
                    Renderer& r = *_gameContext->renderer;
                    _quantaPool[k_loop].x = std::max(0.0f, std::min((float)r.getWidth() -1, _quantaPool[k_loop].x));
                    _quantaPool[k_loop].y = std::max(0.0f, std::min((float)r.getHeight()-1, _quantaPool[k_loop].y));
                }
            }
        }
    }
    for (int k_loop = 0; k_loop < MAX_BAD_QUANTA; ++k_loop) {
        if (_badQuantaPool[k_loop].active) {
            float dx = _badQuantaPool[k_loop].x - _sparkX; float dy = _badQuantaPool[k_loop].y - _sparkY; float dist = sqrt(dx * dx + dy * dy);
            if (dist < PULSE_PUSH_RADIUS && dist > 0.1f) {
                _badQuantaPool[k_loop].health--; _badQuantaPool[k_loop].radius = std::max(1.0f, _badQuantaPool[k_loop].radius - 0.5f);
                debugPrintf("SCENES", "Pulse hit bad quanta %d, health now %d", k_loop, _badQuantaPool[k_loop].health);
                if (_badQuantaPool[k_loop].health <= 0) {
                    _badQuantaPool[k_loop].active = false; _sparkEnergy += 10.0f;
                    debugPrintf("SCENES", "Bad quanta %d killed by pulse!", k_loop);
                } else {
                    _badQuantaPool[k_loop].x += (dx / dist) * (PULSE_STRENGTH * 1.3f); _badQuantaPool[k_loop].y += (dy / dist) * (PULSE_STRENGTH * 1.3f);
                     if(_gameContext && _gameContext->renderer){ // Use context
                        Renderer& r = *_gameContext->renderer;
                        _badQuantaPool[k_loop].x = std::max(0.0f - _badQuantaPool[k_loop].radius, std::min((float)r.getWidth() -1 + _badQuantaPool[k_loop].radius, _badQuantaPool[k_loop].x));
                        _badQuantaPool[k_loop].y = std::max(0.0f - _badQuantaPool[k_loop].radius, std::min((float)r.getHeight()-1 + _badQuantaPool[k_loop].radius, _badQuantaPool[k_loop].y));
                    }
                }
            }
        }
    }
}

void _1AwakeningSparkScene::spawnBadQuanta() {
    if (!_gameContext || !_gameContext->renderer) return; // Use context
    Renderer& r = *_gameContext->renderer;
    for (int i = 0; i < MAX_BAD_QUANTA; ++i) {
        if (!_badQuantaPool[i].active) {
            _badQuantaPool[i].active = true; int edge = random(4);
            if (edge == 0) { _badQuantaPool[i].x = -5; _badQuantaPool[i].y = random(0, r.getHeight()); }
            else if (edge == 1) { _badQuantaPool[i].x = r.getWidth() + 5; _badQuantaPool[i].y = random(0, r.getHeight()); }
            else if (edge == 2) { _badQuantaPool[i].x = random(0, r.getWidth()); _badQuantaPool[i].y = -5; }
            else { _badQuantaPool[i].x = random(0, r.getWidth()); _badQuantaPool[i].y = r.getHeight() + 5; }
            _badQuantaPool[i].vx = ((random(0, 101) / 100.0f) - 0.5f) * BAD_QUANTA_BASE_WANDER_SPEED * 2.0f;
            _badQuantaPool[i].vy = ((random(0, 101) / 100.0f) - 0.5f) * BAD_QUANTA_BASE_WANDER_SPEED * 2.0f;
            _badQuantaPool[i].lastRandomMoveTime = millis(); int sizeRoll = random(100);
            if (sizeRoll < 55) { _badQuantaPool[i].health = 1; _badQuantaPool[i].radius = 1.5f; }
            else if (sizeRoll < 85) { _badQuantaPool[i].health = 2; _badQuantaPool[i].radius = 2.0f; }
            else { _badQuantaPool[i].health = 3; _badQuantaPool[i].radius = 2.5f; }
            _badQuantaPool[i].spawnTime = millis(); _badQuantaPool[i].waveAnimationTime = random(0, 1000);
            debugPrintf("SCENES", "Spawned Bad Quanta %d (Health: %d, Radius: %.1f) at %.1f, %.1f", i, _badQuantaPool[i].health, _badQuantaPool[i].radius, _badQuantaPool[i].x, _badQuantaPool[i].y);
            break;
        }
    }
}

void _1AwakeningSparkScene::updateBadQuanta(unsigned long currentTime) {
    if(!_gameContext || !_gameContext->renderer) return; // Use context
    Renderer& r = *_gameContext->renderer;
    float currentAttractStrength = BAD_QUANTA_BASE_ATTRACT_STRENGTH + (_currentDifficultyLevel * 0.05f);
    float currentWanderSpeed = BAD_QUANTA_BASE_WANDER_SPEED + (_currentDifficultyLevel * 0.03f);
    for (int i = 0; i < MAX_BAD_QUANTA; ++i) {
        if (_badQuantaPool[i].active) {
            if (currentTime - _badQuantaPool[i].spawnTime > _badQuantaPool[i].lifetimeMs) {
                _badQuantaPool[i].active = false; debugPrintf("SCENES", "Bad Quanta %d timed out and disappeared.", i); continue;
            }
            _badQuantaPool[i].waveAnimationTime += 0.05f;
            if (currentTime - _badQuantaPool[i].lastRandomMoveTime > BAD_QUANTA_RANDOM_MOVE_INTERVAL_MS) {
                _badQuantaPool[i].vx = ((random(0, 101) / 100.0f) - 0.5f) * currentWanderSpeed * 2.0f;
                _badQuantaPool[i].vy = ((random(0, 101) / 100.0f) - 0.5f) * currentWanderSpeed * 2.0f;
                _badQuantaPool[i].lastRandomMoveTime = currentTime;
            }
            _badQuantaPool[i].x += _badQuantaPool[i].vx; _badQuantaPool[i].y += _badQuantaPool[i].vy;
            float dx_to_spark = _sparkX - _badQuantaPool[i].x; float dy_to_spark = _sparkY - _badQuantaPool[i].y;
            float dist_to_spark = sqrt(dx_to_spark * dx_to_spark + dy_to_spark * dy_to_spark);
            if (dist_to_spark < BAD_QUANTA_AUTO_ATTRACT_RADIUS && dist_to_spark > 0.1f) {
                _badQuantaPool[i].x += (dx_to_spark / dist_to_spark) * currentAttractStrength;
                _badQuantaPool[i].y += (dy_to_spark / dist_to_spark) * currentAttractStrength;
            }
            if (_badQuantaPool[i].x - _badQuantaPool[i].radius < 0) { _badQuantaPool[i].x = _badQuantaPool[i].radius; _badQuantaPool[i].vx *= -1;}
            if (_badQuantaPool[i].x + _badQuantaPool[i].radius > r.getWidth()) { _badQuantaPool[i].x = r.getWidth() - _badQuantaPool[i].radius; _badQuantaPool[i].vx *= -1;}
            if (_badQuantaPool[i].y - _badQuantaPool[i].radius < 0) { _badQuantaPool[i].y = _badQuantaPool[i].radius; _badQuantaPool[i].vy *= -1;}
            if (_badQuantaPool[i].y + _badQuantaPool[i].radius > r.getHeight()) { _badQuantaPool[i].y = r.getHeight() - _badQuantaPool[i].radius; _badQuantaPool[i].vy *= -1;}
            float collision_dist_sq = (_sparkSize + _badQuantaPool[i].radius - 0.5f) * (_sparkSize + _badQuantaPool[i].radius - 0.5f);
            if (dist_to_spark * dist_to_spark < collision_dist_sq) {
                _sparkEnergy -= BAD_QUANTA_ENERGY_DRAIN; _sparkEnergy = std::max(0.0f, _sparkEnergy);
                _badQuantaPool[i].health--; _badQuantaPool[i].radius = std::max(1.0f, _badQuantaPool[i].radius - 0.5f);
                triggerScreenShake(); 
                debugPrintf("SCENES", "Spark hit by Bad Quanta %d! Energy: %.1f. Bad Quanta Health: %d", i, _sparkEnergy, _badQuantaPool[i].health);
                if (_badQuantaPool[i].health <= 0) {
                    _badQuantaPool[i].active = false; debugPrintf("SCENES", "Bad quanta %d destroyed by collision!", i);
                } else {
                    _badQuantaPool[i].x -= (dx_to_spark / dist_to_spark) * 7.0f; _badQuantaPool[i].y -= (dy_to_spark / dist_to_spark) * 7.0f;
                }
            }
        }
    }
}

void _1AwakeningSparkScene::drawBadQuanta(Renderer& renderer) {
    if (!_gameContext || !_gameContext->display) return; // Use context
    U8G2* u8g2 = _gameContext->display;
    int rOffsetX = renderer.getXOffset(); int rOffsetY = renderer.getYOffset();
    if (_effectsManager) { _effectsManager->applyEffectOffset(rOffsetX, rOffsetY); }
    for (int i = 0; i < MAX_BAD_QUANTA; ++i) {
        if (_badQuantaPool[i].active) {
            int bqx = rOffsetX + (int)_badQuantaPool[i].x; int bqy = rOffsetY + (int)_badQuantaPool[i].y;
            int radius = (int)round(_badQuantaPool[i].radius); if (radius < 1) radius = 1;
            u8g2->setDrawColor(1); u8g2->drawDisc(bqx, bqy, radius);
            if (radius > 1) { u8g2->setDrawColor(0); u8g2->drawDisc(bqx, bqy, radius -1 ); u8g2->setDrawColor(1); }
            for (int w = 0; w < 1; ++w) {
                float waveRadiusProg = fmod(_badQuantaPool[i].waveAnimationTime + w * 0.7f, 1.0f);
                int waveRadius = radius + 2 + (int)(waveRadiusProg * 4.0f);
                if (waveRadiusProg > 0.2f && waveRadiusProg < 0.8f) { u8g2->drawCircle(bqx, bqy, waveRadius); }
            }
        }
    }
}

void _1AwakeningSparkScene::triggerScreenShake() { if (_effectsManager) { _effectsManager->triggerScreenShake(); } }
void _1AwakeningSparkScene::triggerAbsorbEffect(float x, float y, int size) {
    if (_particleSystem) { _particleSystem->spawnAbsorbEffect(_sparkX, _sparkY, x, y, 4, size); }
}

void _1AwakeningSparkScene::completeStage() {
    debugPrint("SCENES", "_1AwakeningSparkScene: Stage 1 Complete!");
    if (_gameContext && _gameContext->gameStats) { // Use context
        _gameContext->gameStats->setCompletedPrequelStage(PrequelStage::STAGE_1_AWAKENING_COMPLETE);
        _gameContext->gameStats->addPoints(50);
        _gameContext->gameStats->save();
    }
    debugPrint("SCENES", "Prequel Stage 1 complete. Transitioning to Stage 2.");
    if (_gameContext && _gameContext->sceneManager) { // Use context
        _gameContext->sceneManager->requestSetCurrentScene("PREQUEL_STAGE_2");
    } else {
        debugPrint("SCENES", "Error: SceneManager (via context) is null. Cannot switch scene.");
    }
}

// Implement the dialog handling for this scene
bool _1AwakeningSparkScene::handleDialogKeyPress(uint8_t keyCode) {
    if (_dialogBox && _dialogBox->isActive()) {
        if (keyCode == GEM_KEY_OK) {
            _dialogBox->close();
            _instructionsShown = true;
            return true;
        }
        // Could add up/down for scrolling if dialog becomes scrollable
        if (keyCode == GEM_KEY_DOWN || keyCode == GEM_KEY_UP) {
            // Placeholder for future scrollable dialogs
            return true;
        }
    }
    return false;
}