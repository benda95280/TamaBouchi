// --- START OF FILE src/ParticleSystem.cpp ---
#include "ParticleSystem.h"
#include <cmath> // For round
#include "DebugUtils.h"

// Forward declare if needed
// extern SerialForwarder* forwardedSerial_ptr;

ParticleSystem::ParticleSystem(Renderer& renderer, const uint8_t* defaultFont)
    : _renderer(renderer), _defaultFont(defaultFont)
{
    // Pool is initialized by Particle default constructor (active=false)
}

int ParticleSystem::findInactiveParticle() {
    for (int i = 0; i < MAX_PARTICLES; ++i) { if (!_particlePool[i].active) { return i; } } return -1;
}

void ParticleSystem::reset() {
    for (int i = 0; i < MAX_PARTICLES; ++i) { _particlePool[i].active = false; }
}

bool ParticleSystem::spawnParticle(float x, float y, float vx, float vy, unsigned long lifetimeMs,
                                   char character, const uint8_t* font, uint8_t drawColor) {
    int index = findInactiveParticle(); if (index == -1) return false;
    Particle& p = _particlePool[index]; p.active = true; p.x = x; p.y = y; p.vx = vx; p.vy = vy; p.spawnTime = millis(); p.lifetimeMs = lifetimeMs; p.visualType = ParticleVisualType::CHARACTER; p.character = character; p.font = font; p.drawColor = drawColor; p.assetData = nullptr; p.currentFrame = 0; p.lastFrameUpdateTime = 0;
    return true;
}

bool ParticleSystem::spawnParticle(float x, float y, float vx, float vy, unsigned long lifetimeMs,
                                   const GraphicAssetData& asset, uint8_t drawColor) {
    if (!asset.isValid()) return false; int index = findInactiveParticle(); if (index == -1) return false;
    Particle& p = _particlePool[index]; p.active = true; p.x = x; p.y = y; p.vx = vx; p.vy = vy; p.spawnTime = millis(); p.lifetimeMs = lifetimeMs; p.drawColor = drawColor; p.assetData = &asset; p.character = ' '; p.font = nullptr;
    if (asset.isSpritesheet()) { p.visualType = ParticleVisualType::ANIMATED_SPRITESHEET; p.currentFrame = 0; p.lastFrameUpdateTime = p.spawnTime; }
    else { p.visualType = ParticleVisualType::STATIC_BITMAP; p.currentFrame = 0; p.lastFrameUpdateTime = 0; }
    return true;
}

// --- NEW: Absorb Effect Implementation ---
void ParticleSystem::spawnAbsorbEffect(float targetX, float targetY, float sourceX, float sourceY, int count, int size) {
    int numParticles = count + size * 2; // Base count + bonus for size
    float angleToTarget = atan2(targetY - sourceY, targetX - sourceX);

    for (int i = 0; i < numParticles; ++i) {
        // Angle spread around the direction towards the target
        float angle = angleToTarget + random(-45, 46) * PI / 180.0f;
        float speed = 0.7f + (random(0, 51) / 100.0f); // Speed range
        float vx = cos(angle) * speed;
        float vy = sin(angle) * speed;
        unsigned long lifetime = 180 + random(0, 121); // Lifetime range

        // Use a tiny font for absorb effect particles
        spawnParticle(sourceX, sourceY, vx, vy, lifetime, '.', u8g2_font_3x5im_te, 1);
    }
}
// --- End Absorb Effect ---


void ParticleSystem::update(unsigned long currentTime, float deltaTime) {
    // (deltaTime parameter is currently unused but kept for potential future physics)
    for (int i = 0; i < MAX_PARTICLES; ++i) {
        if (!_particlePool[i].active) continue; Particle& p = _particlePool[i];
        if (currentTime - p.spawnTime >= p.lifetimeMs) { p.active = false; continue; }
        p.x += p.vx; p.y += p.vy;
        if (p.visualType == ParticleVisualType::ANIMATED_SPRITESHEET && p.assetData && p.assetData->isSpritesheet()) { if (currentTime - p.lastFrameUpdateTime >= p.assetData->frameDurationMs) { p.lastFrameUpdateTime += p.assetData->frameDurationMs; p.currentFrame++; if (p.currentFrame >= p.assetData->frameCount) { p.currentFrame = 0; } } }
    }
}

void ParticleSystem::draw() {
    U8G2* u8g2 = _renderer.getU8G2();
    if (!u8g2) return;

    uint8_t originalDrawColor = u8g2->getDrawColor();
    // uint8_t originalBitmapMode = u8g2->getBitmapMode(); // Cannot get bitmap mode
    // Store the font that was set before this draw call, if possible, or assume a system default
    // For now, we will ensure ParticleSystem's defaultFont is restored.
    const uint8_t* fontBeforeParticleDraw = _defaultFont; // Best guess, or require it to be passed

    for (int i = 0; i < MAX_PARTICLES; ++i) {
        if (!_particlePool[i].active) continue;

        const Particle& p = _particlePool[i];
        int drawX = _renderer.getXOffset() + static_cast<int>(round(p.x));
        int drawY = _renderer.getYOffset() + static_cast<int>(round(p.y));

        u8g2->setDrawColor(p.drawColor);

        switch(p.visualType) {
            case ParticleVisualType::CHARACTER:
                {
                    const uint8_t* targetFont = (p.font != nullptr) ? p.font : _defaultFont;
                    if (targetFont) {
                        u8g2->setFont(targetFont);
                    } else {
                        u8g2->setFont(_defaultFont); // Fallback to particle system's default
                    }
                    u8g2->drawGlyph(drawX, drawY, p.character);
                }
                break;

            case ParticleVisualType::STATIC_BITMAP:
                if (p.assetData && p.assetData->bitmap) {
                    u8g2->setBitmapMode(0); 
                    u8g2->drawXBMP(drawX, drawY, p.assetData->width, p.assetData->height, p.assetData->bitmap);
                }
                break;

            case ParticleVisualType::ANIMATED_SPRITESHEET:
                if (p.assetData && p.assetData->bitmap && p.currentFrame < p.assetData->frameCount) {
                    const uint8_t* framePtr = p.assetData->bitmap + (p.currentFrame * p.assetData->bytesPerFrame);
                    u8g2->setBitmapMode(0); 
                    u8g2->drawXBMP(drawX, drawY, p.assetData->width, p.assetData->height, framePtr);
                }
                break;
        }
    }

    // Restore the default font (of the ParticleSystem) and draw color at the end
    if (_defaultFont) { // Ensure _defaultFont is not null
         u8g2->setFont(_defaultFont);
    }
    u8g2->setDrawColor(originalDrawColor);
    // u8g2->setBitmapMode(0); // Set to a known default if necessary
}
// --- END OF FILE src/ParticleSystem.cpp ---