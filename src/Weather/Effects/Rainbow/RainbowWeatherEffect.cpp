#include "RainbowWeatherEffect.h"
#include <U8g2lib.h>
#include <cmath>     
#include <algorithm> 
#include <memory>    
#include "../../../System/GameContext.h" // Ensure GameContext is known
#include "../../../DebugUtils.h"
#include "SerialForwarder.h"



RainbowWeatherEffect::RainbowWeatherEffect(GameContext& context) // Takes GameContext
    : WeatherEffectBase(context) { // Pass context to base
    if (_context.renderer && _context.defaultFont) { // Check context members
        _sparkleParticleSystem.reset(new ParticleSystem(*_context.renderer, _context.defaultFont)); 
    } else {
        debugPrint("WEATHER", "RainbowWeatherEffect Error: Renderer or DefaultFont from context is null, cannot init ParticleSystem.");
    }
    if (_context.serialForwarder) _context.serialForwarder->println("RainbowWeatherEffect created");
}

void RainbowWeatherEffect::init(unsigned long currentTime) {
    if (!_context.renderer) {
        if (_context.serialForwarder) _context.serialForwarder->println("RainbowWeatherEffect Error: Renderer from context is null in init.");
        return;
    }
    if (_context.serialForwarder) _context.serialForwarder->println("RainbowWeatherEffect init");
    
    _rainbowCenterX = _context.renderer->getWidth() / 2;
    _rainbowStartRadius = _context.renderer->getWidth() * 0.55f; 
    _rainbowCenterY = _context.renderer->getYOffset() + _context.renderer->getHeight() + 10; 
    _rainbowBandThickness = 3; _numRainbowBands = 4; _rainbowBandSpacing = 2; 

    initCloudsRainbow(currentTime);
    if (_sparkleParticleSystem) _sparkleParticleSystem->reset();
    _lastSparkleSpawnTime = currentTime;
}

void RainbowWeatherEffect::update(unsigned long currentTime) {
    updateCloudsRainbow(currentTime);
    updateSparkles(currentTime);
    if (_sparkleParticleSystem) {
        _sparkleParticleSystem->update(currentTime, 33); 
    }
}

bool RainbowWeatherEffect::isCoordOverRainbow(int screenX, int screenY) const {
    if (!_context.renderer) return false;
    int relX = screenX - (_context.renderer->getXOffset() + _rainbowCenterX);
    int relY = screenY - (_context.renderer->getYOffset() + _rainbowCenterY);

    if (relY > 0 && _rainbowCenterY > _context.renderer->getYOffset() + _context.renderer->getHeight()/2) return false; 
    if (relY < 0 && _rainbowCenterY < _context.renderer->getYOffset() + _context.renderer->getHeight()/2) return false; 

    float distSq = static_cast<float>(relX * relX + relY * relY); // Cast to float for sqrt if needed, but comparing squares is fine

    int innerRadiusInnermostBand = static_cast<int>(_rainbowStartRadius); // Cast to int for comparison
    int outerRadiusOutermostBand = static_cast<int>(_rainbowStartRadius + (_numRainbowBands - 1) * (_rainbowBandThickness + _rainbowBandSpacing) + _rainbowBandThickness);
    
    if (distSq >= static_cast<float>(innerRadiusInnermostBand * innerRadiusInnermostBand) &&
        distSq <= static_cast<float>(outerRadiusOutermostBand * outerRadiusOutermostBand)) {
        if (screenY < (_context.renderer->getYOffset() + _rainbowCenterY) ) {
            for (int i = 0; i < _numRainbowBands; ++i) {
                int bandInnerR = static_cast<int>(_rainbowStartRadius + i * (_rainbowBandThickness + _rainbowBandSpacing));
                int bandOuterR = bandInnerR + _rainbowBandThickness;
                if (distSq >= static_cast<float>(bandInnerR * bandInnerR) && distSq <= static_cast<float>(bandOuterR * bandOuterR)) {
                    if (screenY >= _context.renderer->getYOffset() && screenY < _context.renderer->getYOffset() + _context.renderer->getHeight() * 0.75f) { 
                        return true;
                    }
                }
            }
        }
    }
    return false;
}


void RainbowWeatherEffect::updateSparkles(unsigned long currentTime) {
    if (!_context.renderer) return;
    if (currentTime - _lastSparkleSpawnTime >= SPARKLE_SPAWN_INTERVAL_MS) {
        _lastSparkleSpawnTime = currentTime;
        if (_sparkleParticleSystem) {
            for (int i = 0; i < MAX_SPARKLES_PER_BURST; ++i) { 
                if (random(100) < 30) { 
                    int band = random(0, _numRainbowBands);
                    int radius = static_cast<int>(_rainbowStartRadius + band * (_rainbowBandThickness + _rainbowBandSpacing) + random(0, _rainbowBandThickness + 1));
                    
                    float angleDeg = (float)random(20, 161); 
                    float angleRad = angleDeg * PI / 180.0f;

                    float spawnX = (float)_rainbowCenterX + cos(angleRad) * radius;
                    float spawnY = (float)_rainbowCenterY - abs(sin(angleRad) * radius); 

                    spawnX = std::max(2.0f, std::min((float)_context.renderer->getWidth() - 3.0f, spawnX));
                    spawnY = std::max(2.0f, std::min((float)_context.renderer->getHeight() - 5.0f, spawnY));

                    float vx = (float)random(-1, 2) / 20.0f; 
                    float vy = (float)random(-1, 2) / 20.0f;
                    unsigned long lifetime = 200 + random(0, 201); 
                    char sparkleChar = (random(0,2) == 0) ? '.' : '\''; 
                    
                    uint8_t drawColor = isCoordOverRainbow(static_cast<int>(round(spawnX + _context.renderer->getXOffset())), static_cast<int>(round(spawnY + _context.renderer->getYOffset()))) ? 2 : 1;

                    _sparkleParticleSystem->spawnParticle(spawnX, spawnY, vx, vy, lifetime, sparkleChar, u8g2_font_4x6_tf, drawColor);
                }
            }
        }
    }
}

void RainbowWeatherEffect::drawProgrammaticRainbow() {
    if (!_context.renderer || !_context.display) return;
    U8G2* u8g2 = _context.display;
    Renderer& renderer = *_context.renderer;

    uint8_t originalColor = u8g2->getDrawColor();
    u8g2->setDrawColor(1);

    for (int i = 0; i < _numRainbowBands; ++i) {
        int currentBandStartRadius = static_cast<int>(_rainbowStartRadius + i * (_rainbowBandThickness + _rainbowBandSpacing));
        
        for (int thickness_offset = 0; thickness_offset < _rainbowBandThickness; ++thickness_offset) {
            int r = currentBandStartRadius + thickness_offset;
            if (r <= 0) continue;
            u8g2->drawCircle(renderer.getXOffset() + _rainbowCenterX, 
                             renderer.getYOffset() + _rainbowCenterY, 
                             r, U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_UPPER_RIGHT);
        }
    }
    u8g2->setDrawColor(originalColor);
}

void RainbowWeatherEffect::initCloudsRainbow(unsigned long currentTime) {
    if (!_context.renderer) return;
    int screenW = _context.renderer->getWidth();
    int cloudMinY = 0;
    int cloudMaxY = 25;

    int numActiveClouds = random(MIN_ACTIVE_CLOUDS_RAINBOW, MAX_CLOUDS_RAINBOW + 1);

    for (int i = 0; i < MAX_CLOUDS_RAINBOW; ++i) {
        if (i < numActiveClouds) {
            _clouds[i].active = true; 
            _clouds[i].x = (float)random(0, screenW);
            _clouds[i].y = (float)random(cloudMinY, cloudMaxY + 1); 
            _clouds[i].speed = (float)random(CLOUD_SPEED_MIN_TENTHS_RB, CLOUD_SPEED_MAX_TENTHS_RB + 1) / 10.0f;
            _clouds[i].circles.clear();

            int numCircles = random(MIN_CIRCLES_PER_CLOUD_RB, MAX_CIRCLES_PER_CLOUD_RB + 1);
            float baseRadius = MIN_BASE_CLOUD_RADIUS_RB + (float)random(0, (int)((MAX_BASE_CLOUD_RADIUS_RB - MIN_BASE_CLOUD_RADIUS_RB) * 100) + 1) / 100.0f;

            float cloudMinRelX = 0.0f, cloudMaxRelX = 0.0f, cloudMinRelY = 0.0f;
            bool firstCircle = true;

            for (int j = 0; j < numCircles; ++j) {
                CloudCircle circle;
                circle.relativeX = (float)random(- (int)(baseRadius * MAX_CIRCLE_OFFSET_X_FACTOR_RB), (int)(baseRadius * MAX_CIRCLE_OFFSET_X_FACTOR_RB) + 1);
                circle.relativeY = (float)random(- (int)(baseRadius * MAX_CIRCLE_OFFSET_Y_FACTOR_ABOVE_RB), (int)(baseRadius * MAX_CIRCLE_OFFSET_Y_FACTOR_BELOW_RB) +1 );
                
                float radiusVariation = MIN_CIRCLE_RADIUS_VARIATION_FACTOR_RB + (float)random(0, (int)((MAX_CIRCLE_RADIUS_VARIATION_FACTOR_RB - MIN_CIRCLE_RADIUS_VARIATION_FACTOR_RB) * 100) + 1) / 100.0f;
                circle.radius = baseRadius * radiusVariation;
                circle.radius = std::max(1.5f, circle.radius); 
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
        } else {
            _clouds[i].active = false;
        }
    }
    _lastCloudUpdateTimeRainbow = currentTime;
    if (_context.serialForwarder) _context.serialForwarder->printf("RainbowWeatherEffect: Clouds initialized (%d active).\n", numActiveClouds);
}

void RainbowWeatherEffect::updateCloudsRainbow(unsigned long currentTime) {
    if (!_context.renderer) return;
    if (currentTime - _lastCloudUpdateTimeRainbow < CLOUD_UPDATE_INTERVAL_MS_RB) {
        return;
    }
    _lastCloudUpdateTimeRainbow = currentTime;

    int screenW = _context.renderer->getWidth();
    int cloudMinY = 25; 
    int cloudMaxY = _context.renderer->getHeight() / 2 + 10;

    for (int i = 0; i < MAX_CLOUDS_RAINBOW; ++i) {
        if (_clouds[i].active) {
            _clouds[i].x += _clouds[i].speed + _currentWindFactor * 0.1f; 
            float cloudActualLeftEdge = _clouds[i].x + _clouds[i].minRelativeX;
            if (cloudActualLeftEdge > screenW + CLOUD_WRAP_BUFFER_RB) {
                _clouds[i].x = -(_clouds[i].maxRelativeX + random(5, CLOUD_WRAP_BUFFER_RB * 2));
                _clouds[i].y = (float)random(cloudMinY, cloudMaxY + 1);
                _clouds[i].speed = (float)random(CLOUD_SPEED_MIN_TENTHS_RB, CLOUD_SPEED_MAX_TENTHS_RB + 1) / 10.0f;
            } else if (_clouds[i].x + _clouds[i].maxRelativeX < -CLOUD_WRAP_BUFFER_RB) {
                _clouds[i].x = screenW - _clouds[i].minRelativeX + random(5, CLOUD_WRAP_BUFFER_RB * 2);
                _clouds[i].y = (float)random(cloudMinY, cloudMaxY + 1);
                _clouds[i].speed = (float)random(CLOUD_SPEED_MIN_TENTHS_RB, CLOUD_SPEED_MAX_TENTHS_RB + 1) / 10.0f;
            }
        }
    }
}

void RainbowWeatherEffect::drawCloudsRainbow() {
    if (!_context.renderer || !_context.display) return;
    U8G2* u8g2 = _context.display;
    Renderer& renderer = *_context.renderer;

    uint8_t originalColor = u8g2->getDrawColor();
    int screenTopOffset = renderer.getYOffset();
    // int cloudBottomRenderLimit_abs = renderer.getYOffset() + renderer.getHeight();  // Not strictly used with current logic

    for (int i = 0; i < MAX_CLOUDS_RAINBOW; ++i) {
        if (_clouds[i].active) {
            int cloudBaseLineY_abs = screenTopOffset + static_cast<int>(round(_clouds[i].y)); 
            for (const auto& circle : _clouds[i].circles) {
                float r = circle.radius; if (r < 1.0f) r = 1.0f;
                int R_int = static_cast<int>(round(r));
                int absCircleCenterX = renderer.getXOffset() + static_cast<int>(round(_clouds[i].x + circle.relativeX));
                int absCircleCenterY = cloudBaseLineY_abs + static_cast<int>(round(circle.relativeY)); 

                bool overRainbow = isCoordOverRainbow(absCircleCenterX, absCircleCenterY);
                u8g2->setDrawColor(overRainbow ? 2 : 1); 

                for (int y_scan_offset = -R_int; y_scan_offset <= R_int; ++y_scan_offset) {
                    int current_y_scan_abs = absCircleCenterY + y_scan_offset;
                    
                    if (current_y_scan_abs >= cloudBaseLineY_abs + R_int) continue; 
                    if (current_y_scan_abs < screenTopOffset) continue;
                    if (current_y_scan_abs >= screenTopOffset + renderer.getHeight()) continue;

                    float x_span_squared = (r * r) - (y_scan_offset * y_scan_offset);
                    if (x_span_squared < 0) continue;
                    float x_span_float = sqrtf(x_span_squared);
                    if (isnan(x_span_float)) continue;
                    int x_span_int = static_cast<int>(round(x_span_float));

                    if (x_span_int > 0) {
                        u8g2->drawHLine(absCircleCenterX - x_span_int, current_y_scan_abs, x_span_int * 2 + 1);
                    } else { 
                        u8g2->drawPixel(absCircleCenterX, current_y_scan_abs); 
                    }
                }
            }
        }
    }
    u8g2->setDrawColor(originalColor);
}

void RainbowWeatherEffect::drawBackground() {
    drawCloudsRainbow();      
    drawProgrammaticRainbow(); 
}

void RainbowWeatherEffect::drawForeground() {
    if (_sparkleParticleSystem) {
        _sparkleParticleSystem->draw();
    }
}

WeatherType RainbowWeatherEffect::getType() const {
    return WeatherType::RAINBOW;
}