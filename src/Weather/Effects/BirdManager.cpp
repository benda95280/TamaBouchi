#include "BirdManager.h"
#include <U8g2lib.h>
#include <algorithm>
#include <cmath>
#include <iterator>

// 5px width, 6px total height, 2 frames of 5x3 each
const unsigned char epd_bitmap_New_Piskel_Birds[] PROGMEM = {
    0x18, 0x30, 0xf8, // Frame 1 (3 bytes)
    0x00, 0x00, 0xf8  // Frame 2 (3 bytes)
};

BirdManager::BirdManager(Renderer &renderer)
    : _renderer(renderer)
{
    _birdBitmap = epd_bitmap_New_Piskel_Birds;
    debugPrint("WEATHER", "BirdManager created.");
}

void BirdManager::init(unsigned long currentTime)
{
    std::for_each(std::begin(_birds), std::end(_birds), [](Bird &b)
                  { b.active = false; });
    _lastBirdSpawnCheckTime = currentTime;
    _nextBirdSpawnInterval = random(MIN_BIRD_SPAWN_INTERVAL_MS, MAX_BIRD_SPAWN_INTERVAL_MS + 1);
    _isActive = false; // Start inactive, WeatherManager will activate
    debugPrint("WEATHER", "BirdManager initialized.");
}

void BirdManager::setActive(bool active, const char* reason)
{
    if (_isActive == active)
        return;
    _isActive = active;
    
    if (reason && strlen(reason) > 0) {
        debugPrintf("WEATHER", "BirdManager active state set to: %s. Reason: %s", _isActive ? "true" : "false", reason);
    } else {
        debugPrintf("WEATHER", "BirdManager active state set to: %s", _isActive ? "true" : "false");
    }

    if (!_isActive)
    { // If becoming inactive, clear existing birds
        std::for_each(std::begin(_birds), std::end(_birds), [](Bird &b)
                      { b.active = false; });
    }
    else
    {
        // Reset spawn timer when activated to potentially spawn birds sooner
        _lastBirdSpawnCheckTime = millis();
        _nextBirdSpawnInterval = random(MIN_BIRD_SPAWN_INTERVAL_MS, MAX_BIRD_SPAWN_INTERVAL_MS + 1) / 2; // Shorter initial interval
    }
}

void BirdManager::setWindFactor(float windFactor)
{
    _currentWindFactor = windFactor;
}

void BirdManager::spawnBirds(unsigned long currentTime)
{
    int birdsToSpawnThisTime = random(MIN_BIRDS_TO_SPAWN, MAX_BIRDS_TO_SPAWN + 1);
    int spawnedCount = 0;

    for (int i = 0; i < MAX_BIRDS && spawnedCount < birdsToSpawnThisTime; ++i)
    {
        if (!_birds[i].active)
        {
            resetBird(_birds[i], true);                                                      // Pass true for initial spawn
            _birds[i].lastFrameChangeTime = currentTime + random(0, BIRD_FRAME_DURATION_MS); // Stagger animation start
            spawnedCount++;
        }
    }
    if (spawnedCount > 0)
        debugPrintf("WEATHER", "BirdManager: Spawned %d birds.", spawnedCount);
}

void BirdManager::spawnBirdsOnDemand(int count) {
    if (!_isActive) {
        setActive(true, "On-demand spawn command");
    }
    int spawnedCount = 0;
    unsigned long currentTime = millis();
    for (int i = 0; i < MAX_BIRDS && spawnedCount < count; ++i) {
        if (!_birds[i].active) {
            resetBird(_birds[i], true); // true for initial spawn logic
            _birds[i].lastFrameChangeTime = currentTime + random(0, BIRD_FRAME_DURATION_MS);
            spawnedCount++;
        }
    }
    if (spawnedCount > 0) {
        debugPrintf("WEATHER", "BirdManager: Spawned %d birds on demand.", spawnedCount);
    }
}

void BirdManager::resetBird(Bird &bird, bool initialSpawn)
{
    bird.active = true;
    bird.baseY = (float)random(0, BIRD_SPAWN_Y_MAX + 1);
    bird.currentY = bird.baseY;
    bird.x = (float)(_renderer.getWidth() + random(5, BIRD_SPRITE_WIDTH * 2));

    float speedMultiplier = (float)random((int)(MIN_BIRD_SPEED_X_FACTOR * 100), (int)(MAX_BIRD_SPEED_X_FACTOR * 100) + 1) / 100.0f;
    bird.speedX = BASE_BIRD_SPEED_X * speedMultiplier;

    bird.currentFrame = random(0, BIRD_FRAME_COUNT);
    bird.lastFrameChangeTime = millis();

    // New properties for size and wavy movement
    bird.isBig = (random(100) < 30);
    bird.targetYOffset = 0.0f;
    bird.waveTransitionStartTime = millis();
    bird.nextWaveChangeTime = millis() + random(MIN_WAVE_INTERVAL_MS, MAX_WAVE_INTERVAL_MS + 1);
}

void BirdManager::update(unsigned long currentTime)
{
    if (!_isActive)
    {
        return;
    }

    if (currentTime - _lastBirdSpawnCheckTime >= _nextBirdSpawnInterval)
    {
        spawnBirds(currentTime);
        _lastBirdSpawnCheckTime = currentTime;
        _nextBirdSpawnInterval = random(MIN_BIRD_SPAWN_INTERVAL_MS, MAX_BIRD_SPAWN_INTERVAL_MS + 1);
    }

    std::for_each(std::begin(_birds), std::end(_birds), [&](Bird &bird)
                  {
        if (bird.active) {
            bird.x += bird.speedX;

            // Wavy movement logic
            if (currentTime >= bird.nextWaveChangeTime) {
                // Toggle target offset between +amplitude and -amplitude
                if (bird.targetYOffset == 0.0f) {
                    bird.targetYOffset = (random(0, 2) == 0) ? (float)BIRD_WAVE_AMPLITUDE : (float)-BIRD_WAVE_AMPLITUDE;
                } else {
                    bird.targetYOffset *= -1; // Reverse direction
                }
                bird.waveTransitionStartTime = currentTime;
                bird.nextWaveChangeTime = currentTime + random(MIN_WAVE_INTERVAL_MS, MAX_WAVE_INTERVAL_MS + 1);
            }

            // Smooth transition to targetY using easing
            float targetY = bird.baseY + bird.targetYOffset;
            bird.currentY += (targetY - bird.currentY) * 0.05f;

            // Wind affect on current Y position
            bird.currentY += _currentWindFactor * 0.05f;
            if (bird.currentY < 0) bird.currentY = 0;
            if (bird.currentY > BIRD_SPAWN_Y_MAX + 2) bird.currentY = BIRD_SPAWN_Y_MAX + 2;

            if (currentTime - bird.lastFrameChangeTime >= BIRD_FRAME_DURATION_MS) {
                bird.currentFrame = (bird.currentFrame + 1) % BIRD_FRAME_COUNT;
                bird.lastFrameChangeTime = currentTime;
            }

            if (bird.x < -BIRD_SPRITE_WIDTH - 5) {
                bird.active = false;
            }
        } });
}

void BirdManager::draw()
{
    if (!_isActive)
    {
        return;
    }

    U8G2 *u8g2 = _renderer.getU8G2();
    if (!u8g2 || !_birdBitmap)
        return;

    uint8_t originalDrawColor = u8g2->getDrawColor();

    u8g2->setDrawColor(1);
    u8g2->setBitmapMode(0);

    std::for_each(std::begin(_birds), std::end(_birds), [&](const Bird &bird)
                  {
        if (bird.active) {
            if (bird.isBig) {
                drawScaledBird(bird, 1.5f);
            } else {
                int drawX_actual = static_cast<int>(round(bird.x));
                int drawY_actual = static_cast<int>(round(bird.currentY));
                
                const unsigned char* currentFramePtr = _birdBitmap + (bird.currentFrame * BIRD_BYTES_PER_FRAME);
                
                _renderer.getU8G2()->drawXBMP(
                    _renderer.getXOffset() + drawX_actual, 
                    _renderer.getYOffset() + drawY_actual, 
                    BIRD_SPRITE_WIDTH, 
                    BIRD_SPRITE_HEIGHT, 
                    currentFramePtr
                );
            }
        } });

    u8g2->setDrawColor(originalDrawColor);
}

void BirdManager::drawScaledBird(const Bird& bird, float scale) {
    U8G2* u8g2 = _renderer.getU8G2();
    if (!u8g2 || !_birdBitmap) return;

    int scaledW = static_cast<int>(round(BIRD_SPRITE_WIDTH * scale));
    int scaledH = static_cast<int>(round(BIRD_SPRITE_HEIGHT * scale));
    if (scaledW <= 0 || scaledH <= 0) return;

    int drawX = _renderer.getXOffset() + static_cast<int>(round(bird.x)) - (scaledW - BIRD_SPRITE_WIDTH) / 2;
    int drawY = _renderer.getYOffset() + static_cast<int>(round(bird.currentY)) - (scaledH - BIRD_SPRITE_HEIGHT) / 2;
    
    const unsigned char* framePtr = _birdBitmap + (bird.currentFrame * BIRD_BYTES_PER_FRAME);

    for (int destY = 0; destY < scaledH; ++destY) {
        for (int destX = 0; destX < scaledW; ++destX) {
            int srcX = static_cast<int>(floor(static_cast<float>(destX) / scale));
            int srcY = static_cast<int>(floor(static_cast<float>(destY) / scale));

            if (srcX >= 0 && srcX < BIRD_SPRITE_WIDTH && srcY >= 0 && srcY < BIRD_SPRITE_HEIGHT) {
                const unsigned char* bytePtr = framePtr + (srcY * ((BIRD_SPRITE_WIDTH + 7) / 8)) + (srcX / 8);
                unsigned char byteVal = pgm_read_byte(bytePtr);
                 if ((byteVal >> (srcX % 8)) & 0x01) {
                    u8g2->drawPixel(drawX + destX, drawY + destY);
                }
            }
        }
    }
}