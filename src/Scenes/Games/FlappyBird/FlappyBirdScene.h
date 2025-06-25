#pragma once

#include <vector>
#include <memory> 
#include "../GameScene.h" 
#include "GameObject.h"
#include "FlappyBirdGraphics.h" 
#include "CollectibleObject.h"  
#include "EnemyObject.h"        
#include "../../../System/GameContext.h" 

// Constants (can remain here or be moved if shared)
#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
#define BIRD_RADIUS 2

// Forward declare specific game object types
class Pipe;
class Coin;
class PowerUpItem;
class FlyingEnemy;


class FlappyBirdScene : public GameScene { 
public:
    FlappyBirdScene(); 
    // init is not overriding the base Scene::init(), it's a specific initializer for this scene.
    void init(GameContext& context); 
    void onEnter() override; 
    void update(unsigned long deltaTime) override;
    void draw(Renderer& renderer) override;

    SceneType getSceneType() const override { return SceneType::FLAPPY_GAME; }

    void onGamePausedByFatigue() override;
    void onGameResumedFromFatiguePause() override;


private:
    float birdY, birdX, birdVelocity;
    float gravity; 
    bool gameIsOver; 
    int pipePassed, currentLevel, pipeSpeed;
    uint32_t _sessionCoins; 
    uint32_t _highScore;    

    std::vector<std::unique_ptr<GameObject>> gameObjects; 
    std::vector<std::unique_ptr<CollectibleObject>> _collectibles; 
    std::vector<std::unique_ptr<EnemyObject>> _enemies; 

    CollectibleType _activePowerUpType = CollectibleType::COIN; 
    unsigned long _powerUpEndTime = 0;
    bool _isGhostModeActive = false;
    bool _isSlowMotionActive = false;
    static const unsigned long POWERUP_DURATION_MS = 5000; 

    static const int LEVEL_START_COINS = 0;
    static const int LEVEL_START_MOVING_PIPES = 0;
    static const int LEVEL_START_POWERUPS = 12;
    static const int LEVEL_START_GAP_CHANGING_PIPES = 10;
    static const int LEVEL_START_FLYING_ENEMIES = 18;
    static const int LEVEL_START_FRAGILE_PIPES = 14;
    static const int LEVEL_START_PIPE_HAZARDS = 20;

    static const int MOVING_PIPE_BASE_CHANCE_PERCENT = 5;      
    static constexpr float MOVING_PIPE_CHANCE_SCALAR_PER_LEVEL = 1.0f; 
    static const int MOVING_PIPE_MAX_CHANCE_PERCENT = 40;     

    static constexpr float MOVING_PIPE_BASE_SPEED = 0.015f;     
    static constexpr float MOVING_PIPE_SPEED_SCALAR_PER_LEVEL_DIVISOR = 25.0f; 
    static constexpr float MOVING_PIPE_SPEED_INCREMENT_AMOUNT = 0.005f; 
    static constexpr float MOVING_PIPE_MAX_SPEED = 0.06f;      

    void resetGame();
    void updateLevel();
    int getPipeGap();  
    int getPipeWidth();
    int getMinPipeSpacing();
    int getMaxPipeSpacing();
    float getGravityForLevel();
    void flap();
    
    void addPipeObject(int xPos);
    void configurePipe(Pipe& pipe); 

    void spawnCoin();
    void spawnPowerUp();
    void updateCollectibles(unsigned long deltaTime);
    void checkCollectibleCollisions();
    void activatePowerUp(CollectibleType type);
    void deactivatePowerUp();

    void spawnFlyingEnemy();
    void updateEnemies(unsigned long deltaTime);
    void checkEnemyCollisions();
    
    void ensureObjectDensity(); 
    void manageObjectSpawning(unsigned long currentTime); 

    void drawUI(Renderer& renderer);
    void drawGameOverScreen(Renderer& renderer);

    unsigned long _lastCoinSpawnTime = 0;
    unsigned long _nextCoinSpawnInterval = 0;
    unsigned long _lastPowerUpSpawnTime = 0;
    unsigned long _nextPowerUpSpawnInterval = 0;
    unsigned long _lastEnemySpawnTime = 0;
    unsigned long _nextEnemySpawnInterval = 0;

    static const unsigned long MIN_COIN_SPAWN_INTERVAL_MS = 2000;
    static const unsigned long MAX_COIN_SPAWN_INTERVAL_MS = 5000;
    static const unsigned long MIN_POWERUP_SPAWN_INTERVAL_MS = 10000;
    static const unsigned long MAX_POWERUP_SPAWN_INTERVAL_MS = 20000;
    static const unsigned long MIN_ENEMY_SPAWN_INTERVAL_MS = 4000;
    static const unsigned long MAX_ENEMY_SPAWN_INTERVAL_MS = 8000;

    bool _wasPausedByFatigue = false; 
};