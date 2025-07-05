#include "SunnyWeatherEffect.h"
#include "SerialForwarder.h" 
#include "Renderer.h" 
#include <U8g2lib.h>
#include <cmath>     
#include <algorithm> 
#include "../../../System/GameContext.h"
#include "../../../DebugUtils.h"

// Constructor now takes GameContext
SunnyWeatherEffect::SunnyWeatherEffect(GameContext& context)
    : WeatherEffectBase(context) {
    if (_context.serialForwarder) _context.serialForwarder->println("SunnyWeatherEffect created");
}

void SunnyWeatherEffect::init(unsigned long currentTime) {
    if (!_context.renderer) {
        if (_context.serialForwarder) _context.serialForwarder->println("SunnyWeatherEffect Error: Renderer from context is null in init.");
        return;
    }
    if (_context.serialForwarder) _context.serialForwarder->println("SunnyWeatherEffect init");
    
    _sunIsLeft = (random(0, 2) == 0);
    _sunRayAnimationPhase = 0.0f;
    _lastSunRayUpdateTime = currentTime;
    _sunCenterY = SUN_Y_OFFSET_FROM_TOP;

    if (_sunIsLeft) {
        _sunCenterX = SUN_X_OFFSET_FROM_EDGE;
    } else {
        _sunCenterX = _context.renderer->getWidth() - SUN_X_OFFSET_FROM_EDGE;
    }
    
    _isFadingOut = false;
}

void SunnyWeatherEffect::startFadeOut(unsigned long duration) {
    if (_isFadingOut) return;
    _isFadingOut = true;
    _fadeOutStartTime = millis();
    _fadeOutDuration = duration;
    _initialFadeX = _sunCenterX;
    // Target is just off-screen
    _targetFadeX = _sunIsLeft ? - (float)SUN_OUTER_RADIUS : (float)_context.renderer->getWidth() + (float)SUN_OUTER_RADIUS;
    debugPrintf("WEATHER", "Sunny effect: Starting fade out. From %.1f to %.1f over %lu ms", _initialFadeX, _targetFadeX, _fadeOutDuration);
}


void SunnyWeatherEffect::update(unsigned long currentTime) {
    if (_isFadingOut) {
        unsigned long elapsedTime = currentTime - _fadeOutStartTime;
        if (elapsedTime >= _fadeOutDuration) {
            _sunCenterX = _targetFadeX;
        } else {
            float progress = (float)elapsedTime / (float)_fadeOutDuration;
            _sunCenterX = _initialFadeX + (_targetFadeX - _initialFadeX) * progress;
        }
    }
    
    if(currentTime - _lastSunRayUpdateTime > 50) { 
        _sunRayAnimationPhase += SUN_RAY_PULSATION_SPEED;
        if (_sunRayAnimationPhase > TWO_PI) _sunRayAnimationPhase -= TWO_PI;
        _lastSunRayUpdateTime = currentTime;
    }
}

void SunnyWeatherEffect::drawBackground() {
    if (!_context.renderer || !_context.display) return;
    U8G2* u8g2 = _context.display;
    Renderer& renderer = *_context.renderer;

    int sunDrawX = static_cast<int>(round(_sunCenterX));
    int sunDrawY = static_cast<int>(round(_sunCenterY));

    uint8_t originalDrawColor = u8g2->getDrawColor();
    u8g2->setDrawColor(1);
    
    // Draw the sun disc - U8G2 handles clipping for this perfectly.
    renderer.drawFilledCircle(sunDrawX, sunDrawY, SUN_INNER_RADIUS);

    // Cohen-Sutherland constants for line clipping
    const int INSIDE = 0; // 0000
    const int LEFT = 1;   // 0001
    const int RIGHT = 2;  // 0010
    const int BOTTOM = 4; // 0100
    const int TOP = 8;    // 1000
    const int x_min = 0;
    const int y_min = 0;
    const int x_max = renderer.getWidth() - 1;
    const int y_max = renderer.getHeight() - 1;

    auto computeOutCode = [&](int x, int y) {
        int code = INSIDE;
        if (x < x_min) code |= LEFT;
        else if (x > x_max) code |= RIGHT;
        if (y < y_min) code |= BOTTOM;
        else if (y > y_max) code |= TOP;
        return code;
    };

    for (int i = 0; i < SUN_NUM_RAYS; ++i) {
        float angle = (TWO_PI / SUN_NUM_RAYS) * i + _sunRayAnimationPhase;
        float rayLengthVariation = SUN_RAY_LENGTH_VARIATION * sin(_sunRayAnimationPhase * 2.5f + i * 0.7f);

        int x1 = sunDrawX + static_cast<int>(round(cos(angle) * (SUN_INNER_RADIUS + 1)));
        int y1 = sunDrawY + static_cast<int>(round(sin(angle) * (SUN_INNER_RADIUS + 1)));
        int x2 = sunDrawX + static_cast<int>(round(cos(angle) * (SUN_OUTER_RADIUS + rayLengthVariation)));
        int y2 = sunDrawY + static_cast<int>(round(sin(angle) * (SUN_OUTER_RADIUS + rayLengthVariation)));
        
        int outcode1 = computeOutCode(x1, y1);
        int outcode2 = computeOutCode(x2, y2);
        bool accept = false;

        while (true) {
            if (!(outcode1 | outcode2)) { // Both endpoints inside
                accept = true;
                break;
            } else if (outcode1 & outcode2) { // Both endpoints outside on same side
                break;
            } else {
                int x = 0, y = 0;
                int outcodeOut = outcode1 ? outcode1 : outcode2;
                
                if (outcodeOut & TOP) {
                    x = x1 + (x2 - x1) * (y_max - y1) / (y2 - y1);
                    y = y_max;
                } else if (outcodeOut & BOTTOM) {
                    x = x1 + (x2 - x1) * (y_min - y1) / (y2 - y1);
                    y = y_min;
                } else if (outcodeOut & RIGHT) {
                    y = y1 + (y2 - y1) * (x_max - x1) / (x2 - x1);
                    x = x_max;
                } else if (outcodeOut & LEFT) {
                    y = y1 + (y2 - y1) * (x_min - x1) / (x2 - x1);
                    x = x_min;
                }

                if (outcodeOut == outcode1) {
                    x1 = x; y1 = y;
                    outcode1 = computeOutCode(x1, y1);
                } else {
                    x2 = x; y2 = y;
                    outcode2 = computeOutCode(x2, y2);
                }
            }
        }
        if (accept) {
            renderer.drawLine(x1, y1, x2, y2);
        }
    }
    u8g2->setDrawColor(originalDrawColor);
}

void SunnyWeatherEffect::drawForeground() { }

WeatherType SunnyWeatherEffect::getType() const {
    return WeatherType::SUNNY;
}