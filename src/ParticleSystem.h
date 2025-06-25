// --- START OF FILE src/ParticleSystem.h ---
#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H

#include <vector>
#include <Arduino.h>
#include "Renderer.h"          // Needs Renderer
#include <U8g2lib.h>           // Needs U8g2
#include "Helper/GraphicAssetTypes.h" // <<< MODIFIED: Include new path for GraphicAssetData

// Maximum number of concurrent particles
#define MAX_PARTICLES 50

// Enum defining what the particle looks like
enum class ParticleVisualType {
    CHARACTER,
    STATIC_BITMAP,
    ANIMATED_SPRITESHEET
};

// Represents a single particle
struct Particle {
    bool active = false;

    // Physics
    float x = 0.0f;
    float y = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;

    // Lifetime
    unsigned long spawnTime = 0;
    unsigned long lifetimeMs = 0;

    // Visuals
    ParticleVisualType visualType = ParticleVisualType::CHARACTER;
    uint8_t drawColor = 1; // 1=Normal, 2=XOR

    // --- Visual Data Storage ---
    // Character specific
    char character = ' ';
    const uint8_t* font = nullptr;

    // Bitmap/Spritesheet specific (Use pointer to shared data)
    const GraphicAssetData* assetData = nullptr;

    // Spritesheet state
    int currentFrame = 0;
    unsigned long lastFrameUpdateTime = 0;
    // --- End Visual Data ---
};


class ParticleSystem {
public:
    // Constructor now takes the default font to restore to
    ParticleSystem(Renderer& renderer, const uint8_t* defaultFont);
    ~ParticleSystem() = default;

    void update(unsigned long currentTime, float deltaTime);
    void draw();
    void reset();

    // --- Spawning Functions ---
    bool spawnParticle(float x, float y, float vx, float vy, unsigned long lifetimeMs,
                       char character, const uint8_t* font, uint8_t drawColor = 1);
    bool spawnParticle(float x, float y, float vx, float vy, unsigned long lifetimeMs,
                       const GraphicAssetData& asset, uint8_t drawColor = 1);

    // --- NEW: Helper for Absorb Effect ---
    // Spawns multiple character particles directed towards a target point.
    // 'size' parameter influences particle count.
    void spawnAbsorbEffect(float targetX, float targetY, float sourceX, float sourceY, int count, int size);


private:
    Renderer& _renderer;
    Particle _particlePool[MAX_PARTICLES];
    const uint8_t* _defaultFont; // <-- Store the default font

    int findInactiveParticle();
};

#endif // PARTICLE_SYSTEM_H
// --- END OF FILE src/ParticleSystem.h ---