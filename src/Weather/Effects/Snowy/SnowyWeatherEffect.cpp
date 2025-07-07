#include "SnowyWeatherEffect.h"
#include <U8g2lib.h>
#include <algorithm> 
#include <cmath>     
#include "../../../System/GameContext.h"
#include "../../WeatherManager.h" 
#include "SerialForwarder.h"
#include "Renderer.h"
#include <iterator>

extern WeatherManager* weatherManager_ptr; 

// Constructor now takes GameContext
SnowyWeatherEffect::SnowyWeatherEffect(GameContext& context, bool isHeavy)
    : WeatherEffectBase(context), _isHeavySnow(isHeavy) {
    _snowFont = u8g2_font_4x6_tf;
    _largeSnowFont = u8g2_font_5x7_tf;
    if (_context.serialForwarder) {
        _context.serialForwarder->printf("%sWeatherEffect created\n", _isHeavySnow ? "HeavySnow" : "Snowy");
    }
}

SnowyWeatherEffect::~SnowyWeatherEffect() {
    debugPrintf("WEATHER", "%sWeatherEffect destroyed, freeing buffers.", _isHeavySnow ? "HeavySnow" : "Snowy");
    if (_snowFlakes != nullptr) {
        delete[] _snowFlakes;
        _snowFlakes = nullptr;
    }
}

void SnowyWeatherEffect::init(unsigned long currentTime) {
    if (_snowFlakes == nullptr) {
        _snowFlakes = new (std::nothrow) SnowFlake[MAX_SNOW_FLAKES];
        if (_snowFlakes == nullptr) {
            debugPrint("WEATHER", "FATAL: Failed to allocate memory for snow flakes!");
            return;
        }
        debugPrintf("WEATHER", "%sWeatherEffect: Allocated buffers.", _isHeavySnow ? "HeavySnow" : "Snowy");
    }
    initSnowFlakes();
    if (_context.serialForwarder) {
        _context.serialForwarder->printf("%sWeatherEffect init\n", _isHeavySnow ? "HeavySnow" : "Snowy");
    }
}

void SnowyWeatherEffect::initSnowFlakes() {
    if (!_context.renderer || !_snowFlakes) return;
    int screenW = _context.renderer->getWidth();
    int screenH = _context.renderer->getHeight();
    unsigned long currentTime = millis();

    for (int i = 0; i < MAX_SNOW_FLAKES; ++i) {
        _snowFlakes[i] = {
            (float)random(0, screenW),
            (float)random(-screenH, -5),
            (float)random(3, _isHeavySnow ? 8 : 7) / 10.0f, 
            (float)random(-4, 5) / 10.0f, 
            (random(0,3) == 0) ? '.' : '*',
            (random(100) < 15), // canGrow
            false, // isGrowing
            1.0f, // currentSizeMultiplier
            currentTime + random(2000, 8001) // nextSizeChangeTime
        };
    }
    if (_context.serialForwarder) _context.serialForwarder->println("Snowflakes initialized/re-initialized.");
}

void SnowyWeatherEffect::update(unsigned long currentTime) {
    updateSnowFlakes(currentTime);
}

void SnowyWeatherEffect::updateSnowFlakes(unsigned long currentTime) {
    if (!_context.renderer || !_snowFlakes) return;
    int screenW = _context.renderer->getWidth();
    int screenH = _context.renderer->getHeight();
    uint8_t speed_factor = _isHeavySnow ? 2 : 1;
    uint8_t current_density = _particleDensity;
    uint8_t flakes_to_update = (MAX_SNOW_FLAKES * current_density) / 100;
    flakes_to_update = std::min((int)MAX_SNOW_FLAKES, (int)flakes_to_update);

    for (int i = 0; i < flakes_to_update; ++i) {
        SnowFlake& flake = _snowFlakes[i];
        flake.y += flake.speedY * speed_factor;
        flake.x += (_currentWindFactor * flake.speedXFactor * speed_factor * 1.2f); 

        if (flake.canGrow) {
            if (currentTime >= flake.nextSizeChangeTime) {
                flake.isGrowing = !flake.isGrowing;
                flake.nextSizeChangeTime = currentTime + random(2000, 8001);
            }
            
            float targetSize = flake.isGrowing ? 1.5f : 1.0f;
            // Simple linear interpolation for smooth transition
            flake.currentSizeMultiplier += (targetSize - flake.currentSizeMultiplier) * 0.05f;
        }

        if (flake.y > screenH + 5) {
            flake.x = (float)random(0, screenW);
            flake.y = (float)random(-20, -5);
            flake.speedY = (float)random(3, _isHeavySnow ? 8 : 7) / 10.0f;
            flake.speedXFactor = (float)random(-4, 5) / 10.0f;
            flake.displayChar = (random(0,3) == 0) ? '.' : '*';
        } else if (flake.x < -5) {
            flake.x = screenW + 4;
        } else if (flake.x > screenW + 5) {
            flake.x = -4;
        }
    }
}

void SnowyWeatherEffect::drawBackground() { }

void SnowyWeatherEffect::drawForeground() {
    drawSnow();
}

void SnowyWeatherEffect::drawSnow() {
    if (!_context.renderer || !_context.display || !_snowFlakes) return;
    U8G2* u8g2 = _context.display;
    Renderer& renderer = *_context.renderer;


    uint8_t current_density = _particleDensity;
    uint8_t flakes_to_draw = (MAX_SNOW_FLAKES * current_density) / 100;
    flakes_to_draw = std::min((int)MAX_SNOW_FLAKES, (int)flakes_to_draw);

    uint8_t originalColor = u8g2->getDrawColor();
    u8g2->setDrawColor(1); 

    for (int i = 0; i < flakes_to_draw; ++i) {
        const SnowFlake& flake = _snowFlakes[i];
        int x_local = static_cast<int>(round(flake.x));
        int y_local = static_cast<int>(round(flake.y));

        if (x_local >= 0 && x_local < renderer.getWidth() &&
            y_local >= 0 && y_local < renderer.getHeight()) {
            
            char str[2] = {flake.displayChar, '\0'};
            const uint8_t* fontToUse = _snowFont;

            if (flake.canGrow) {
                if (flake.currentSizeMultiplier >= 1.4f) {
                    fontToUse = _largeSnowFont;
                    str[0] = '*';
                } else if (flake.currentSizeMultiplier >= 1.2f) {
                    fontToUse = _snowFont;
                    str[0] = '*';
                } else {
                    fontToUse = _snowFont;
                    str[0] = '.';
                }
            }
            
            u8g2->setFont(fontToUse);
            // Draw directly with u8g2, applying the renderer's offsets manually
            u8g2->drawStr(renderer.getXOffset() + x_local, renderer.getYOffset() + y_local, str);
        }
    }

    if (_context.defaultFont) {
         u8g2->setFont(_context.defaultFont);
    }
    u8g2->setDrawColor(originalColor);
}

WeatherType SnowyWeatherEffect::getType() const {
    return _isHeavySnow ? WeatherType::HEAVY_SNOW : WeatherType::SNOWY;
}