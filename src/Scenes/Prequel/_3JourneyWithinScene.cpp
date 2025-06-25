#include "_3JourneyWithinScene.h"
#include "SceneManager.h"
#include "InputManager.h"
#include "Renderer.h"
#include "GameStats.h"
#include "SerialForwarder.h"
#include "Localization.h"
#include <cmath>
#include "../../Helper/EffectsManager.h"
#include "../../ParticleSystem.h"
#include "../../DebugUtils.h"
#include "GEM_u8g2.h"
#include "../../System/GameContext.h"

const float _3JourneyWithinScene::_gatePositions[GATE_COUNT] = {
    _3JourneyWithinScene::TARGET_PROGRESS * 0.33f,
    _3JourneyWithinScene::TARGET_PROGRESS * 0.66f};

extern String nextSceneName;
extern bool replaceSceneStack;
extern bool sceneChangeRequested;

float normalizeAngleLocal_S3(float angle)
{
    while (angle <= -PI)
        angle += 2 * PI;
    while (angle > PI)
        angle -= 2 * PI;
    return angle;
}

_3JourneyWithinScene::_3JourneyWithinScene()
{
    debugPrint("SCENES", "_3JourneyWithinScene constructor: Initializing noise generators...");
    _zoneNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2S);
    _zoneNoise.SetFrequency(0.025f);
    _zoneNoise.SetSeed(esp_random());
    _currentNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    _currentNoise.SetFrequency(0.08f);
    _currentNoise.SetSeed(esp_random());
    _thicketElementNoise.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
    _thicketElementNoise.SetFrequency(0.15f);
    _thicketElementNoise.SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction_Hybrid);
    _thicketElementNoise.SetCellularReturnType(FastNoiseLite::CellularReturnType_CellValue);
    _thicketElementNoise.SetSeed(esp_random());
    debugPrint("SCENES", "_3JourneyWithinScene constructor DONE");
}

_3JourneyWithinScene::~_3JourneyWithinScene()
{
    debugPrint("SCENES", "_3JourneyWithinScene destroyed");
}

void _3JourneyWithinScene::init(GameContext &context)
{
    Scene::init();
    _gameContext = &context;
    debugPrint("SCENES", "_3JourneyWithinScene::init - _gameContext is");
    if (_gameContext)
        debugPrint("SCENES", "VALID");
    else
        debugPrint("SCENES", "NULL");

    if (!_gameContext || !_gameContext->renderer || !_gameContext->display)
    {
        debugPrint("SCENES", "ERROR: _3JourneyWithinScene::init - Critical context members are null!");
        return;
    }
    _dialogBox.reset(new DialogBox(*_gameContext->renderer));
    _effectsManager.reset(new EffectsManager(*_gameContext->renderer));
    _particleSystem.reset(new ParticleSystem(*_gameContext->renderer, _gameContext->defaultFont)); 

    _gameContext->inputManager->registerButtonListener(EDGE_Button::LEFT, EDGE_Event::PRESS, this, [this](){ 
        if (handleDialogKeyPress(GEM_KEY_UP)) { return; }
        if (_currentPhase != JourneyPhase::AT_GATE_QTE) this->handleButton1(millis());
        else this->handleQTEInput(QTEButton::LEFT); });
    _gameContext->inputManager->registerButtonListener(EDGE_Button::RIGHT, EDGE_Event::PRESS, this, [this](){ 
        if (handleDialogKeyPress(GEM_KEY_DOWN)) { return; }
        if (_currentPhase != JourneyPhase::AT_GATE_QTE) this->handleButton2(millis());
        else this->handleQTEInput(QTEButton::RIGHT); });
    _gameContext->inputManager->registerButtonListener(EDGE_Button::OK, EDGE_Event::PRESS, this, [this](){ 
        if (handleDialogKeyPress(GEM_KEY_OK)) { 
            if (_dialogBox && !_dialogBox->isActive() && !_instructionsShown) _instructionsShown = true;
            return; 
        }
        if (_currentPhase != JourneyPhase::AT_GATE_QTE) this->handleButton3(millis());
        else this->handleQTEInput(QTEButton::OK); });
    _gameContext->inputManager->registerButtonListener(EDGE_Button::OK, EDGE_Event::LONG_PRESS, this, [this]()
                          {
        if (handleDialogKeyPress(GEM_KEY_CANCEL)) { return; }
        if (_dialogBox && _dialogBox->isActive()) { _dialogBox->close(); _instructionsShown = true; } });
}

void _3JourneyWithinScene::onEnter()
{
    debugPrint("SCENES", "_3JourneyWithinScene::onEnter - _gameContext is");
    if (_gameContext)
        debugPrint("SCENES", "VALID");
    else
        debugPrint("SCENES", "NULL");
    _instructionsShown = false;
    _zoneNoise.SetSeed(esp_random());
    _currentNoise.SetSeed(esp_random());
    _thicketElementNoise.SetSeed(esp_random());
    debugPrint("SCENES", "  Noise re-seeded.");

    if (_gameContext && _gameContext->renderer)
    { 
        Renderer &r = *_gameContext->renderer;
        _playerX = r.getWidth() / 2.0f;
        _playerY = r.getHeight() / 2.0f;
        debugPrintf("SCENES", "  Player Start: (%.1f, %.1f) in %dx%d space. Renderer: %p", _playerX, _playerY, r.getWidth(), r.getHeight(), &r);
    }
    else
    {
        _playerX = 128 / 2.0f;
        _playerY = 64 / 2.0f;
        debugPrint("SCENES", "  WARNING: GameContext or Renderer is NULL in onEnter, using default player pos.");
    }
    _playerAngle = -PI / 2.0f;
    _playerVX = 0.0f;
    _playerVY = 0.0f;
    _journeyProgress = 0.0f;
    _currentPhase = JourneyPhase::CALM_FLOW;
    _phaseTransitionGraceTime = 0;
    _noiseOffsetX = (float)random(0, 20000) / 100.0f;
    _noiseOffsetY = (float)random(0, 20000) / 100.0f;
    _backgroundScrollX = 0.0f;
    _backgroundScrollY = 0.0f;
    _lastUpdateTime = millis();
    debugPrintf("SCENES", "  Initial Noise Offsets: X=%.2f, Y=%.2f. Scroll: X=%.1f, Y=%.1f", _noiseOffsetX, _noiseOffsetY, _backgroundScrollX, _backgroundScrollY);
    _isBoosting = false;
    _boostEndTime = 0;
    _boostCooldownTime = 0;
    for (int i = 0; i < GATE_COUNT; ++i)
    {
        _gateActive[i] = false;
        _gatePassed[i] = false;
    }
    _qteCurrentStep = 0;
    if (_dialogBox)
    {
        String instructions = String(loc(StringKey::PREQUEL_STAGE3_TITLE)) + "\n" +
                              String(loc(StringKey::PREQUEL_STAGE3_INST_LINE1)) + "\n" +
                              String(loc(StringKey::PREQUEL_STAGE3_INST_LINE2)) + "\n" +
                              String(loc(StringKey::PREQUEL_STAGE3_INST_LINE3));
        _dialogBox->show(instructions.c_str());
    }
    else
    {
        _instructionsShown = true;
    }
    if (_effectsManager)
    { 
    }
    if (_particleSystem)
        _particleSystem->reset();
    if (_gameContext && _gameContext->display)
    {
        U8G2 *u8g2 = _gameContext->display;
        u8g2->setFont(u8g2_font_4x6_tf);
        u8g2->setDrawColor(1);
        u8g2->setFontPosTop();
        debugPrint("SCENES", "  U8G2 font and color reset in onEnter.");
    }
}

void _3JourneyWithinScene::onExit()
{
    debugPrint("SCENES", "_3JourneyWithinScene::onExit");
    if (_dialogBox)
        _dialogBox->close();
    if (_gameContext && _gameContext->inputManager)
        _gameContext->inputManager->unregisterAllListenersForScene(this);
}

void _3JourneyWithinScene::update(unsigned long deltaTime)
{
    unsigned long currentTime = millis();
    float dt = (currentTime - _lastUpdateTime) / 1000.0f;
    if (dt <= 0.001f)
        dt = 0.016f;
    _lastUpdateTime = currentTime;
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
    if (_particleSystem)
        _particleSystem->update(currentTime, deltaTime);
    if (_isBoosting && currentTime >= _boostEndTime)
    {
        _isBoosting = false;
    }
    if (_phaseTransitionGraceTime > 0 && currentTime >= _phaseTransitionGraceTime)
    {
        _phaseTransitionGraceTime = 0;
        debugPrint("SCENES", "  Phase transition grace period ended.");
    }
    if (_currentPhase == JourneyPhase::GATE_OPENING)
    {
        if (currentTime >= _gateAnimEndTime)
        {
            _currentPhase = _phaseBeforeGate;
        }
        else
        {
            return;
        }
    }
    else if (_currentPhase == JourneyPhase::AT_GATE_QTE)
    {
        updateGateQTE(currentTime, dt);
        return;
    }
    else if (_currentPhase == JourneyPhase::STAGE_COMPLETE_ANIM)
    {
        if (currentTime >= _gateAnimEndTime)
        {
            completeStage();
        }
        return;
    }
    checkZoneTransition();
    switch (_currentPhase)
    {
    case JourneyPhase::CALM_FLOW:
        updateCurrentRiding(currentTime, dt);
        break;
    case JourneyPhase::THICKET:
        updateThicketNavigation(currentTime, dt);
        break;
    case JourneyPhase::RAPID_CURRENT:
        updateRapidCurrent(currentTime, dt);
        break;
    default:
        break;
    }
    applyPhysics(dt);
    if (_currentPhase != JourneyPhase::AT_GATE_QTE && _currentPhase != JourneyPhase::GATE_OPENING)
    {
        float speedFactor = 1.0f;
        if (_currentPhase == JourneyPhase::THICKET)
            speedFactor = 0.5f;
        else if (_currentPhase == JourneyPhase::RAPID_CURRENT && _phaseTransitionGraceTime == 0)
            speedFactor = 1.5f;
        else if (_currentPhase == JourneyPhase::RAPID_CURRENT && _phaseTransitionGraceTime > 0)
            speedFactor = 0.75f;
        if (_isBoosting && (_currentPhase == JourneyPhase::CALM_FLOW || _currentPhase == JourneyPhase::RAPID_CURRENT))
            speedFactor *= 1.8f;
        _journeyProgress += PROGRESS_PER_SECOND_BASE * dt * speedFactor;
        _backgroundScrollX += _playerVX * dt * 0.05f;
        _backgroundScrollY += _playerVY * dt * 0.05f;
    }
    for (int i = 0; i < GATE_COUNT; ++i)
    {
        if (!_gatePassed[i] && !_gateActive[i] && _journeyProgress >= _gatePositions[i])
        {
            triggerGate(i, currentTime);
            break;
        }
    }
    if (_journeyProgress >= TARGET_PROGRESS && _currentPhase != JourneyPhase::STAGE_COMPLETE_ANIM)
    {
        _currentPhase = JourneyPhase::STAGE_COMPLETE_ANIM;
        _gateAnimEndTime = currentTime + 1500;
        debugPrint("SCENES", "Stage 3: Reached target progress, starting completion anim.");
    }
}

void _3JourneyWithinScene::applyPhysics(float dt)
{
    _playerX += _playerVX * dt * 60.0f;
    _playerY += _playerVY * dt * 60.0f;
    float currentDrag = DRAG_FACTOR;
    if (_currentPhase == JourneyPhase::THICKET)
        currentDrag = THICKET_DRAG;
    _playerVX *= pow(currentDrag, dt * 60.0f);
    _playerVY *= pow(currentDrag, dt * 60.0f);
    if (abs(_playerVX) < 0.001f)
        _playerVX = 0.0f;
    if (abs(_playerVY) < 0.001f)
        _playerVY = 0.0f;
    if (_gameContext && _gameContext->renderer)
    {
        Renderer &r = *_gameContext->renderer;
        float boundLeft = PLAYER_RADIUS;
        float boundRight = r.getWidth() - PLAYER_RADIUS - 1;
        float boundTop = PLAYER_RADIUS;
        float boundBottom = r.getHeight() - PLAYER_RADIUS - 1;
        bool hitWall = false;
        float wallDamping = -0.1f;
        if (_playerX < boundLeft)
        {
            _playerX = boundLeft;
            _playerVX *= wallDamping;
            hitWall = true;
        }
        if (_playerX > boundRight)
        {
            _playerX = boundRight;
            _playerVX *= wallDamping;
            hitWall = true;
        }
        if (_playerY < boundTop)
        {
            _playerY = boundTop;
            _playerVY *= wallDamping;
            hitWall = true;
        }
        if (_playerY > boundBottom)
        {
            _playerY = boundBottom;
            _playerVY *= wallDamping;
            hitWall = true;
        }
        if (hitWall && _effectsManager && _currentPhase == JourneyPhase::RAPID_CURRENT)
        {
            if (!_effectsManager->isShaking())
                _effectsManager->triggerScreenShake(1, 100);
        }
        else if (hitWall && _effectsManager)
        {
            if (!_effectsManager->isShaking())
                _effectsManager->triggerScreenShake(1, 60);
        }
    }
}

void _3JourneyWithinScene::checkZoneTransition()
{
    if (!_gameContext || !_gameContext->renderer || !_gameContext->display)
        return;
    Renderer &r = *_gameContext->renderer;
    U8G2 *u8g2 = _gameContext->display;
    float zoneNoiseVal = _zoneNoise.GetNoise(_playerX * 0.015f + _noiseOffsetX, _playerY * 0.015f + _noiseOffsetY);
    JourneyPhase newPhase = JourneyPhase::CALM_FLOW;
    if (zoneNoiseVal < THICKET_NOISE_THRESHOLD)
    {
        newPhase = JourneyPhase::THICKET;
    }
    else if (zoneNoiseVal > RAPID_NOISE_THRESHOLD)
    {
        newPhase = JourneyPhase::RAPID_CURRENT;
    }
    if (_currentPhase == JourneyPhase::AT_GATE_QTE || _currentPhase == JourneyPhase::GATE_OPENING || _currentPhase == JourneyPhase::STAGE_COMPLETE_ANIM)
    {
        return;
    }
    if (newPhase != _currentPhase)
    {
        debugPrintf("SCENES", "Phase transition: %d -> %d (Noise: %.2f)", (int)_currentPhase, (int)newPhase, zoneNoiseVal);
        JourneyPhase oldPhase = _currentPhase;
        _currentPhase = newPhase;
        _phaseTransitionGraceTime = millis() + PHASE_TRANSITION_GRACE_MS;
        if (_particleSystem)
        {
            int numParticles = 15;
            float speed = 0.8f;
            unsigned long lifetime = 400;
            char pChar = '*';
            const uint8_t *pFont = (_gameContext && _gameContext->defaultFont) ? _gameContext->defaultFont : u8g2_font_spleen5x8_mr; // Use context defaultFont

            if (_currentPhase == JourneyPhase::THICKET)
            {
                _playerVX *= 0.1f;
                _playerVY *= 0.1f;
                pChar = '#';
                pFont = u8g2_font_4x6_tf;
                speed = 0.4f;
                lifetime = 600;
            }
            else if (_currentPhase == JourneyPhase::RAPID_CURRENT)
            {
                pChar = '>';
                speed = 1.5f;
                lifetime = 250;
            }
            else if (_currentPhase == JourneyPhase::CALM_FLOW && oldPhase != JourneyPhase::CALM_FLOW)
            {
                pChar = '~';
                pFont = u8g2_font_spleen5x8_mr;
                speed = 0.3f;
                lifetime = 700;
                numParticles = 25;
            }
            for (int i = 0; i < numParticles; ++i)
            {
                float angle = random(0, 360) * PI / 180.0f;
                _particleSystem->spawnParticle(_playerX, _playerY, cos(angle) * speed, sin(angle) * speed, lifetime, pChar, pFont);
            }
        }
        if (_effectsManager)
            _effectsManager->triggerScreenShake(1, 150);
    }
}

void _3JourneyWithinScene::updateCurrentRiding(unsigned long currentTime, float dt)
{
    float currentForceScale = CALM_FORCE_MAGNITUDE;
    if (_phaseTransitionGraceTime > 0 && currentTime < _phaseTransitionGraceTime)
    {
        currentForceScale *= 0.5f;
    }
    float angle = _currentNoise.GetNoise(_playerX * 0.1f, _playerY * 0.1f, currentTime * 0.0002f) * PI * 2.0f;
    _playerVX += cos(angle) * currentForceScale;
    _playerVY += sin(angle) * currentForceScale;
    if (_particleSystem && random(100) < 5)
    {
        _particleSystem->spawnParticle(_playerX, _playerY, -_playerVX * 0.1f, -_playerVY * 0.1f, 300, '.', u8g2_font_3x5im_te);
    }
}
void _3JourneyWithinScene::updateThicketNavigation(unsigned long currentTime, float dt)
{
    if (_particleSystem && random(100) < 8)
    {
        _particleSystem->spawnParticle(_playerX, _playerY, -_playerVX * 0.2f, -_playerVY * 0.2f, 200, 'o', u8g2_font_4x6_tf, 2);
    }
}
void _3JourneyWithinScene::updateRapidCurrent(unsigned long currentTime, float dt)
{
    float currentForceScale = RAPID_FORCE_MAGNITUDE;
    if (_phaseTransitionGraceTime > 0 && currentTime < _phaseTransitionGraceTime)
    {
        currentForceScale *= 0.3f;
    }
    float angle = _currentNoise.GetNoise(_playerX * 0.08f + 100.f, _playerY * 0.08f + 100.f, currentTime * 0.0005f) * PI * 2.0f;
    _playerVX += cos(angle) * currentForceScale;
    _playerVY += sin(angle) * currentForceScale;
    float speedSq = _playerVX * _playerVX + _playerVY * _playerVY;
    if (speedSq > RAPID_COLLISION_VELOCITY_THRESHOLD * RAPID_COLLISION_VELOCITY_THRESHOLD)
    {
        if (_effectsManager && !_effectsManager->isShaking() && random(100) < 20)
        {
            _effectsManager->triggerScreenShake(1, 100);
            if (_particleSystem && random(100) < 50)
            {
                for (int i = 0; i < 2; ++i)
                {
                    _particleSystem->spawnParticle(_playerX, _playerY, ((float)random(-5, 6)) / 10.f, ((float)random(-5, 6)) / 10.f, 150, '.', u8g2_font_3x5im_te, 2);
                }
            }
        }
        _playerVX *= 0.85f;
        _playerVY *= 0.85f;
    }
    if (_particleSystem && random(100) < 15)
    {
        _particleSystem->spawnParticle(_playerX, _playerY, -_playerVX * 0.3f, -_playerVY * 0.3f, 150, '`', u8g2_font_3x5im_te, (_isBoosting ? 1 : 2));
    }
}
void _3JourneyWithinScene::triggerGate(int gateIndex, unsigned long currentTime)
{
    if (gateIndex < 0 || gateIndex >= GATE_COUNT || _gateActive[gateIndex] || _gatePassed[gateIndex])
        return;
    debugPrintf("SCENES", "Stage 3: Triggering Gate %d QTE.", gateIndex);
    _phaseBeforeGate = _currentPhase;
    _currentPhase = JourneyPhase::AT_GATE_QTE;
    _gateActive[gateIndex] = true;
    _qteCurrentStep = 0;
    generateQTESequence();
    _qteStepStartTime = currentTime;
    if (_effectsManager)
        _effectsManager->triggerScreenShake(2, 200);
    if (_particleSystem && _gameContext && _gameContext->renderer)
    {
        Renderer &r = *_gameContext->renderer;
        for (int i = 0; i < 20; ++i)
        {
            _particleSystem->spawnParticle(r.getWidth() * 0.75f + random(-5, 6), random(0, r.getHeight()),
                                           ((float)random(-5, 6)) / 10.f, ((float)random(-15, 16)) / 10.f,
                                           500, ':', u8g2_font_spleen5x8_mr);
        }
    }
    if (_dialogBox)
    {
        _dialogBox->showTemporary(loc(StringKey::PREQUEL_STAGE3_QTE_PROMPT), QTE_STEP_TIME_LIMIT_MS * QTE_SEQUENCE_LENGTH + 500);
    }
}
void _3JourneyWithinScene::generateQTESequence()
{
    for (int i = 0; i < QTE_SEQUENCE_LENGTH; ++i)
    {
        int r = random(3);
        if (r == 0)
            _qteSequence[i] = QTEButton::LEFT;
        else if (r == 1)
            _qteSequence[i] = QTEButton::RIGHT;
        else
            _qteSequence[i] = QTEButton::OK;
    }
}
void _3JourneyWithinScene::updateGateQTE(unsigned long currentTime, float dt)
{
    if (currentTime - _qteStepStartTime > QTE_STEP_TIME_LIMIT_MS)
    {
        debugPrint("SCENES", "Stage 3: QTE step timed out. Failed gate.");
        if (_dialogBox)
            _dialogBox->showTemporary(loc(StringKey::PREQUEL_STAGE3_GATE_FAILED), 1500);
        _journeyProgress -= GATE_PROGRESS_PENALTY;
        _journeyProgress = std::max(0.0f, _journeyProgress);
        _currentPhase = _phaseBeforeGate;
        for (int i = 0; i < GATE_COUNT; ++i)
            _gateActive[i] = false;
        if (_effectsManager)
            _effectsManager->triggerScreenShake(3, 300);
        if (_particleSystem)
        {
            for (int i = 0; i < 15; ++i)
            {
                _particleSystem->spawnParticle(_playerX, _playerY, ((float)random(-15, 16)) / 10.f, ((float)random(-15, 16)) / 10.f, 600, 'x', u8g2_font_spleen5x8_mr, 2);
            }
        }
    }
}
void _3JourneyWithinScene::handleQTEInput(QTEButton pressedButton)
{
    if (_currentPhase != JourneyPhase::AT_GATE_QTE)
        return;
    unsigned long currentTime = millis();
    if (_dialogBox && _dialogBox->isActive())
        _dialogBox->close();
    if (pressedButton == _qteSequence[_qteCurrentStep])
    {
        debugPrintf("SCENES", "Stage 3: QTE Step %d Correct!", _qteCurrentStep);
        _qteCurrentStep++;
        _qteStepStartTime = currentTime;
        if (_particleSystem)
        {
            _particleSystem->spawnParticle(_playerX, _playerY, 0, -0.5f, 200, '^', u8g2_font_spleen5x8_mr);
        }
        if (_qteCurrentStep >= QTE_SEQUENCE_LENGTH)
        {
            debugPrint("SCENES", "Stage 3: QTE Success! Gate passed.");
            if (_dialogBox)
                _dialogBox->showTemporary(loc(StringKey::PREQUEL_STAGE3_GATE_PASSED), 1000);
            for (int i = 0; i < GATE_COUNT; ++i)
            {
                if (_gateActive[i])
                    _gatePassed[i] = true;
                _gateActive[i] = false;
            }
            _currentPhase = JourneyPhase::GATE_OPENING;
            _gateAnimEndTime = currentTime + GATE_OPEN_ANIM_DURATION_MS;
            if (_effectsManager)
                _effectsManager->triggerScreenShake(1, GATE_OPEN_ANIM_DURATION_MS - 50);
            if (_particleSystem && _gameContext && _gameContext->renderer)
            {
                Renderer &r = *_gameContext->renderer;
                for (int i = 0; i < 25; ++i)
                {
                    float angle = (float)i / 25.f * 2.f * PI;
                    _particleSystem->spawnParticle(r.getWidth() * 0.75f, _playerY + random(-10, 11),
                                                   cos(angle) * 1.5f, sin(angle) * 1.5f, 600, '*', u8g2_font_courR08_tr);
                }
            }
        }
    }
    else
    {
        debugPrint("SCENES", "Stage 3: QTE Step Failed.");
        if (_dialogBox)
            _dialogBox->showTemporary(loc(StringKey::PREQUEL_STAGE3_GATE_FAILED), 1500);
        _journeyProgress -= GATE_PROGRESS_PENALTY;
        _journeyProgress = std::max(0.0f, _journeyProgress);
        _currentPhase = _phaseBeforeGate;
        for (int i = 0; i < GATE_COUNT; ++i)
            _gateActive[i] = false;
        if (_effectsManager)
            _effectsManager->triggerScreenShake(3, 300);
        if (_particleSystem)
        {
            for (int i = 0; i < 15; ++i)
            {
                _particleSystem->spawnParticle(_playerX, _playerY, ((float)random(-15, 16)) / 10.f, ((float)random(-15, 16)) / 10.f, 600, 'x', u8g2_font_spleen5x8_mr, 2);
            }
        }
    }
}

void _3JourneyWithinScene::draw(Renderer &renderer)
{
    if (!_gameContext || !_gameContext->display)
    {
        debugPrint("SCENES", "ERROR: _3JourneyWithinScene::draw - Critical context member (display) null!");
        return;
    }
    U8G2 *u8g2 = _gameContext->display;
    int rOffsetX = renderer.getXOffset();
    int rOffsetY = renderer.getYOffset();
    if (_effectsManager && _effectsManager->isShaking())
    {
        _effectsManager->applyEffectOffset(rOffsetX, rOffsetY);
    }
    u8g2->setFont(u8g2_font_4x6_tf);
    u8g2->setDrawColor(1);
    u8g2->setFontPosTop();
    drawBackgroundAndZones(renderer, rOffsetX, rOffsetY);
    if (_currentPhase != JourneyPhase::GATE_OPENING && _currentPhase != JourneyPhase::STAGE_COMPLETE_ANIM)
    {
        drawPlayer(renderer, rOffsetX, rOffsetY);
    }
    for (int i = 0; i < GATE_COUNT; ++i)
    {
        if (!_gatePassed[i])
        {
            drawGate(renderer, rOffsetX, rOffsetY, i);
        }
    }
    if (_currentPhase == JourneyPhase::AT_GATE_QTE)
    {
        drawQTEUI(renderer);
    }
    drawProgressBar(renderer);
    if (_particleSystem)
        _particleSystem->draw();
    if (_dialogBox && _dialogBox->isActive())
    {
        _dialogBox->draw();
    }
}

void _3JourneyWithinScene::drawBackgroundAndZones(Renderer &renderer, int rOffsetX, int rOffsetY)
{
    if (!_gameContext || !_gameContext->display)
        return;
    U8G2 *u8g2 = _gameContext->display;
    float scrollFactor = 0.05f;
    uint8_t physicalWidth = u8g2->getDisplayWidth();
    uint8_t physicalHeight = u8g2->getDisplayHeight();
    for (int y_base = 0; y_base < renderer.getHeight(); y_base += 3)
    {
        for (int x_base = 0; x_base < renderer.getWidth(); x_base += 3)
        {
            int finalDrawX = rOffsetX + x_base;
            int finalDrawY = rOffsetY + y_base;
            if (finalDrawX + 2 < 0 || finalDrawX >= physicalWidth || finalDrawY + 2 < 0 || finalDrawY >= physicalHeight)
            {
                continue;
            }
            float noiseX = (float)x_base + _noiseOffsetX - _backgroundScrollX * scrollFactor;
            float noiseY = (float)y_base + _noiseOffsetY - _backgroundScrollY * scrollFactor;
            float zoneVal = _zoneNoise.GetNoise(noiseX * 0.015f, noiseY * 0.015f);
            if (zoneVal < THICKET_NOISE_THRESHOLD)
            {
                float thicketVal = _thicketElementNoise.GetNoise(noiseX * 0.3f, noiseY * 0.3f);
                if (thicketVal > 0.65f)
                    u8g2->drawPixel(finalDrawX, finalDrawY);
                else if (thicketVal < -0.65f)
                    u8g2->drawPixel(finalDrawX + 1, finalDrawY + 1);
            }
            else if (zoneVal > RAPID_NOISE_THRESHOLD)
            {
                float currentAngle = _currentNoise.GetNoise(noiseX * 0.1f, noiseY * 0.1f, millis() * 0.0005f) * PI;
                if (random(100) < 15)
                {
                    int len = random(3, 7);
                    u8g2->drawLine(finalDrawX, finalDrawY, finalDrawX + (int)(cos(currentAngle) * len), finalDrawY + (int)(sin(currentAngle) * len));
                }
            }
            else
            {
                float currentVal = _currentNoise.GetNoise(noiseX * 0.05f, noiseY * 0.05f, millis() * 0.0002f);
                if (currentVal > 0.55f)
                    u8g2->drawPixel(finalDrawX, finalDrawY);
                else if (currentVal < -0.55f && random(100) < 5)
                    u8g2->drawPixel(finalDrawX, finalDrawY);
            }
        }
    }
}
void _3JourneyWithinScene::drawPlayer(Renderer &renderer, int rOffsetX, int rOffsetY)
{
    if (!_gameContext || !_gameContext->display)
        return;
    U8G2 *u8g2 = _gameContext->display;
    int drawX = rOffsetX + static_cast<int>(round(_playerX));
    int drawY = rOffsetY + static_cast<int>(round(_playerY));
    uint8_t physicalWidth = u8g2->getDisplayWidth();
    uint8_t physicalHeight = u8g2->getDisplayHeight();
    int maxPlayerExtent = PLAYER_RADIUS + 3;
    if (drawX + maxPlayerExtent < 0 || drawX - maxPlayerExtent > physicalWidth ||
        drawY + maxPlayerExtent < 0 || drawY - maxPlayerExtent > physicalHeight)
    {
        return;
    }
    u8g2->setDrawColor(1);
    u8g2->drawDisc(drawX, drawY, PLAYER_RADIUS);
    float pulse = abs(sin(millis() * 0.005f));
    u8g2->drawCircle(drawX, drawY, PLAYER_RADIUS + (int)(pulse * 1.5f));
    if (_currentPhase == JourneyPhase::THICKET)
    {
        int lineEndX = drawX + static_cast<int>(round(cos(_playerAngle) * (PLAYER_RADIUS + 2)));
        int lineEndY = drawY + static_cast<int>(round(sin(_playerAngle) * (PLAYER_RADIUS + 2)));
        u8g2->drawLine(drawX, drawY, lineEndX, lineEndY);
    }
    if (_isBoosting)
    {
        u8g2->drawCircle(drawX, drawY, PLAYER_RADIUS + 2 + random(0, 2));
        if (_particleSystem && random(100) < 80)
        {
            _particleSystem->spawnParticle(_playerX, _playerY, -_playerVX * 0.7f, -_playerVY * 0.7f, 250, '*', u8g2_font_spleen5x8_mr, 2);
        }
    }
}
void _3JourneyWithinScene::drawProgressBar(Renderer &renderer)
{
    if (!_gameContext || !_gameContext->display)
        return;
    U8G2 *u8g2 = _gameContext->display;
    int barWidth = renderer.getWidth() / 2;
    int barHeight = 5;
    int barX = renderer.getXOffset() + (renderer.getWidth() - barWidth) / 2;
    int barY = renderer.getYOffset() + renderer.getHeight() - barHeight - 2;
    float progressRatio = (_journeyProgress / TARGET_PROGRESS);
    progressRatio = std::max(0.0f, std::min(1.0f, progressRatio));
    int fillWidth = static_cast<int>(progressRatio * (barWidth - 2));
    fillWidth = std::max(0, std::min(fillWidth, barWidth - 2));
    u8g2->setDrawColor(1);
    u8g2->drawRFrame(barX, barY, barWidth, barHeight, 1);
    if (fillWidth > 0)
    {
        u8g2->drawRBox(barX + 1, barY + 1, fillWidth, barHeight - 2, 0);
    }
}
void _3JourneyWithinScene::drawGate(Renderer &renderer, int rOffsetX, int rOffsetY, int gateIndex)
{
    if (!_gameContext || !_gameContext->display || !_gameContext->renderer)
        return;
    U8G2 *u8g2 = _gameContext->display;
    Renderer &r_ref = *_gameContext->renderer;
    int logicalGateX = static_cast<int>(r_ref.getWidth() * 0.75f);
    int drawGateX = rOffsetX + logicalGateX;
    if (_currentPhase == JourneyPhase::GATE_OPENING && _gateActive[gateIndex])
    {
        unsigned long timeSinceOpenStart = millis() - (_gateAnimEndTime - GATE_OPEN_ANIM_DURATION_MS);
        float openProgress = (float)timeSinceOpenStart / GATE_OPEN_ANIM_DURATION_MS;
        openProgress = std::min(1.0f, openProgress);
        int openingGap = (int)(r_ref.getHeight() * openProgress * 1.2f);
        int halfGap = openingGap / 2;
        int gatePartHeight = r_ref.getHeight() / 2 - halfGap;
        u8g2->setDrawColor(1);
        if (gatePartHeight > 0)
            u8g2->drawRBox(drawGateX - 2, rOffsetY, 4, gatePartHeight, 1);
        if (gatePartHeight > 0)
            u8g2->drawRBox(drawGateX - 2, rOffsetY + r_ref.getHeight() - gatePartHeight, 4, gatePartHeight, 1);
    }
    else if (_gateActive[gateIndex])
    {
        u8g2->setDrawColor(1);
        for (int y_local = 0; y_local < r_ref.getHeight(); y_local += 5)
        {
            int drawY = rOffsetY + y_local;
            if ((y_local / 5 + (int)(millis() / 150)) % 2 == 0)
                u8g2->drawBox(drawGateX - 2, drawY, 4, 3);
            else
                u8g2->drawFrame(drawGateX - 2, drawY, 4, 3);
        }
    }
}
void _3JourneyWithinScene::drawQTEUI(Renderer &renderer)
{
    if (!_gameContext || !_gameContext->display)
        return;
    U8G2 *u8g2 = _gameContext->display;
    int uiX = renderer.getXOffset() + renderer.getWidth() / 2;
    int uiY = renderer.getYOffset() + 8;
    u8g2->setFont(u8g2_font_6x12_tf);
    int charSpacing = 8;
    for (int i = 0; i < QTE_SEQUENCE_LENGTH; ++i)
    {
        char btnChar = '?';
        switch (_qteSequence[i])
        {
        case QTEButton::LEFT:
            btnChar = '<';
            break;
        case QTEButton::RIGHT:
            btnChar = '>';
            break;
        case QTEButton::OK:
            btnChar = 'O';
            break;
        default:
            break;
        }
        int charX = uiX - (QTE_SEQUENCE_LENGTH * charSpacing / 2) + i * charSpacing;
        if (i == _qteCurrentStep)
        {
            u8g2->setDrawColor(1);
            u8g2->drawRBox(charX - 2, uiY - 2, u8g2->getStrWidth(&btnChar) + 4, u8g2->getMaxCharHeight() + 2, 1);
            u8g2->setDrawColor(0);
            u8g2->drawGlyph(charX, uiY, btnChar);
            u8g2->setDrawColor(1);
        }
        else if (i < _qteCurrentStep)
        {
            u8g2->setDrawColor(1);
            u8g2->drawGlyph(charX, uiY, '.');
        }
        else
        {
            u8g2->setDrawColor(1);
            u8g2->drawGlyph(charX, uiY, btnChar);
        }
    }
    unsigned long timeElapsedInStep = millis() - _qteStepStartTime;
    float timeProgress = (float)timeElapsedInStep / QTE_STEP_TIME_LIMIT_MS;
    int timeBarWidth = renderer.getWidth() / 2;
    int timeBarFill = (int)((1.0f - timeProgress) * (timeBarWidth - 2));
    timeBarFill = std::max(0, timeBarFill);
    u8g2->setDrawColor(1);
    u8g2->drawRFrame(renderer.getXOffset() + (renderer.getWidth() - timeBarWidth) / 2, uiY + 12, timeBarWidth, 5, 1);
    if (timeBarFill > 0)
        u8g2->drawRBox(renderer.getXOffset() + (renderer.getWidth() - timeBarWidth) / 2 + 1, uiY + 13, timeBarFill, 3, 0);
}

void _3JourneyWithinScene::completeStage()
{
    debugPrint("SCENES", "_3JourneyWithinScene: Stage 3 Complete!");
    if (_gameContext && _gameContext->gameStats && _gameContext->sceneManager)
    {
        _gameContext->gameStats->setCompletedPrequelStage(PrequelStage::STAGE_3_JOURNEY_COMPLETE);
        _gameContext->gameStats->addPoints(150);
        _gameContext->gameStats->save();
        _gameContext->sceneManager->requestSetCurrentScene("PREQUEL_STAGE_4");
    }
    else
    {
        debugPrint("SCENES", "Error: Critical context members null in completeStage.");
    }
}

void _3JourneyWithinScene::handleButton1(unsigned long currentTime)
{
    if (_isBoosting && _currentPhase != JourneyPhase::THICKET)
        return;
    switch (_currentPhase)
    {
    case JourneyPhase::CALM_FLOW:
        _playerVX -= (CALM_STEER_FORCE * 1.5f);
        break;
    case JourneyPhase::THICKET:
        _playerX += cos(_playerAngle) * THICKET_MOVE_SPEED;
        _playerY += sin(_playerAngle) * THICKET_MOVE_SPEED;
        break;
    case JourneyPhase::RAPID_CURRENT:
        _playerVX -= _playerVX * RAPID_RESIST_FORCE * 1.5f;
        _playerVY -= _playerVY * RAPID_RESIST_FORCE * 1.5f;
        break;
    default:
        break;
    }
}
void _3JourneyWithinScene::handleButton2(unsigned long currentTime)
{
    if (_isBoosting && _currentPhase != JourneyPhase::THICKET)
        return;
    switch (_currentPhase)
    {
    case JourneyPhase::CALM_FLOW:
        _playerVX += (CALM_STEER_FORCE * 1.5f);
        break;
    case JourneyPhase::THICKET:
        _playerX -= cos(_playerAngle) * THICKET_MOVE_SPEED * 0.6f;
        _playerY -= sin(_playerAngle) * THICKET_MOVE_SPEED * 0.6f;
        break;
    case JourneyPhase::RAPID_CURRENT:
        _playerVX += (_playerVX > 0 ? -1 : 1) * RAPID_RESIST_FORCE * 0.75f;
        _playerVY += (_playerVY > 0 ? -1 : 1) * RAPID_RESIST_FORCE * 0.75f;
        break;
    default:
        break;
    }
}
void _3JourneyWithinScene::handleButton3(unsigned long currentTime)
{
    if (_isBoosting || currentTime < _boostCooldownTime)
        return;
    _isBoosting = true;
    _boostEndTime = currentTime + BOOST_DURATION_MS;
    _boostCooldownTime = _boostEndTime + BOOST_COOLDOWN_MS;
    if (_particleSystem)
    {
        for (int i = 0; i < 10; ++i)
        {
            _particleSystem->spawnParticle(_playerX, _playerY,
                                           ((float)random(-10, 11)) / 20.f - _playerVX * 0.5f,
                                           ((float)random(-10, 11)) / 20.f - _playerVY * 0.5f,
                                           250, '+', u8g2_font_spleen5x8_mr, 1);
        }
    }
    switch (_currentPhase)
    {
    case JourneyPhase::CALM_FLOW:
    case JourneyPhase::RAPID_CURRENT:
    {
        float currentSpeed = sqrt(_playerVX * _playerVX + _playerVY * _playerVY);
        float boostAngle = _playerAngle;
        if (currentSpeed > 0.01f)
        {
            boostAngle = atan2(_playerVY, _playerVX);
        }
        _playerVX += cos(boostAngle) * BOOST_FORCE_MAGNITUDE;
        _playerVY += sin(boostAngle) * BOOST_FORCE_MAGNITUDE;
    }
    break;
    case JourneyPhase::THICKET:
        _playerAngle += radians(THICKET_ROTATE_SPEED_DEG * 10.0f);
        _playerAngle = normalizeAngleLocal_S3(_playerAngle);
        _playerX += cos(_playerAngle) * BOOST_NUDGE_THICKET;
        _playerY += sin(_playerAngle) * BOOST_NUDGE_THICKET;
        break;
    default:
        _isBoosting = false;
        break;
    }
    if (_isBoosting)
        debugPrintf("SCENES", "Stage 3: Boost Activated (Phase: %d)", (int)_currentPhase);
}

bool _3JourneyWithinScene::handleDialogKeyPress(uint8_t keyCode) {
    if (_dialogBox && _dialogBox->isActive()) {
        if (keyCode == GEM_KEY_OK) {
            _dialogBox->close();
            _instructionsShown = true;
            return true;
        }
    }
    return false;
}