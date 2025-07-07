#include "RainyWeatherEffect.h"
#include <U8g2lib.h>
#include <algorithm> 
#include "../../../System/GameContext.h"
#include "../../../DebugUtils.h"
#include "SerialForwarder.h"
#include "Renderer.h"
#include <iterator>

RainyWeatherEffect::RainyWeatherEffect(GameContext& context, bool isHeavy)
    : WeatherEffectBase(context), _isHeavyRain(isHeavy) {
    if (_context.serialForwarder) _context.serialForwarder->printf("%sWeatherEffect created\n", _isHeavyRain ? "HeavyRain" : "Rainy");
}

RainyWeatherEffect::~RainyWeatherEffect() {
    debugPrintf("WEATHER", "%sWeatherEffect destroyed, freeing buffers.", _isHeavyRain ? "HeavyRain" : "Rainy");
    if (_rainDrops != nullptr) {
        delete[] _rainDrops;
        _rainDrops = nullptr;
    }
}

void RainyWeatherEffect::init(unsigned long currentTime) {
    if (_rainDrops == nullptr) {
        _rainDrops = new (std::nothrow) RainDrop[MAX_RAIN_DROPS];
        if (_rainDrops == nullptr) {
            debugPrint("WEATHER", "FATAL: Failed to allocate memory for rain drops!");
            return;
        }
        debugPrintf("WEATHER", "%sWeatherEffect: Allocated buffers.", _isHeavyRain ? "HeavyRain" : "Rainy");
    }
    initRainDrops();
    if (_context.serialForwarder) _context.serialForwarder->printf("%sWeatherEffect init\n", _isHeavyRain ? "HeavyRain" : "Rainy");
}

void RainyWeatherEffect::initRainDrops() {
    if (!_context.renderer || !_rainDrops) return;
    int screenW = _context.renderer->getWidth();
    int screenH = _context.renderer->getHeight();

    for(int i = 0; i < MAX_RAIN_DROPS; ++i) {
        _rainDrops[i] = { (int)random(0, screenW), (int)random(-screenH, -5), (int)random(1, _isHeavyRain ? 5 : 4) };
    }

    if (_context.serialForwarder) _context.serialForwarder->println("Raindrops initialized/re-initialized.");
}

void RainyWeatherEffect::update(unsigned long currentTime) {
    updateRainDrops();
}

void RainyWeatherEffect::updateRainDrops() {
    if (!_context.renderer || !_rainDrops) return;
    int screenW = _context.renderer->getWidth();
    int screenH = _context.renderer->getHeight();
    uint8_t speed_factor = _isHeavyRain ? 2 : 1;
    uint8_t current_density = _particleDensity;
    uint8_t drops_to_update = (MAX_RAIN_DROPS * current_density) / 100;
    drops_to_update = std::min((int)MAX_RAIN_DROPS, (int)drops_to_update);

    for (int i = 0; i < drops_to_update; ++i) {
        RainDrop& drop = _rainDrops[i];
        int current_speed = std::abs(drop.speed);
        if (current_speed == 0) current_speed = 1;
        drop.y += current_speed * speed_factor;
        drop.x += static_cast<int>(round(_currentWindFactor * speed_factor * 0.5f)); 

        if (drop.y > screenH || drop.x < -5 || drop.x > screenW + 5) {
            drop.x = (int)random(0, screenW);
            drop.y = -(int)random(5, 21);
            drop.speed = (int)random(1, _isHeavyRain ? 5 : 4);
        }
    }
}

void RainyWeatherEffect::drawBackground() { }

void RainyWeatherEffect::drawForeground() {
    uint8_t speed_factor = _isHeavyRain ? 2 : 1;
    drawRain(speed_factor);
}

void RainyWeatherEffect::drawRain(uint8_t speed_factor) {
    if (!_context.renderer || !_context.display || !_rainDrops) return;
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
        const RainDrop& drop = _rainDrops[i];
        int x_local = drop.x; 
        int y1_local = drop.y; 
        int y2_local = y1_local + (_isHeavyRain ? 3 : 2); 

        if (x_local < 0 || x_local >= screenW || y2_local < 0 || y1_local >= screenH) {
            continue;
        }
        
        int draw_y1 = std::max(0, y1_local);
        int draw_y2 = std::min(screenH - 1, y2_local);
        
        if (draw_y1 <= draw_y2) {
             renderer.drawLine(x_local, draw_y1, x_local, draw_y2);
        }
    }
    u8g2->setDrawColor(originalColor);
}

WeatherType RainyWeatherEffect::getType() const {
    return _isHeavyRain ? WeatherType::HEAVY_RAIN : WeatherType::RAINY;
}