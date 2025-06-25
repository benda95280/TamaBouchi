#ifndef BIRD_MANAGER_H
#define BIRD_MANAGER_H

#include <Arduino.h>
#include <vector>
#include "Renderer.h" // Adjusted path
#include "../../DebugUtils.h"   // Using debug system instead of SerialForwarder

// Forward declaration
class U8G2;

class BirdManager {
public:
    BirdManager(Renderer& renderer); // logger parameter kept for backward compatibility
    ~BirdManager() = default;

    void init(unsigned long currentTime);
    void update(unsigned long currentTime);
    void draw(); // Drawing will be relative to renderer offsets
    void setActive(bool active);
    void setWindFactor(float windFactor);

private:
    struct Bird {
        bool active;
        float x, y;
        float speedX;
        int currentFrame;
        unsigned long lastFrameChangeTime;
    };

    Renderer& _renderer;
    // Removed SerialForwarder logger - using debug system instead

    static const int MAX_BIRDS = 5;
    Bird _birds[MAX_BIRDS];
    bool _isActive = false;
    float _currentWindFactor = 0.0f;

    unsigned long _lastBirdSpawnCheckTime = 0;
    unsigned long _nextBirdSpawnInterval = 0;
    
    // Bird sprite properties - UPDATED
    static const int BIRD_SPRITE_WIDTH = 5;
    static const int BIRD_SPRITE_HEIGHT = 3; // Height of a single frame
    static const int BIRD_FRAME_COUNT = 2;
    static const unsigned long BIRD_FRAME_DURATION_MS = 250; 
    static const size_t BIRD_BYTES_PER_FRAME = ((BIRD_SPRITE_WIDTH + 7) / 8 * BIRD_SPRITE_HEIGHT); // Recalculates to 1 * 3 = 3 bytes

    static const unsigned long MIN_BIRD_SPAWN_INTERVAL_MS = 30000; 
    static const unsigned long MAX_BIRD_SPAWN_INTERVAL_MS = 90000; 
    static const int MIN_BIRDS_TO_SPAWN = 1;
    static const int MAX_BIRDS_TO_SPAWN = 5;
    static const int BIRD_SPAWN_Y_MAX = 20; 
    static constexpr float BASE_BIRD_SPEED_X = -0.7f; 
    static constexpr float MIN_BIRD_SPEED_X_FACTOR = 0.8f;
    static constexpr float MAX_BIRD_SPEED_X_FACTOR = 1.2f;


    const unsigned char* _birdBitmap = nullptr; 

    void spawnBirds(unsigned long currentTime);
    void resetBird(Bird& bird, bool initialSpawn = false);
};

#endif // BIRD_MANAGER_H