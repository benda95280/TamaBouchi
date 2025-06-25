#include "FlappyBirdScene.h"
#include "InputManager.h" 
#include "Renderer.h"     
#include "SceneManager.h" 
#include "espasyncbutton.hpp"
#include "Pipe.h" 
#include <vector>
#include <memory> 
#include <algorithm> 
#include <limits>    
#include "GameStats.h" 
#include "SerialForwarder.h" 
#include "FlappyBirdGraphics.h" 
#include "Coin.h"
#include "PowerUpItem.h"
#include "FlyingEnemy.h"
#include "../../../DebugUtils.h"
#include "GEM_u8g2.h" 
#include "../../../System/GameContext.h" 

// --- Re-add or ensure these constants are defined (either here or in .h) ---
// Constants from the original FlappyBirdScene.cpp
#define JUMP_FORCE -2.0f
#define PIPE_SPEED_NORMAL_BASE 1 
// #define PIPE_SPEED_FAST_BASE 2 // Not used directly, speed scales with level
#define TOP_BOTTOM_PADDING 10
#define INITIAL_GRAVITY 0.18f 
// --- End Constants ---


FlappyBirdScene::FlappyBirdScene() : gameIsOver(false), _sessionCoins(0), _highScore(0) { 
    debugPrint("SCENES", "FlappyBirdScene constructor.");
}

int FlappyBirdScene::getMinPipeSpacing() {
    int baseMinSpacing = SCREEN_WIDTH / 2 - 5; 
    int reduction = (currentLevel / 2) * 4; 
    int minSafeSpacing = BIRD_RADIUS * 4 + getPipeWidth(); 
    int calculatedMin = baseMinSpacing - reduction;
    return std::max({minSafeSpacing, calculatedMin, 30}); 
}
int FlappyBirdScene::getMaxPipeSpacing() {
    int baseMaxSpacing = SCREEN_WIDTH / 2 + 25; 
    int reduction = (currentLevel / 2) * 6; 
    int minPossible = getMinPipeSpacing() + 5; 
    int calculatedMax = baseMaxSpacing - reduction;
    return std::max(minPossible, calculatedMax);
}
float FlappyBirdScene::getGravityForLevel() {
    float base = INITIAL_GRAVITY + (currentLevel / 9.0f) * 0.020f; 
    float range = 0.015f + (currentLevel / 5.0f) * 0.005f; 
    float calculatedGravity = random(0, (int)(range * 1000)) / 1000.0f + base;
    calculatedGravity = std::max(0.15f, std::min(0.30f, calculatedGravity)); 
    return calculatedGravity;
}

void FlappyBirdScene::configurePipe(Pipe& pipe) {
    int movingPipeChance = MOVING_PIPE_BASE_CHANCE_PERCENT + static_cast<int>((currentLevel - LEVEL_START_MOVING_PIPES) * MOVING_PIPE_CHANCE_SCALAR_PER_LEVEL);
    movingPipeChance = std::min(movingPipeChance, MOVING_PIPE_MAX_CHANCE_PERCENT);
    movingPipeChance = std::max(0, movingPipeChance); 

    if (currentLevel >= LEVEL_START_MOVING_PIPES && random(100) < movingPipeChance) { 
        float verticalSpeed = MOVING_PIPE_BASE_SPEED + ((currentLevel - LEVEL_START_MOVING_PIPES) / MOVING_PIPE_SPEED_SCALAR_PER_LEVEL_DIVISOR) * MOVING_PIPE_SPEED_INCREMENT_AMOUNT;
        verticalSpeed = std::min(verticalSpeed, MOVING_PIPE_MAX_SPEED);                
        verticalSpeed = std::max(0.005f, verticalSpeed); 

        int moveRange = 6 + currentLevel / 1.0f; 
        moveRange = std::min(moveRange, 15);
        int pipeGapCurrent = pipe.getGapHeight(); 
        int pipeCenterY = pipe.getGapY() + pipeGapCurrent / 2; 

        int minY = std::max(TOP_BOTTOM_PADDING + 3, pipeCenterY - moveRange - pipeGapCurrent / 2);
        int maxY = std::min(SCREEN_HEIGHT - pipeGapCurrent - TOP_BOTTOM_PADDING - 3, pipeCenterY + moveRange - pipeGapCurrent / 2);
        if (minY >= maxY) { minY = TOP_BOTTOM_PADDING + 3; maxY = SCREEN_HEIGHT - pipeGapCurrent - TOP_BOTTOM_PADDING - 4; }
        if (maxY <= minY) maxY = minY + 1; 

        pipe.setMoving(true, verticalSpeed, minY, maxY);
    }

    if (currentLevel >= LEVEL_START_GAP_CHANGING_PIPES && random(100) < 8 + currentLevel * 2.0f) { 
        float gapChangeSpeedAbs = 0.005f + (currentLevel / 28.0f) * 0.01f; 
        gapChangeSpeedAbs = std::min(gapChangeSpeedAbs, 0.03f); 
        float gapChangeSpeed = (random(0,2) == 0 ? -1 : 1) * gapChangeSpeedAbs;
        
        int currentPipeGap = pipe.getGapHeight(); 
        int changeAmount = 2 + currentLevel / 4; 
        int targetGap = currentPipeGap + (gapChangeSpeed > 0 ? -changeAmount : changeAmount); 
        targetGap = std::max(8, std::min(30, targetGap)); 
        pipe.setGapChanging(true, gapChangeSpeed, targetGap);
    }

    if (currentLevel >= LEVEL_START_FRAGILE_PIPES && random(100) < 6 + currentLevel * 1.2f) { 
        pipe.setFragile(true);
    }

    if (currentLevel >= LEVEL_START_PIPE_HAZARDS && random(100) < 8 + currentLevel * 1.5f) { 
        bool topHazard = random(0,2) == 0;
        bool bottomHazard = random(0,2) == 0;
        if (!topHazard && !bottomHazard && random(0,3)==0) topHazard = true; 
        if (topHazard || bottomHazard) pipe.setHazards(topHazard, bottomHazard, 3);
    }
}

void FlappyBirdScene::addPipeObject(int xPos) {
    int pipeWidth = getPipeWidth();
    int pipeGap = getPipeGap();
    int pipeGapY = random(TOP_BOTTOM_PADDING, SCREEN_HEIGHT - pipeGap - TOP_BOTTOM_PADDING);
    
    // Pipe constructor does not take Renderer, its draw method does.
    std::unique_ptr<Pipe> newPipe(new Pipe(xPos, SCREEN_HEIGHT, pipeGapY, pipeWidth, pipeGap));
    configurePipe(*newPipe); 
    gameObjects.push_back(std::move(newPipe));
}

void FlappyBirdScene::ensureObjectDensity() {
    int rightmostX = -SCREEN_WIDTH; 
    bool anyPipes = false;
    if (!gameObjects.empty()) {
        for(const auto& obj : gameObjects) { 
            if (obj->getGameObjectType() == GameObjectType::PIPE) {
                anyPipes = true;
                if (obj->getX() > rightmostX) {
                    rightmostX = obj->getX();
                }
            }
        }
    }
    int minSpacing = getMinPipeSpacing();
    int maxSpacing = getMaxPipeSpacing();
    if (maxSpacing <= minSpacing) maxSpacing = minSpacing + 5; 

    if (!anyPipes || rightmostX < SCREEN_WIDTH - minSpacing) {
        int randomSpacing = random(minSpacing, maxSpacing + 1);
        int nextPipeX = !anyPipes ? SCREEN_WIDTH + random(0,10) : rightmostX + randomSpacing; 
        addPipeObject(nextPipeX);
    }
}

void FlappyBirdScene::init(GameContext& context) { 
    GameScene::init(context); 
    if (_gameContext && _gameContext->gameStats) { 
        _highScore = _gameContext->gameStats->flappyBirdHighScore;
    }
    // The InputManager is now accessed via the context
    if (_gameContext && _gameContext->inputManager) {
        // Corrected call to registerButtonListener using abstract engine types
        _gameContext->inputManager->registerButtonListener(EDGE_Button::OK, EDGE_Event::CLICK, this, [this]() {
            if (handleDialogKeyPress(GEM_KEY_OK)) {
                return;
            }
            if (gameIsOver) {
                debugPrint("SCENES", "FlappyBird: OK Click on GameOver, signaling exit.");
                signalGameExit(false, pipePassed);
            } else {
                this->flap();
            }
        });
    }
    debugPrint("SCENES", "Flappy Bird Initialized (Derived from GameScene)");
}

void FlappyBirdScene::onEnter() {
    GameScene::onEnter(); 
    debugPrint("SCENES", "FlappyBirdScene::onEnter");
    if (_gameContext && _gameContext->gameStats) { 
        _highScore = _gameContext->gameStats->flappyBirdHighScore; 
    }
    resetGame(); 
    _wasPausedByFatigue = false;

    unsigned long currentTime = millis();
    _lastCoinSpawnTime = currentTime;
    _nextCoinSpawnInterval = random(MIN_COIN_SPAWN_INTERVAL_MS, MAX_COIN_SPAWN_INTERVAL_MS + 1);
    _lastPowerUpSpawnTime = currentTime + 5000; 
    _nextPowerUpSpawnInterval = random(MIN_POWERUP_SPAWN_INTERVAL_MS, MAX_POWERUP_SPAWN_INTERVAL_MS + 1);
    _lastEnemySpawnTime = currentTime + 2000; 
    _nextEnemySpawnInterval = random(MIN_ENEMY_SPAWN_INTERVAL_MS, MAX_ENEMY_SPAWN_INTERVAL_MS + 1);
}

void FlappyBirdScene::flap() { if (!gameIsOver && !isGamePausedByFatigue() && !isFatigueDialogActive()) birdVelocity = JUMP_FORCE; } 

void FlappyBirdScene::update(unsigned long deltaTime) {
    GameScene::update(deltaTime); 
    if (!_gameContext || !_gameContext->gameStats || !_gameContext->renderer) { 
        debugPrint("SCENES", "FlappyBirdScene FATAL: Critical context members null in update.");
        return;
    }

    if ((getDialogBox() && getDialogBox()->isActive()) || isGamePausedByFatigue()) {
        return;
    }

    if (_wasPausedByFatigue && !isGamePausedByFatigue() && !isFatigueDialogActive()) { 
        debugPrint("SCENES", "FlappyBirdScene specific resume logic (if any).");
    }
    _wasPausedByFatigue = isGamePausedByFatigue();

    unsigned long currentTime = millis();

    if (!gameIsOver) { 
        float currentGravity = gravity;
        int currentPipeSpeed = pipeSpeed;
        if (_isSlowMotionActive) {
            currentGravity *= 0.5f;
            currentPipeSpeed = std::max(1, currentPipeSpeed / 2);
        }
        birdVelocity += currentGravity;
        birdY += birdVelocity;

        for (auto& objPtr : gameObjects) { 
            objPtr->update(currentPipeSpeed, deltaTime);
            if (objPtr->getGameObjectType() == GameObjectType::PIPE) {
                 Pipe* p = static_cast<Pipe*>(objPtr.get()); 
                if (!_isGhostModeActive && p->collidesWith(birdX, birdY, BIRD_RADIUS)) {
                    if (p->isFragile() && !p->isBroken()) { 
                        p->markAsBroken(); 
                        debugPrint("SCENES", "Fragile pipe broken!");
                    } else if (!p->isFragile() || (p->isFragile() && p->isBroken())) { 
                        gameIsOver = true; 
                        debugPrint("SCENES", "Game Over: Collision with solid pipe part.");
                        if (_gameContext->gameStats) {  
                            _gameContext->gameStats->flappyBirdCoins += _sessionCoins;
                            if (pipePassed > _gameContext->gameStats->flappyBirdHighScore) {
                                _gameContext->gameStats->flappyBirdHighScore = pipePassed;
                                _highScore = pipePassed; 
                            }
                            _gameContext->gameStats->save();
                        }
                        break; 
                    }
                }
                if (p->needsScoreCheck() && !p->hasBeenScored() && birdX > p->getX() + p->getWidth()) {
                    p->markScored();
                    pipePassed++;
                    updateLevel();
                }
            }
        }
        if (gameIsOver) return; 

        gameObjects.erase(
            std::remove_if(gameObjects.begin(), gameObjects.end(),
                           [](const std::unique_ptr<GameObject>& objPtr) {
                               return objPtr->isOffScreen() || (objPtr->getGameObjectType() == GameObjectType::PIPE && static_cast<Pipe*>(objPtr.get())->isBroken()); 
                            }),
            gameObjects.end());
        ensureObjectDensity(); 

        updateCollectibles(deltaTime); 
        checkCollectibleCollisions();

        updateEnemies(deltaTime); 
        if (!_isGhostModeActive) checkEnemyCollisions();
        if (gameIsOver) return; 

        manageObjectSpawning(currentTime);

        bool outOfBounds = birdY - BIRD_RADIUS < -TOP_BOTTOM_PADDING || birdY + BIRD_RADIUS > SCREEN_HEIGHT + TOP_BOTTOM_PADDING;
        if (outOfBounds) {
            gameIsOver = true; 
            debugPrint("SCENES", "Game Over: Out of bounds.");
            if (_gameContext->gameStats) { 
                _gameContext->gameStats->flappyBirdCoins += _sessionCoins;
                if (pipePassed > _gameContext->gameStats->flappyBirdHighScore) {
                    _gameContext->gameStats->flappyBirdHighScore = pipePassed;
                    _highScore = pipePassed; 
                }
                _gameContext->gameStats->save();
            }
        }

        if ((_isGhostModeActive || _isSlowMotionActive) && currentTime >= _powerUpEndTime) {
            deactivatePowerUp();
        }
    } 
}

void FlappyBirdScene::draw(Renderer& renderer) { 
    GameScene::draw(renderer); 

    if (isFatigueDialogActive() && !gameIsOver && getDialogBox() && getDialogBox()->isActive()) {
        return; 
    }

    if (gameIsOver) {
        drawGameOverScreen(renderer); 
        return;
    }

    for (const auto& objPtr : gameObjects) { objPtr->draw(renderer); }
    for (const auto& collectiblePtr : _collectibles) { collectiblePtr->draw(renderer); }
    for (const auto& enemyPtr : _enemies) { enemyPtr->draw(renderer); }

    if (_isGhostModeActive && (millis() / 150) % 2 == 0) { 
    } else {
        renderer.drawFilledCircle(static_cast<int>(birdX), static_cast<int>(birdY), BIRD_RADIUS);
    }
    
    drawUI(renderer);
}

void FlappyBirdScene::resetGame() {
    debugPrint("SCENES", "FlappyBirdScene: Resetting game state...");
    birdY = SCREEN_HEIGHT / 2.0f; birdX = 10.0f; birdVelocity = 0;
    gameIsOver = false; pipePassed = 0; currentLevel = 1; _sessionCoins = 0;
    gravity = INITIAL_GRAVITY; pipeSpeed = 1; 
    updateLevel(); 

    gameObjects.clear();
    _collectibles.clear();
    _enemies.clear();
    
    ensureObjectDensity(); 
    deactivatePowerUp();   

    unsigned long currentTime = millis();
    _lastCoinSpawnTime = currentTime;
    _nextCoinSpawnInterval = random(MIN_COIN_SPAWN_INTERVAL_MS, MAX_COIN_SPAWN_INTERVAL_MS + 1);
    _lastPowerUpSpawnTime = currentTime + 5000; 
    _nextPowerUpSpawnInterval = random(MIN_POWERUP_SPAWN_INTERVAL_MS, MAX_POWERUP_SPAWN_INTERVAL_MS + 1);
    _lastEnemySpawnTime = currentTime + 2000; 
    _nextEnemySpawnInterval = random(MIN_ENEMY_SPAWN_INTERVAL_MS, MAX_ENEMY_SPAWN_INTERVAL_MS + 1);

    debugPrint("SCENES", "FlappyBirdScene: Game Reset Complete.");
}

void FlappyBirdScene::updateLevel() {
    int prevLevel = currentLevel;
    currentLevel = (pipePassed / 5) + 1; 
    if (currentLevel != prevLevel) {
        debugPrintf("SCENES", "Level Up! %d -> %d", prevLevel, currentLevel);
        gravity = getGravityForLevel();
        pipeSpeed = 1 + (currentLevel-1)/6; 
        pipeSpeed = std::min(pipeSpeed, 1 + 1); 
        
    } else if (prevLevel == 1 && currentLevel == 1 && pipePassed == 0) { 
        gravity = getGravityForLevel(); pipeSpeed = 1; 
    }
}

int FlappyBirdScene::getPipeGap() {
    int baseGap = 28 - currentLevel; 
    baseGap = std::max(12, baseGap); 
    return baseGap;
}
int FlappyBirdScene::getPipeWidth() {
    return 10 + (currentLevel / 2); 
}

void FlappyBirdScene::onGamePausedByFatigue() {
    GameScene::onGamePausedByFatigue(); 
    _wasPausedByFatigue = true;
    debugPrint("SCENES", "FlappyBirdScene: Paused by fatigue.");
}

void FlappyBirdScene::onGameResumedFromFatiguePause() {
    GameScene::onGameResumedFromFatiguePause(); 
    _wasPausedByFatigue = false;
    debugPrint("SCENES", "FlappyBirdScene: Resumed from fatigue pause.");
}

void FlappyBirdScene::spawnCoin() {
    if (!_gameContext || !_gameContext->renderer) return;  

    float coinX = _gameContext->renderer->getWidth() + random(2, 6); 
    float coinY; 

    int typicalGapHeight = getPipeGap(); 
    int minCoinY = TOP_BOTTOM_PADDING + COIN_SPRITE_HEIGHT;
    int maxCoinY = SCREEN_HEIGHT - TOP_BOTTOM_PADDING - COIN_SPRITE_HEIGHT - typicalGapHeight;
    if (maxCoinY <= minCoinY) maxCoinY = minCoinY + 5; 

    coinY = random(minCoinY, maxCoinY + 1) + typicalGapHeight / 2.0f - COIN_SPRITE_HEIGHT / 2.0f;
    coinY = std::max((float)TOP_BOTTOM_PADDING, std::min((float)SCREEN_HEIGHT - TOP_BOTTOM_PADDING - COIN_SPRITE_HEIGHT, coinY));
    
    _collectibles.push_back(std::unique_ptr<CollectibleObject>(new Coin(coinX, coinY)));
    debugPrintf("SCENES", "Spawned Coin at X: %.1f, Y: %.1f", coinX, coinY);
}


void FlappyBirdScene::spawnPowerUp() {
    if (!_gameContext || !_gameContext->renderer || currentLevel < LEVEL_START_POWERUPS) return; 
    float x = _gameContext->renderer->getWidth() + random(10, 30); 
    float y = random(TOP_BOTTOM_PADDING + POWERUP_SPRITE_HEIGHT, SCREEN_HEIGHT - TOP_BOTTOM_PADDING - POWERUP_SPRITE_HEIGHT * 2);
    
    CollectibleType type = (random(0,2) == 0) ? CollectibleType::POWERUP_GHOST : CollectibleType::POWERUP_SLOWMO;
    _collectibles.push_back(std::unique_ptr<CollectibleObject>(new PowerUpItem(x, y, type)));
    debugPrintf("SCENES", "Spawned PowerUp Type %d at X: %.1f, Y: %.1f", (int)type, x, y);
}

void FlappyBirdScene::updateCollectibles(unsigned long deltaTime) {
    int currentPipeSpeedAdj = _isSlowMotionActive ? std::max(1, pipeSpeed / 2) : pipeSpeed;
    for (auto& collectible : _collectibles) {
        collectible->update(currentPipeSpeedAdj, deltaTime);
    }
    _collectibles.erase(
        std::remove_if(_collectibles.begin(), _collectibles.end(),
                       [](const std::unique_ptr<CollectibleObject>& c) { return c->isOffScreen(); }),
        _collectibles.end());
}

void FlappyBirdScene::checkCollectibleCollisions() {
    auto it = _collectibles.begin();
    while (it != _collectibles.end()) {
        if ((*it)->collidesWith(birdX, birdY, BIRD_RADIUS)) {
            CollectibleType type = (*it)->getCollectibleType();
            if (type == CollectibleType::COIN) {
                _sessionCoins++;
            } else {
                activatePowerUp(type);
            }
            it = _collectibles.erase(it); 
        } else {
            ++it;
        }
    }
}

void FlappyBirdScene::activatePowerUp(CollectibleType type) {
    deactivatePowerUp(); 
    _activePowerUpType = type;
    _powerUpEndTime = millis() + POWERUP_DURATION_MS;

    if (type == CollectibleType::POWERUP_GHOST) { _isGhostModeActive = true; debugPrint("SCENES", "Ghost Mode ACTIVE!"); } 
    else if (type == CollectibleType::POWERUP_SLOWMO) { _isSlowMotionActive = true; debugPrint("SCENES", "Slow Motion ACTIVE!"); }
}

void FlappyBirdScene::deactivatePowerUp() {
    if (_activePowerUpType != CollectibleType::COIN) { }
    _activePowerUpType = CollectibleType::COIN; 
    _isGhostModeActive = false; _isSlowMotionActive = false; _powerUpEndTime = 0;
}

void FlappyBirdScene::spawnFlyingEnemy() {
    if (!_gameContext || !_gameContext->renderer || currentLevel < LEVEL_START_FLYING_ENEMIES) return; 
    float x = _gameContext->renderer->getWidth() + random(5, 15); 
    float y = random(TOP_BOTTOM_PADDING + ENEMY_BIRD_SPRITE_HEIGHT, SCREEN_HEIGHT - TOP_BOTTOM_PADDING - ENEMY_BIRD_SPRITE_HEIGHT * 2);
    float speedX = - ( (float)random(6,12) / 10.0f ) - (currentLevel / 10.0f * 0.2f); 
    speedX = std::max(-1.8f, speedX); 

    _enemies.push_back(std::unique_ptr<EnemyObject>(new FlyingEnemy(x, y, speedX)));
    debugPrintf("SCENES", "Spawned Flying Enemy at X: %.1f, Y: %.1f, VX: %.1f", x, y, speedX);
}

void FlappyBirdScene::updateEnemies(unsigned long deltaTime) {
    int currentScreenSpeedAdj = _isSlowMotionActive ? std::max(1, pipeSpeed / 2) : pipeSpeed;
    for (auto& enemy : _enemies) {
        enemy->update(currentScreenSpeedAdj, deltaTime); 
    }
    _enemies.erase(
        std::remove_if(_enemies.begin(), _enemies.end(),
                       [](const std::unique_ptr<EnemyObject>& e) { return e->isOffScreen(); }),
        _enemies.end());
}

void FlappyBirdScene::checkEnemyCollisions() {
    if (gameIsOver) return;
    for (const auto& enemy : _enemies) {
        if (enemy->collidesWith(birdX, birdY, BIRD_RADIUS)) {
            gameIsOver = true; 
            debugPrint("SCENES", "Game Over: Collision with enemy.");
             if (_gameContext && _gameContext->gameStats) { 
                _gameContext->gameStats->flappyBirdCoins += _sessionCoins;
                if (pipePassed > _gameContext->gameStats->flappyBirdHighScore) {
                    _gameContext->gameStats->flappyBirdHighScore = pipePassed;
                    _highScore = pipePassed; 
                }
                _gameContext->gameStats->save();
            }
            break;
        }
    }
}

void FlappyBirdScene::manageObjectSpawning(unsigned long currentTime) {
    if (currentLevel >= LEVEL_START_COINS && currentTime >= _lastCoinSpawnTime + _nextCoinSpawnInterval) {
        int spawnChance = std::max(25, 75 - (currentLevel * 2)); // Increased base chance, minimum 25%
        if (random(100) < spawnChance) { 
            spawnCoin();
        }
        _lastCoinSpawnTime = currentTime;
        _nextCoinSpawnInterval = random(MIN_COIN_SPAWN_INTERVAL_MS - (currentLevel*50), MAX_COIN_SPAWN_INTERVAL_MS - (currentLevel*100) + 1);
        if (_nextCoinSpawnInterval < 500) _nextCoinSpawnInterval = 500;
    }
    if (currentLevel >= LEVEL_START_POWERUPS && currentTime >= _lastPowerUpSpawnTime + _nextPowerUpSpawnInterval) {
        if (random(100) < 10 + currentLevel) { spawnPowerUp(); }
        _lastPowerUpSpawnTime = currentTime;
        _nextPowerUpSpawnInterval = random(MIN_POWERUP_SPAWN_INTERVAL_MS - (currentLevel*200), MAX_POWERUP_SPAWN_INTERVAL_MS - (currentLevel*300) + 1);
        if (_nextPowerUpSpawnInterval < 4000) _nextPowerUpSpawnInterval = 4000;
    }
    if (currentLevel >= LEVEL_START_FLYING_ENEMIES && currentTime >= _lastEnemySpawnTime + _nextEnemySpawnInterval) { 
        if (random(100) < 15 + (currentLevel * 2.5f)) { spawnFlyingEnemy(); }
        _lastEnemySpawnTime = currentTime;
        _nextEnemySpawnInterval = random(MIN_ENEMY_SPAWN_INTERVAL_MS - (currentLevel*150), MAX_ENEMY_SPAWN_INTERVAL_MS - (currentLevel*250) + 1);
         if (_nextEnemySpawnInterval < 2000) _nextEnemySpawnInterval = 2000;
    }
}

void FlappyBirdScene::drawUI(Renderer& renderer) { 
    renderer.setFont(u8g2_font_5x7_tf); 
    char buffer[20];
    snprintf(buffer, sizeof(buffer), "%d", pipePassed);
    renderer.drawTextSafe(SCREEN_WIDTH - (strlen(buffer) * 5) - 2, 1, buffer); 
    snprintf(buffer, sizeof(buffer), "L:%d", currentLevel);
    renderer.drawTextSafe(2, 1, buffer);
    snprintf(buffer, sizeof(buffer), "C:%u", _sessionCoins);
    renderer.drawTextSafe(2, SCREEN_HEIGHT - 8, buffer);
    snprintf(buffer, sizeof(buffer), "H:%u", _highScore);
    renderer.drawTextSafe(SCREEN_WIDTH - (strlen(buffer) * 5) - 2, SCREEN_HEIGHT - 8, buffer);

    if (_isGhostModeActive || _isSlowMotionActive) {
        unsigned long timeLeft = (_powerUpEndTime > millis()) ? (_powerUpEndTime - millis()) / 1000 : 0;
        const char* pType = _isGhostModeActive ? "G" : "S";
        snprintf(buffer, sizeof(buffer), "%s:%lu", pType, timeLeft);
        renderer.drawTextSafe(SCREEN_WIDTH / 2 - (strlen(buffer) * 5 / 2), 1, buffer);
    }
}

void FlappyBirdScene::drawGameOverScreen(Renderer& renderer) { 
    renderer.setFont(u8g2_font_6x10_tf); 
    int y_pos = SCREEN_HEIGHT / 2 - 24; 
    char buffer[30];
    snprintf(buffer, sizeof(buffer), "Game Over!");
    renderer.drawTextSafe(SCREEN_WIDTH/2 - (strlen(buffer)*6/2), y_pos, buffer); y_pos += 10;
    snprintf(buffer, sizeof(buffer), "Level: %d", currentLevel);
    renderer.drawTextSafe(SCREEN_WIDTH/2 - (strlen(buffer)*6/2), y_pos, buffer); y_pos += 10;
    snprintf(buffer, sizeof(buffer), "Score: %d", pipePassed);
    renderer.drawTextSafe(SCREEN_WIDTH/2 - (strlen(buffer)*6/2), y_pos, buffer); y_pos += 10;
    snprintf(buffer, sizeof(buffer), "Coins: %u", _sessionCoins);
    renderer.drawTextSafe(SCREEN_WIDTH/2 - (strlen(buffer)*6/2), y_pos, buffer); y_pos += 10;
    snprintf(buffer, sizeof(buffer), "Best: %u", _highScore);
    renderer.drawTextSafe(SCREEN_WIDTH/2 - (strlen(buffer)*6/2), y_pos, buffer);
}