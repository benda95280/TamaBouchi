#include "SunnyWeatherEffect.h"
#include "SerialForwarder.h" 
#include "Renderer.h" 
#include <U8g2lib.h>
#include <cmath>     
#include <algorithm> 
#include "../../../System/GameContext.h" // For GameContext definition access (if not fully via base)
#include "../../../DebugUtils.h"


// Constructor now takes GameContext
SunnyWeatherEffect::SunnyWeatherEffect(GameContext& context)
    : WeatherEffectBase(context) { // Pass context to base
    if (_context.serialForwarder) _context.serialForwarder->println("SunnyWeatherEffect created");
}

void SunnyWeatherEffect::init(unsigned long currentTime) {
    _sunIsLeft = (random(0, 2) == 0);
    _sunRayAnimationPhase = 0.0f;
    _lastSunRayUpdateTime = currentTime;
    if (_context.serialForwarder) _context.serialForwarder->printf("SunnyWeatherEffect init. Sun is %s.\n", _sunIsLeft ? "left" : "right");
}

void SunnyWeatherEffect::update(unsigned long currentTime) {
    if(currentTime - _lastSunRayUpdateTime > 50) { 
        _sunRayAnimationPhase += SUN_RAY_PULSATION_SPEED;
        if (_sunRayAnimationPhase > TWO_PI) _sunRayAnimationPhase -= TWO_PI;
        _lastSunRayUpdateTime = currentTime;
    }
}

void SunnyWeatherEffect::drawBackground() {
    if (!_context.renderer || !_context.display) return; // Use context for renderer and display
    U8G2* u8g2 = _context.display;
    Renderer& renderer = *_context.renderer;


    int sunCenterX, sunCenterY;
    if (_sunIsLeft) {
        sunCenterX = renderer.getXOffset() + SUN_X_OFFSET_FROM_EDGE;
        sunCenterY = renderer.getYOffset() + SUN_Y_OFFSET_FROM_TOP;
    } else {
        sunCenterX = renderer.getXOffset() + renderer.getWidth() - SUN_X_OFFSET_FROM_EDGE;
        sunCenterY = renderer.getYOffset() + SUN_Y_OFFSET_FROM_TOP;
    }

    uint8_t originalDrawColor = u8g2->getDrawColor();
    u8g2->setDrawColor(1);
    u8g2->drawDisc(sunCenterX, sunCenterY, SUN_INNER_RADIUS);

    for (int i = 0; i < SUN_NUM_RAYS; ++i) {
        float angle = (TWO_PI / SUN_NUM_RAYS) * i + _sunRayAnimationPhase;
        float rayLengthVariation = SUN_RAY_LENGTH_VARIATION * sin(_sunRayAnimationPhase * 2.5f + i * 0.7f);

        int startX = sunCenterX + static_cast<int>(round(cos(angle) * (SUN_INNER_RADIUS + 1)));
        int startY = sunCenterY + static_cast<int>(round(sin(angle) * (SUN_INNER_RADIUS + 1)));
        int endX = sunCenterX + static_cast<int>(round(cos(angle) * (SUN_OUTER_RADIUS + rayLengthVariation)));
        int endY = sunCenterY + static_cast<int>(round(sin(angle) * (SUN_OUTER_RADIUS + rayLengthVariation)));
        
        bool startInBounds = (startX >= renderer.getXOffset() && startX < renderer.getXOffset() + renderer.getWidth() &&
                              startY >= renderer.getYOffset() && startY < renderer.getYOffset() + renderer.getHeight());
        bool endInBounds = (endX >= renderer.getXOffset() && endX < renderer.getXOffset() + renderer.getWidth() &&
                            endY >= renderer.getYOffset() && endY < renderer.getYOffset() + renderer.getHeight());

        if (startInBounds || endInBounds) { 
            int clipped_startX = std::max(renderer.getXOffset(), std::min(renderer.getXOffset() + renderer.getWidth() - 1, startX));
            int clipped_startY = std::max(renderer.getYOffset(), std::min(renderer.getYOffset() + renderer.getHeight() - 1, startY));
            int clipped_endX = std::max(renderer.getXOffset(), std::min(renderer.getXOffset() + renderer.getWidth() - 1, endX));
            int clipped_endY = std::max(renderer.getYOffset(), std::min(renderer.getYOffset() + renderer.getHeight() - 1, endY));

            u8g2->drawLine(clipped_startX, clipped_startY, clipped_endX, clipped_endY);
        }
    }
    u8g2->setDrawColor(originalDrawColor);
}

void SunnyWeatherEffect::drawForeground() { }

WeatherType SunnyWeatherEffect::getType() const {
    return WeatherType::SUNNY;
}