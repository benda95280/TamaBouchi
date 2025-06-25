#include "BirdManager.h"
#include <U8g2lib.h> 
#include <algorithm> 
#include <cmath>     

// 5px width, 6px total height, 2 frames of 5x3 each
const unsigned char epd_bitmap_New_Piskel_Birds [] PROGMEM = {
	0x18, 0x30, 0xf8, // Frame 1 (3 bytes)
    0x00, 0x00, 0xf8  // Frame 2 (3 bytes)
};


BirdManager::BirdManager(Renderer& renderer) 
    : _renderer(renderer) {
    _birdBitmap = epd_bitmap_New_Piskel_Birds; 
    debugPrint("WEATHER", "BirdManager created.");
}

void BirdManager::init(unsigned long currentTime) {
    for (int i = 0; i < MAX_BIRDS; ++i) {
        _birds[i].active = false;
    }
    _lastBirdSpawnCheckTime = currentTime;
    _nextBirdSpawnInterval = random(MIN_BIRD_SPAWN_INTERVAL_MS, MAX_BIRD_SPAWN_INTERVAL_MS + 1);
    _isActive = false; // Start inactive, WeatherManager will activate
    debugPrint("WEATHER", "BirdManager initialized.");
}

void BirdManager::setActive(bool active) {
    if (_isActive == active) return;
    _isActive = active;
    debugPrintf("WEATHER", "BirdManager active state set to: %s", _isActive ? "true" : "false");
    if (!_isActive) { // If becoming inactive, clear existing birds
        for (int i = 0; i < MAX_BIRDS; ++i) {
            _birds[i].active = false;
        }
    } else {
        // Reset spawn timer when activated to potentially spawn birds sooner
        _lastBirdSpawnCheckTime = millis();
        _nextBirdSpawnInterval = random(MIN_BIRD_SPAWN_INTERVAL_MS, MAX_BIRD_SPAWN_INTERVAL_MS + 1) / 2; // Shorter initial interval
    }
}

void BirdManager::setWindFactor(float windFactor) {
    _currentWindFactor = windFactor;
    // Wind factor could adjust bird speed if desired:
    // e.g., birds flying against strong wind are slower, with strong wind are faster.
    // For now, wind factor is not directly used for bird speed.
}


void BirdManager::spawnBirds(unsigned long currentTime) {
    int birdsToSpawnThisTime = random(MIN_BIRDS_TO_SPAWN, MAX_BIRDS_TO_SPAWN + 1);
    int spawnedCount = 0;
    for (int i = 0; i < MAX_BIRDS && spawnedCount < birdsToSpawnThisTime; ++i) {
        if (!_birds[i].active) {
            resetBird(_birds[i], true); // Pass true for initial spawn
            _birds[i].lastFrameChangeTime = currentTime + random(0, BIRD_FRAME_DURATION_MS); // Stagger animation start
            spawnedCount++;
        }
    }
    if (spawnedCount > 0) debugPrintf("WEATHER", "BirdManager: Spawned %d birds.", spawnedCount);
}

void BirdManager::resetBird(Bird& bird, bool initialSpawn) {
    bird.active = true;
    bird.y = (float)random(0, BIRD_SPAWN_Y_MAX + 1); 
    // Ensure birds spawn off-screen right
    bird.x = (float)(_renderer.getWidth() + random(5, BIRD_SPRITE_WIDTH * 2)); 
    
    float speedMultiplier = (float)random((int)(MIN_BIRD_SPEED_X_FACTOR * 100), (int)(MAX_BIRD_SPEED_X_FACTOR * 100) + 1) / 100.0f;
    bird.speedX = BASE_BIRD_SPEED_X * speedMultiplier;

    // Apply wind effect subtly if desired:
    // bird.speedX += _currentWindFactor * 0.1f; // Example: wind pushes birds slightly

    bird.currentFrame = random(0, BIRD_FRAME_COUNT);
    bird.lastFrameChangeTime = millis(); // Reset frame time on spawn
}


void BirdManager::update(unsigned long currentTime) {
    if (!_isActive) {
        return;
    }

    // Check if it's time to attempt spawning new birds
    if (currentTime - _lastBirdSpawnCheckTime >= _nextBirdSpawnInterval) {
        spawnBirds(currentTime);
        _lastBirdSpawnCheckTime = currentTime;
        _nextBirdSpawnInterval = random(MIN_BIRD_SPAWN_INTERVAL_MS, MAX_BIRD_SPAWN_INTERVAL_MS + 1);
    }

    // Update active birds
    for (int i = 0; i < MAX_BIRDS; ++i) {
        if (_birds[i].active) {
            _birds[i].x += _birds[i].speedX;
            // Apply a slight vertical bobbing or drift based on wind if desired
             _birds[i].y += _currentWindFactor * 0.05f; // Very subtle vertical drift with wind
             if (_birds[i].y < 0) _birds[i].y = 0;
             if (_birds[i].y > BIRD_SPAWN_Y_MAX + 2) _birds[i].y = BIRD_SPAWN_Y_MAX + 2;


            // Animation frame update
            if (currentTime - _birds[i].lastFrameChangeTime >= BIRD_FRAME_DURATION_MS) {
                _birds[i].currentFrame = (_birds[i].currentFrame + 1) % BIRD_FRAME_COUNT;
                _birds[i].lastFrameChangeTime = currentTime;
            }

            // Deactivate if off-screen left
            if (_birds[i].x < -BIRD_SPRITE_WIDTH - 5) { // Add a small buffer
                _birds[i].active = false;
            }
        }
    }
}

void BirdManager::draw() {
    if (!_isActive) {
        return;
    }

    U8G2* u8g2 = _renderer.getU8G2();
    if (!u8g2 || !_birdBitmap) return;

    uint8_t originalDrawColor = u8g2->getDrawColor();
    // uint8_t originalBitmapMode = u8g2->getBitmapMode(); // U8G2 does not have getBitmapMode

    u8g2->setDrawColor(1);   // Birds are drawn normally (not XOR)
    u8g2->setBitmapMode(0);  // Transparent background for XBMP

    for (int i = 0; i < MAX_BIRDS; ++i) {
        if (_birds[i].active) {
            const Bird& bird = _birds[i];
            int drawX_actual = static_cast<int>(round(bird.x));
            int drawY_actual = static_cast<int>(round(bird.y));

            // Get the correct frame from the spritesheet
            // Sprites are stacked vertically
            const unsigned char* currentFramePtr = _birdBitmap + (bird.currentFrame * BIRD_BYTES_PER_FRAME);
            // Debug code for bitmap offset calculation
            // if (i==0 && bird.currentFrame==1) debugPrintf("WEATHER", "Bird 0, Frame 1, Offset: %d", (int)(currentFramePtr - _birdBitmap));
            
            _renderer.getU8G2()->drawXBMP(
                _renderer.getXOffset() + drawX_actual, 
                _renderer.getYOffset() + drawY_actual, 
                BIRD_SPRITE_WIDTH, 
                BIRD_SPRITE_HEIGHT, 
                currentFramePtr
            );
        }
    }
    u8g2->setDrawColor(originalDrawColor);
    // u8g2->setBitmapMode(originalBitmapMode); // Cannot restore, set to a known default if needed.
                                             // For XBMP, 0 (transparent) is usually desired.
}