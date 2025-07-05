#include "ParticleSystem.h"
#include <cmath> // For round
#include "DebugUtils.h"
#include <algorithm> // For std::for_each, std::find_if
#include <iterator> // For std::begin, std::end


ParticleSystem::ParticleSystem(Renderer& renderer, const uint8_t* defaultFont)
    : _renderer(renderer), _defaultFont(defaultFont)
{
    // Pool is initialized by Particle default constructor (active=false)
}

int ParticleSystem::findInactiveParticle() {
    auto it = std::find_if(std::begin(_particlePool), std::end(_particlePool), [](const Particle& p) {
        return !p.active;
    });

    if (it != std::end(_particlePool)) {
        return std::distance(std::begin(_particlePool), it);
    }
    return -1;
}

void ParticleSystem::reset() {
    std::for_each(std::begin(_particlePool), std::end(_particlePool), [](Particle& p) {
        p.active = false;
    });
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
    std::for_each(std::begin(_particlePool), std::end(_particlePool), [&](Particle& p) {
        if (!p.active) return;
        
        if (currentTime - p.spawnTime >= p.lifetimeMs) {
            p.active = false;
            return;
        }

        p.x += p.vx;
        p.y += p.vy;

        if (p.visualType == ParticleVisualType::ANIMATED_SPRITESHEET && p.assetData && p.assetData->isSpritesheet()) {
            if (currentTime - p.lastFrameUpdateTime >= p.assetData->frameDurationMs) {
                p.lastFrameUpdateTime += p.assetData->frameDurationMs;
                p.currentFrame++;
                if (p.currentFrame >= p.assetData->frameCount) {
                    p.currentFrame = 0;
                }
            }
        }
    });
}

void ParticleSystem::draw() {
    U8G2* u8g2 = _renderer.getU8G2();
    if (!u8g2) return;

    uint8_t originalDrawColor = u8g2->getDrawColor();
    const uint8_t* fontBeforeParticleDraw = _defaultFont; 

    std::for_each(std::begin(_particlePool), std::end(_particlePool), [&](const Particle& p) {
        if (!p.active) return;

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
    });

    if (_defaultFont) {
         u8g2->setFont(_defaultFont);
    }
    u8g2->setDrawColor(originalDrawColor);
}