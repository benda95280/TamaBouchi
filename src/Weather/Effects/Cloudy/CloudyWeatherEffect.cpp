#include "CloudyWeatherEffect.h"
#include <cmath>     
#include <algorithm> 
#include <U8g2lib.h>
#include "../../../System/GameContext.h" // Ensure GameContext is known for _context usage
#include "../../../DebugUtils.h"
#include "SerialForwarder.h"
#include "Renderer.h"


CloudyWeatherEffect::CloudyWeatherEffect(GameContext& context) // Takes GameContext
    : WeatherEffectBase(context) { // Pass context to base
    if (_context.serialForwarder) _context.serialForwarder->println("CloudyWeatherEffect created");
}

void CloudyWeatherEffect::init(unsigned long currentTime) {
    if (!_context.renderer) return;
    int screenW = _context.renderer->getWidth();
    int screenH_half = _context.renderer->getHeight() / 2;

    for (int i = 0; i < MAX_CLOUDS; ++i) {
        _clouds[i].active = (i < 2); 
        _clouds[i].x = (float)random(0, screenW);
        _clouds[i].y = (float)random(CLOUD_VERTICAL_OFFSET, screenH_half - (int)(MAX_BASE_CLOUD_RADIUS * 1.5f) - 10 + CLOUD_VERTICAL_OFFSET);
        _clouds[i].speed = (float)random(CLOUD_SPEED_MIN_TENTHS, CLOUD_SPEED_MAX_TENTHS + 1) / 10.0f;
        _clouds[i].circles.clear();

        int numCircles = random(MIN_CIRCLES_PER_CLOUD, MAX_CIRCLES_PER_CLOUD + 1);
        float baseRadius = MIN_BASE_CLOUD_RADIUS + (float)random(0, (int)((MAX_BASE_CLOUD_RADIUS - MIN_BASE_CLOUD_RADIUS) * 100) + 1) / 100.0f;

        float cloudMinRelX = 0.0f, cloudMaxRelX = 0.0f, cloudMinRelY = 0.0f;
        bool firstCircle = true;

        for (int j = 0; j < numCircles; ++j) {
            CloudCircle circle;
            circle.relativeX = (float)random(- (int)(baseRadius * MAX_CIRCLE_OFFSET_X_FACTOR), (int)(baseRadius * MAX_CIRCLE_OFFSET_X_FACTOR) + 1);
            circle.relativeY = (float)random(- (int)(baseRadius * MAX_CIRCLE_OFFSET_Y_FACTOR_ABOVE), (int)(baseRadius * MAX_CIRCLE_OFFSET_Y_FACTOR_BELOW) +1 );
            
            float radiusVariation = MIN_CIRCLE_RADIUS_VARIATION_FACTOR + (float)random(0, (int)((MAX_CIRCLE_RADIUS_VARIATION_FACTOR - MIN_CIRCLE_RADIUS_VARIATION_FACTOR) * 100) + 1) / 100.0f;
            circle.radius = baseRadius * radiusVariation;
            circle.radius = std::max(2.0f, circle.radius);
            _clouds[i].circles.push_back(circle);

            float c_left = circle.relativeX - circle.radius;
            float c_right = circle.relativeX + circle.radius;
            float c_top_relative_to_flat_bottom = circle.relativeY - circle.radius;

            if (firstCircle) {
                cloudMinRelX = c_left; cloudMaxRelX = c_right; cloudMinRelY = c_top_relative_to_flat_bottom; firstCircle = false;
            } else {
                if (c_left < cloudMinRelX) cloudMinRelX = c_left;
                if (c_right > cloudMaxRelX) cloudMaxRelX = c_right;
                if (c_top_relative_to_flat_bottom < cloudMinRelY) cloudMinRelY = c_top_relative_to_flat_bottom;
            }
        }
        _clouds[i].minRelativeX = cloudMinRelX;
        _clouds[i].maxRelativeX = cloudMaxRelX;
        _clouds[i].minRelativeY = cloudMinRelY;
        _clouds[i].maxRelativeY = 0; 
    }
    _lastCloudUpdateTime = currentTime;
    if (_context.serialForwarder) _context.serialForwarder->println("CloudyWeatherEffect init: Clouds initialized.");
}

void CloudyWeatherEffect::update(unsigned long currentTime) {
    if (!_context.renderer) return;
    if (currentTime - _lastCloudUpdateTime < CLOUD_UPDATE_INTERVAL_MS) {
        return;
    }
    _lastCloudUpdateTime = currentTime;

    int screenW = _context.renderer->getWidth();
    int screenH_half = _context.renderer->getHeight() / 2;

    for (int i = 0; i < MAX_CLOUDS; ++i) {
        if (_clouds[i].active) {
            _clouds[i].x += _clouds[i].speed + _currentWindFactor * 0.3f;

            float cloudDrawWidth = _clouds[i].maxRelativeX - _clouds[i].minRelativeX;
            float cloudActualLeftEdge = _clouds[i].x + _clouds[i].minRelativeX;

            if (cloudActualLeftEdge > screenW + CLOUD_WRAP_BUFFER) {
                _clouds[i].x = -(_clouds[i].maxRelativeX + random(5, CLOUD_WRAP_BUFFER * 2));
                _clouds[i].y = (float)random(CLOUD_VERTICAL_OFFSET, screenH_half - (int)(MAX_BASE_CLOUD_RADIUS * 1.5f) - 10 + CLOUD_VERTICAL_OFFSET);
                _clouds[i].speed = (float)random(CLOUD_SPEED_MIN_TENTHS, CLOUD_SPEED_MAX_TENTHS + 1) / 10.0f;
            } else if (_clouds[i].x + _clouds[i].maxRelativeX < -CLOUD_WRAP_BUFFER) {
                _clouds[i].x = screenW - _clouds[i].minRelativeX + random(5, CLOUD_WRAP_BUFFER * 2);
                _clouds[i].y = (float)random(CLOUD_VERTICAL_OFFSET, screenH_half - (int)(MAX_BASE_CLOUD_RADIUS * 1.5f) - 10 + CLOUD_VERTICAL_OFFSET);
                _clouds[i].speed = (float)random(CLOUD_SPEED_MIN_TENTHS, CLOUD_SPEED_MAX_TENTHS + 1) / 10.0f;
            }
        }
    }
}

void CloudyWeatherEffect::drawBackground() {
    if (!_context.renderer || !_context.display) return;
    U8G2* u8g2 = _context.display;
    Renderer& renderer = *_context.renderer;

    uint8_t originalColor = u8g2->getDrawColor();
    u8g2->setDrawColor(1);

    int screenTopOffset = renderer.getYOffset();
    int cloudBottomRenderLimit_abs = screenTopOffset + renderer.getHeight() / 2 - 5 + CLOUD_VERTICAL_OFFSET;

    for (int i = 0; i < MAX_CLOUDS; ++i) {
        if (_clouds[i].active) {
            int cloudFlatBottomY_abs = screenTopOffset + static_cast<int>(round(_clouds[i].y));
            for (const auto& circle : _clouds[i].circles) {
                float r = circle.radius; if (r < 1.0f) r = 1.0f;
                int R_int = static_cast<int>(round(r));
                int absCircleCenterX = renderer.getXOffset() + static_cast<int>(round(_clouds[i].x + circle.relativeX));
                int absCircleCenterY = cloudFlatBottomY_abs + static_cast<int>(round(circle.relativeY));

                for (int y_scan_relative_to_circle_center = -R_int; y_scan_relative_to_circle_center <= R_int; ++y_scan_relative_to_circle_center) {
                    int current_y_scan_abs = absCircleCenterY + y_scan_relative_to_circle_center;
                    if (current_y_scan_abs >= cloudFlatBottomY_abs) continue;
                    if (current_y_scan_abs >= cloudBottomRenderLimit_abs) continue;
                    if (current_y_scan_abs < screenTopOffset - R_int) continue; // Also check against screenTopOffset
                    float x_span_squared = (r * r) - (y_scan_relative_to_circle_center * y_scan_relative_to_circle_center);
                    if (x_span_squared < 0) continue;
                    float x_span_float = sqrtf(x_span_squared);
                    if (isnan(x_span_float)) continue;
                    int x_span_int = static_cast<int>(round(x_span_float));
                    if (x_span_int > 0) {
                        u8g2->drawHLine(absCircleCenterX - x_span_int, current_y_scan_abs, x_span_int * 2 + 1);
                    } else { u8g2->drawPixel(absCircleCenterX, current_y_scan_abs); }
                }
            }
        }
    }
    u8g2->setDrawColor(originalColor);
}

void CloudyWeatherEffect::drawForeground() {}

WeatherType CloudyWeatherEffect::getType() const {
    return WeatherType::CLOUDY;
}