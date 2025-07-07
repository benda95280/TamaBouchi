#include "WindyWeatherEffect.h"
#include <U8g2lib.h>
#include <algorithm>
#include <cmath>
#include "../../../System/GameContext.h"
#include "../../../DebugUtils.h"
#include "SerialForwarder.h"
#include "Renderer.h"

WindyWeatherEffect::WindyWeatherEffect(GameContext& context)
    : WeatherEffectBase(context) {
    if (_context.serialForwarder) _context.serialForwarder->println("WindyWeatherEffect created");
}

WindyWeatherEffect::~WindyWeatherEffect() {
    debugPrint("WEATHER", "WindyWeatherEffect destroyed, freeing buffers.");
    if (_windLines != nullptr) {
        delete[] _windLines;
        _windLines = nullptr;
    }
}

void WindyWeatherEffect::init(unsigned long currentTime) {
    if (_windLines == nullptr) {
        _windLines = new (std::nothrow) WindLine[MAX_WIND_LINES];
        if (_windLines == nullptr) {
            debugPrint("WEATHER", "FATAL: Failed to allocate memory for wind lines!");
            return;
        }
        debugPrint("WEATHER", "WindyWeatherEffect: Allocated buffers.");
    }
    initWindLines();
    if (_context.serialForwarder) _context.serialForwarder->println("WindyWeatherEffect init");
}

void WindyWeatherEffect::update(unsigned long currentTime) {
    updateWindLines();
}

void WindyWeatherEffect::initWindLines() {
    if (!_context.renderer || !_windLines) return;
    int screenW = _context.renderer->getWidth();
    int screenH = _context.renderer->getHeight();
    for (int i = 0; i < MAX_WIND_LINES; ++i) {
        int dx = (int)random(8, 20);
        int dy = (int)random(-3, 4);
        _windLines[i].x1 = (int)random(0, screenW);
        _windLines[i].y1 = (int)random(0, screenH);
        _windLines[i].x2 = _windLines[i].x1 - dx;
        _windLines[i].y2 = _windLines[i].y1 + dy;
    }
}

void WindyWeatherEffect::updateWindLines() {
    if (!_context.renderer || !_windLines) return;
    int screenW = _context.renderer->getWidth();
    int screenH = _context.renderer->getHeight();
    for (int i = 0; i < MAX_WIND_LINES; ++i) {
        float baseSpeed = 1.5f; // Wind on its own is gentler than in a storm
        float windEffectSpeed = baseSpeed + std::abs(_currentWindFactor) * 2.0f;
        int dy = static_cast<int>(round(_currentWindFactor * 0.8f));
        dy = std::max(-2, std::min(2, dy));
        int dx_magnitude = static_cast<int>(round(windEffectSpeed));
        dx_magnitude = std::max(2, std::min(8, dx_magnitude));
        int dx = static_cast<int>(_currentWindFactor * (float)dx_magnitude);

        if (dx == 0 && _currentWindFactor != 0.0f) dx = (_currentWindFactor < 0) ? -1 : 1;

        _windLines[i].x1 += dx;
        _windLines[i].x2 += dx;
        _windLines[i].y1 += dy;
        _windLines[i].y2 += dy;

        bool outOfBounds = (_windLines[i].x1 < -30 && _windLines[i].x2 < -30) ||
                           (_windLines[i].x1 > screenW + 30 && _windLines[i].x2 > screenW + 30) ||
                           (_windLines[i].y1 < -20 && _windLines[i].y2 < -20) ||
                           (_windLines[i].y1 > screenH + 20 && _windLines[i].y2 > screenH + 20);
        
        if (outOfBounds) {
            if (_currentWindFactor > 0.1f) _windLines[i].x1 = -random(5, 20);
            else if (_currentWindFactor < -0.1f) _windLines[i].x1 = screenW + random(5, 20);
            else _windLines[i].x1 = random(0, screenW);
            _windLines[i].y1 = random(0, screenH);
            int len_dx = random(12, 25);
            int len_dy = random(-3, 4);
            _windLines[i].x2 = (_currentWindFactor > 0.1f) ? (_windLines[i].x1 + len_dx) : (_windLines[i].x1 - len_dx);
            _windLines[i].y2 = _windLines[i].y1 + len_dy;
        }
    }
}

void WindyWeatherEffect::drawWindLines() {
    if (!_context.renderer || !_windLines) return;
    Renderer& renderer = *_context.renderer;

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

    for (int i = 0; i < MAX_WIND_LINES; ++i) {
        int x1 = _windLines[i].x1;
        int y1 = _windLines[i].y1;
        int x2 = _windLines[i].x2;
        int y2 = _windLines[i].y2;
        
        int outcode1 = computeOutCode(x1, y1);
        int outcode2 = computeOutCode(x2, y2);
        bool accept = false;

        while (true) {
            if (!(outcode1 | outcode2)) { 
                accept = true;
                break;
            } else if (outcode1 & outcode2) {
                break;
            } else {
                int x=0, y=0;
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
}


void WindyWeatherEffect::drawBackground() { }
void WindyWeatherEffect::drawForeground() {
    if (std::abs(_currentWindFactor) < 0.6f) {
        return; // Don't draw if the wind is too calm
    }

    if (!_context.display) return;
    U8G2* u8g2 = _context.display;
    uint8_t originalColor = u8g2->getDrawColor();
    u8g2->setDrawColor(2); // XOR
    drawWindLines();
    u8g2->setDrawColor(originalColor);
}

WeatherType WindyWeatherEffect::getType() const {
    return WeatherType::WINDY;
}