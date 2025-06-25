#include "GameScene.h"
#include "SceneManager.h"
#include "GameStats.h"
#include "Localization.h"
#include "SerialForwarder.h"
#include "../../DebugUtils.h"
#include "GEM_u8g2.h" 
#include "../../System/GameContext.h" 
#include "InputManager.h"

// extern GameStats* gameStats_ptr; // Removed
// extern String nextSceneName; // Removed - cause of linker error
// extern bool replaceSceneStack; // Removed - cause of linker error
// extern bool sceneChangeRequested; // Removed - cause of linker error
// extern SerialForwarder* forwardedSerial_ptr; // Access via context

GameScene::GameScene() :
    _gameStartTimeMs(0),
    _lastFatigueUpdateTimeMs(0),
    _isPausedByFatigue(false),
    _lowFatigueWarningShown(false),
    _highFatigueWarningShown(false)
{
    debugPrint("SCENES", "GameScene constructor");
}

GameScene::~GameScene() {
    debugPrint("SCENES", "GameScene destructor");
}

void GameScene::init(GameContext& context) { 
    Scene::init(); 
    _gameContext = &context;
    
    if (!_gameContext || !_gameContext->renderer) {
        debugPrint("SCENES", "ERROR: GameScene::init - Critical context members (renderer) are null!");
        return;
    }
    _fatigueWarningDialog.reset(new DialogBox(*_gameContext->renderer)); 
    debugPrint("SCENES", "GameScene::init");
}

void GameScene::onEnter() {
    Scene::onEnter(); 
    _gameStartTimeMs = millis();
    _lastFatigueUpdateTimeMs = _gameStartTimeMs;
    _isPausedByFatigue = false;
    _lowFatigueWarningShown = false;
    _highFatigueWarningShown = false;
    if (_fatigueWarningDialog) _fatigueWarningDialog->close();
    debugPrint("SCENES", "GameScene::onEnter");
}

void GameScene::onExit() {
    Scene::onExit(); 
    if (_fatigueWarningDialog) _fatigueWarningDialog->close();
    debugPrint("SCENES", "GameScene::onExit");
}

void GameScene::update(unsigned long deltaTime) {
    Scene::update(deltaTime); 
    unsigned long currentTime = millis();

    if (_fatigueWarningDialog && _fatigueWarningDialog->isActive()) {
        _fatigueWarningDialog->update(currentTime);
    }
    
    if (_isPausedByFatigue && _fatigueWarningDialog && _fatigueWarningDialog->isActive()) {
        return; 
    } else if (_isPausedByFatigue && _fatigueWarningDialog && !_fatigueWarningDialog->isActive()) {
        if (_gameContext && _gameContext->gameStats && _gameContext->gameStats->fatigue < FATIGUE_MAX_EXIT_THRESHOLD) {
            _isPausedByFatigue = false;
            onGameResumedFromFatiguePause();
            debugPrint("SCENES", "GameScene: Resumed from fatigue pause.");
        } else {
            signalGameExit(false, 0); 
            return;
        }
    }
    
    handleFatigueManagement(currentTime);
}

void GameScene::draw(Renderer& renderer) { 
    Scene::draw(renderer); 
    if (_fatigueWarningDialog && _fatigueWarningDialog->isActive()) {
        _fatigueWarningDialog->draw();
    }
}

bool GameScene::isFatigueDialogActive() const {
    return _fatigueWarningDialog && _fatigueWarningDialog->isActive();
}

bool GameScene::handleDialogKeyPress(uint8_t keyCode) {
    DialogBox* dialog = getDialogBox();
    if (dialog && dialog->isActive()) {
        debugPrintf("SCENES", "GameScene::handleDialogKeyPress: Dialog active, processing key %d\n", keyCode);
        switch (keyCode) {
            case GEM_KEY_UP:
                dialog->scrollUp();
                return true;
            case GEM_KEY_DOWN:
                dialog->scrollDown();
                return true;
            case GEM_KEY_OK:
                if (dialog->isTemporary()) {
                    dialog->close();
                } else {
                    if (dialog->isAtBottom()) dialog->close();
                    else dialog->scrollDown();
                }
                return true;
            case GEM_KEY_CANCEL:
                dialog->close();
                return true;
            default:
                debugPrintf("SCENES", "GameScene::handleDialogKeyPress: Key %d not handled by standard dialog logic.\n", keyCode);
                return false;
        }
    }
    return false;
}

void GameScene::signalGameExit(bool completedSuccessfully, uint32_t pointsEarned) {
    debugPrintf("SCENES", "GameScene: Signaling game exit. Success: %s, Points: %u",
               completedSuccessfully ? "true" : "false", pointsEarned);

    if (_gameContext && _gameContext->gameStats && pointsEarned > 0) { // Use context
        _gameContext->gameStats->addPoints(pointsEarned);
    }

    if (_gameContext && _gameContext->sceneManager) { // Use context
        _gameContext->sceneManager->requestSetCurrentScene("ACTIONS");
    } else {
        debugPrint("SCENES", "Error: SceneManager (via context) is null. Cannot signal game exit.");
    }

    if (_isPausedByFatigue) {
        _isPausedByFatigue = false;
        onGameResumedFromFatiguePause(); 
    }
}

unsigned long GameScene::getGameDurationMs() const {
    return millis() - _gameStartTimeMs;
}

void GameScene::handleFatigueManagement(unsigned long currentTime) {
    if (!_gameContext || !_gameContext->gameStats) return; // Use context
    GameStats* gameStats = _gameContext->gameStats;


    if (currentTime - _lastFatigueUpdateTimeMs >= FATIGUE_INCREASE_INTERVAL_MS) {
        unsigned long intervalsPassed = (currentTime - _lastFatigueUpdateTimeMs) / FATIGUE_INCREASE_INTERVAL_MS;
        _lastFatigueUpdateTimeMs += intervalsPassed * FATIGUE_INCREASE_INTERVAL_MS; 
        uint8_t oldFatigue = gameStats->fatigue;
        gameStats->setFatigue(std::min((uint8_t)100, (uint8_t)(gameStats->fatigue + FATIGUE_PER_INTERVAL * intervalsPassed)));
        if (gameStats->fatigue != oldFatigue) {
            debugPrintf("SCENES", "GameScene: Fatigue increased by %u to %u during play.", (FATIGUE_PER_INTERVAL * intervalsPassed), gameStats->fatigue);
        }
    }

    if (gameStats->fatigue >= FATIGUE_MAX_EXIT_THRESHOLD) {
        debugPrint("SCENES", "GameScene: Fatigue MAXED OUT! Exiting game.");
        if (_fatigueWarningDialog) {
             if (!_fatigueWarningDialog->isActive() || !_isPausedByFatigue) { 
                _fatigueWarningDialog->showTemporary(loc(StringKey::DIALOG_STILL_TOO_TIRED), 2500); 
             }
            _isPausedByFatigue = true; 
            onGamePausedByFatigue(); 
        }
        if (!(_fatigueWarningDialog && _fatigueWarningDialog->isActive())) {
             signalGameExit(false, 0); 
        }
    } else if (gameStats->fatigue >= FATIGUE_WARNING_HIGH_THRESHOLD) {
        if (!_highFatigueWarningShown && !(_fatigueWarningDialog && _fatigueWarningDialog->isActive())) { 
            showFatigueWarningDialog(gameStats->fatigue);
            _highFatigueWarningShown = true;
            _lowFatigueWarningShown = true; 
            _isPausedByFatigue = true;
            onGamePausedByFatigue();
            debugPrint("SCENES", "GameScene: High fatigue warning. Game paused.");
        }
    } else if (gameStats->fatigue >= FATIGUE_WARNING_LOW_THRESHOLD) {
        if (!_lowFatigueWarningShown && !(_fatigueWarningDialog && _fatigueWarningDialog->isActive())) { 
            showFatigueWarningDialog(gameStats->fatigue);
            _lowFatigueWarningShown = true;
            debugPrint("SCENES", "GameScene: Low fatigue warning.");
        }
    } else {
        _lowFatigueWarningShown = false;
        _highFatigueWarningShown = false;
        if (_isPausedByFatigue && !(_fatigueWarningDialog && _fatigueWarningDialog->isActive())) { 
            _isPausedByFatigue = false;
            onGameResumedFromFatiguePause();
             debugPrint("SCENES", "GameScene: Fatigue dropped, resuming game from pause.");
        }
    }
}

void GameScene::showFatigueWarningDialog(uint8_t currentFatigue) {
    if (_fatigueWarningDialog) {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "%s (%d%%)", loc(StringKey::DIALOG_NOT_TIRED_ENOUGH), currentFatigue); 
        _fatigueWarningDialog->showTemporary(buffer, 3000); 
    }
}

void GameScene::onGamePausedByFatigue() {
    debugPrint("SCENES", "GameScene Base: Game paused by fatigue.");
}

void GameScene::onGameResumedFromFatiguePause() {
    debugPrint("SCENES", "GameScene Base: Game resumed from fatigue pause.");
}