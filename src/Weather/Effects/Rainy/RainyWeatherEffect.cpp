#include "RainyWeatherEffect.h"
#include <U8g2lib.h>
#include <algorithm> 
#include "../../../System/GameContext.h" // Ensure GameContext is known for _context usage
#include "../../../DebugUtils.h"
#include "SerialForwarder.h"
#include "Renderer.h"


RainyWeatherEffect::RainyWeatherEffect(GameContext& context, bool isHeavy) // Takes GameContext
    : WeatherEffectBase(context), _isHeavyRain(isHeavy) { // Pass context to base
    if (_context.serialForwarder) _context.serialForwarder->printf("%sWeatherEffect created\n", _isHeavyRain ? "HeavyRain" : "Rainy");
}

void RainyWeatherEffect::init(unsigned long currentTime) {
    initRainDrops();
    if (_context.serialForwarder) _context.serialForwarder->printf("%sWeatherEffect init\n", _isHeavyRain ? "HeavyRain" : "Rainy");
}

void RainyWeatherEffect::initRainDrops() {
    if (!_context.renderer) return;
    int screenW = _context.renderer->getWidth();
    int screenH = _context.renderer->getHeight();
    for (int i = 0; i < MAX_RAIN_DROPS; ++i) {
        _rainDrops[i] = { (int)random(0, screenW), (int)random(-screenH, -5), (int)random(1, _isHeavyRain ? 5 : 4) };
    }
    if (_context.serialForwarder) _context.serialForwarder->println("Raindrops initialized/re-initialized.");
}

void RainyWeatherEffect::update(unsigned long currentTime) {
    updateRainDrops();
}

void RainyWeatherEffect::updateRainDrops() {
    if (!_context.renderer) return;
    int screenW = _context.renderer->getWidth();
    int screenH = _context.renderer->getHeight();
    uint8_t speed_factor = _isHeavyRain ? 2 : 1;
    uint8_t current_density = _particleDensity; 
    uint8_t drops_to_update = (MAX_RAIN_DROPS * current_density) / 100;
    drops_to_update = std::min((int)MAX_RAIN_DROPS, (int)drops_to_update);

    for (int i = 0; i < drops_to_update; ++i) {
        int current_speed = std::abs(_rainDrops[i].speed);
        if (current_speed == 0) current_speed = 1;
        _rainDrops[i].y += current_speed * speed_factor;
        _rainDrops[i].x += static_cast<int>(round(_currentWindFactor * speed_factor * 0.5f)); 

        if (_rainDrops[i].y > screenH || _rainDrops[i].x < -5 || _rainDrops[i].x > screenW + 5) {
            _rainDrops[i].x = (int)random(0, screenW);
            _rainDrops[i].y = -(int)random(5, 21);
            _rainDrops[i].speed = (int)random(1, _isHeavyRain ? 5 : 4);
        }
    }
}

void RainyWeatherEffect::drawBackground() { }

void RainyWeatherEffect::drawForeground() {
    uint8_t speed_factor = _isHeavyRain ? 2 : 1;
    drawRain(speed_factor);
}

void RainyWeatherEffect::drawRain(uint8_t speed_factor) {
    if (!_context.renderer || !_context.display) return;
    U8G2* u8g2 = _context.display;
    Renderer& renderer = *_context.renderer;

    int screenW = renderer.getWidth();
    int screenH = renderer.getHeight();
    uint8_t current_density = _particleDensity;
    uint8_t drops_to_draw = (MAX_RAIN_DROPS * current_density) / 100;
    drops_to_draw = std::min((int)MAX_RAIN_DROPS, (int)drops_to_draw);

    uint8_t originalColor = u8g2->getDrawColor();
    u8g2->setDrawColor(2); 

    for (int i = 0; i < drops_to_draw; ++i) {
        int x_local = _rainDrops[i].x; // x relative to 0
        int y1_local = _rainDrops[i].y; // y1 relative to 0
        int y2_local = y1_local + (_isHeavyRain ? 3 : 2); 

        // Check if any part of the line is within the logical screen bounds
        // (not the absolute U8G2 buffer bounds, but the game's render area)
        if (x_local >= 0 && x_local < screenW &&
            ( (y1_local >=0 && y1_local < screenH) || (y2_local >=0 && y2_local < screenH) || (y1_local < 0 && y2_local >= screenH) ) ) {
            
            int draw_y1 = std::max(0, std::min(screenH - 1, y1_local));
            int draw_y2 = std::max(0, std::min(screenH - 1, y2_local));
            if (draw_y1 <= draw_y2) { // Ensure line has positive or zero length after clipping
                 renderer.drawLine(x_local, draw_y1, x_local, draw_y2);
            }
        }
    }
    u8g2->setDrawColor(originalColor);
}

WeatherType RainyWeatherEffect::getType() const {
    return _isHeavyRain ? WeatherType::HEAVY_RAIN : WeatherType::RAINY;
}