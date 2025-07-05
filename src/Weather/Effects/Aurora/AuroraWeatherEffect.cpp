#include "AuroraWeatherEffect.h"
#include <U8g2lib.h>
#include <algorithm>
#include <cmath>
#include "../../../System/GameContext.h"
#include "../../../DebugUtils.h"
#include "Renderer.h"

AuroraWeatherEffect::AuroraWeatherEffect(GameContext& context)
    : WeatherEffectBase(context) {
    debugPrint("WEATHER", "AuroraWeatherEffect created");
}

void AuroraWeatherEffect::init(unsigned long currentTime) {
    if (!_context.renderer) return;
    int screenW = _context.renderer->getWidth();
    int screenH = _context.renderer->getHeight();

    for(int i = 0; i < MAX_CURTAINS; ++i) {
        _curtains[i].baseY = (float)random(5, screenH / 3); // Start Y position
        _curtains[i].amplitude = (float)random(4, 12);
        _curtains[i].frequency = (float)random(4, 10) / 100.0f;
        _curtains[i].speed = ((float)random(3, 11) / 100.0f) * 0.7f; // Reduced speed by 30%
        _curtains[i].timeOffset = (float)random(0, 1000);
        _curtains[i].thickness = (float)random(2, 5);
    }
    _globalAlpha = 1.0f;
    _isFadingOut = false;
    debugPrint("WEATHER", "AuroraWeatherEffect initialized.");
}

void AuroraWeatherEffect::startFadeOut(unsigned long duration) {
    if (_isFadingOut) return;
    _isFadingOut = true;
    _fadeOutStartTime = millis();
    _fadeOutDuration = duration;
}

void AuroraWeatherEffect::update(unsigned long currentTime) {
    if (_isFadingOut) {
        unsigned long elapsedTime = currentTime - _fadeOutStartTime;
        if (elapsedTime >= _fadeOutDuration) {
            _globalAlpha = 0.0f;
        } else {
            float progress = (float)elapsedTime / (float)_fadeOutDuration;
            _globalAlpha = 1.0f - progress;
        }
    } else {
        _globalAlpha = 1.0f;
    }

    for(int i = 0; i < MAX_CURTAINS; ++i) {
        _curtains[i].timeOffset += _curtains[i].speed;
    }
}

void AuroraWeatherEffect::drawBackground() {
    if (!_context.renderer || !_context.display || _globalAlpha <= 0.0f) return;
    U8G2* u8g2 = _context.display;
    Renderer& renderer = *_context.renderer;

    uint8_t originalColor = u8g2->getDrawColor();
    u8g2->setDrawColor(2); // Use XOR for transparency

    int screenW = renderer.getWidth();

    for(int i = 0; i < MAX_CURTAINS; ++i) {
        const auto& curtain = _curtains[i];
        for (float x = 0; x < screenW; x += 1.0f) {
            // Dither based on X position and global alpha
            if (random(100) > (x / screenW * 70 + 30) * _globalAlpha) continue;

            float y_offset = sin(x * curtain.frequency + curtain.timeOffset) * curtain.amplitude;
            float y_pos = curtain.baseY + y_offset;

            for(int t = 0; t < curtain.thickness; ++t) {
                int drawX = renderer.getXOffset() + static_cast<int>(round(x));
                int drawY = renderer.getYOffset() + static_cast<int>(round(y_pos + t - curtain.thickness / 2.0f));
                u8g2->drawPixel(drawX, drawY);
            }
        }
    }
    u8g2->setDrawColor(originalColor);
}

void AuroraWeatherEffect::drawForeground() {}

WeatherType AuroraWeatherEffect::getType() const {
    return WeatherType::AURORA;
}