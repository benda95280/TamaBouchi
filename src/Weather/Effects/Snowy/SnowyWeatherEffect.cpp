#include "SnowyWeatherEffect.h"
#include <U8g2lib.h>
#include <algorithm> 
#include <cmath>     
#include "../../../System/GameContext.h" // Ensure GameContext is known
#include "../../WeatherManager.h" 
#include "SerialForwarder.h"
#include "Renderer.h"

extern WeatherManager* weatherManager_ptr; // Still used for default font, consider passing default font via context too

// Constructor now takes GameContext
SnowyWeatherEffect::SnowyWeatherEffect(GameContext& context, bool isHeavy)
    : WeatherEffectBase(context), _isHeavySnow(isHeavy) { // Pass context to base
    _snowFont = u8g2_font_4x6_tf; 
    if (_context.serialForwarder) { // Access logger via context
        _context.serialForwarder->printf("%sWeatherEffect created\n", _isHeavySnow ? "HeavySnow" : "Snowy");
    }
}

void SnowyWeatherEffect::init(unsigned long currentTime) {
    initSnowFlakes();
    if (_context.serialForwarder) { // Access logger via context
        _context.serialForwarder->printf("%sWeatherEffect init\n", _isHeavySnow ? "HeavySnow" : "Snowy");
    }
}

void SnowyWeatherEffect::initSnowFlakes() {
    if (!_context.renderer) return;
    int screenW = _context.renderer->getWidth();
    int screenH = _context.renderer->getHeight();
    for (int i = 0; i < MAX_SNOW_FLAKES; ++i) {
        _snowFlakes[i] = {
            (float)random(0, screenW),
            (float)random(-screenH, -5),
            (float)random(3, _isHeavySnow ? 8 : 7) / 10.0f, 
            (float)random(-4, 5) / 10.0f, 
            (random(0,3) == 0) ? '.' : '*'
        };
    }
    if (_context.serialForwarder) _context.serialForwarder->println("Snowflakes initialized/re-initialized.");
}

void SnowyWeatherEffect::update(unsigned long currentTime) {
    updateSnowFlakes();
}

void SnowyWeatherEffect::updateSnowFlakes() {
    if (!_context.renderer) return;
    int screenW = _context.renderer->getWidth();
    int screenH = _context.renderer->getHeight();
    uint8_t speed_factor = _isHeavySnow ? 2 : 1;
    uint8_t current_density = _particleDensity; // This is a member of WeatherEffectBase
    uint8_t flakes_to_update = (MAX_SNOW_FLAKES * current_density) / 100;
    flakes_to_update = std::min((int)MAX_SNOW_FLAKES, (int)flakes_to_update);

    for (int i = 0; i < flakes_to_update; ++i) {
        _snowFlakes[i].y += _snowFlakes[i].speedY * speed_factor;
        _snowFlakes[i].x += (_currentWindFactor * _snowFlakes[i].speedXFactor * speed_factor * 1.2f); 

        if (_snowFlakes[i].y > screenH + 5) {
            _snowFlakes[i].x = (float)random(0, screenW);
            _snowFlakes[i].y = (float)random(-20, -5);
            _snowFlakes[i].speedY = (float)random(3, _isHeavySnow ? 8 : 7) / 10.0f;
            _snowFlakes[i].speedXFactor = (float)random(-4, 5) / 10.0f;
            _snowFlakes[i].displayChar = (random(0,3) == 0) ? '.' : '*';
        } else if (_snowFlakes[i].x < -5) {
            _snowFlakes[i].x = screenW + 4;
        } else if (_snowFlakes[i].x > screenW + 5) {
            _snowFlakes[i].x = -4;
        }
    }
}

void SnowyWeatherEffect::drawBackground() { }

void SnowyWeatherEffect::drawForeground() {
    float speed_factor = _isHeavySnow ? 0.8f : 0.5f; 
    drawSnow(speed_factor);
}

void SnowyWeatherEffect::drawSnow(float speed_factor_unused) {
    if (!_context.renderer || !_context.display) return;
    U8G2* u8g2 = _context.display;
    Renderer& renderer = *_context.renderer;


    uint8_t current_density = _particleDensity;
    uint8_t flakes_to_draw = (MAX_SNOW_FLAKES * current_density) / 100;
    flakes_to_draw = std::min((int)MAX_SNOW_FLAKES, (int)flakes_to_draw);

    const uint8_t* fontToUse = _snowFont ? _snowFont : _context.defaultFont; // Use specific snow font or context default

    u8g2->setFont(fontToUse);
    uint8_t originalColor = u8g2->getDrawColor();
    u8g2->setDrawColor(1); 

    for (int i = 0; i < flakes_to_draw; ++i) {
        int x_abs = renderer.getXOffset() + static_cast<int>(round(_snowFlakes[i].x));
        int y_abs = renderer.getYOffset() + static_cast<int>(round(_snowFlakes[i].y));
        if (x_abs >= renderer.getXOffset() && x_abs < renderer.getXOffset() + renderer.getWidth() &&
            y_abs >= renderer.getYOffset() && y_abs < renderer.getYOffset() + renderer.getHeight()) {
             char str[2] = {_snowFlakes[i].displayChar, '\0'};
             // drawText will handle offsets correctly
             renderer.drawText(static_cast<int>(round(_snowFlakes[i].x)), static_cast<int>(round(_snowFlakes[i].y)), str);
        }
    }
    // Restore default font from context if it was changed
    if (_context.defaultFont && fontToUse != _context.defaultFont) {
         u8g2->setFont(_context.defaultFont);
    }
    u8g2->setDrawColor(originalColor);
}

WeatherType SnowyWeatherEffect::getType() const {
    return _isHeavySnow ? WeatherType::HEAVY_SNOW : WeatherType::SNOWY;
}