#include "FogWeatherEffect.h"
#include <U8g2lib.h>
#include <algorithm>
#include <cmath>
#include "../../../System/GameContext.h"
#include "../../../DebugUtils.h"
#include "Renderer.h"

FogWeatherEffect::FogWeatherEffect(GameContext& context)
    : WeatherEffectBase(context) {
    _fogNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    _fogNoise.SetFrequency(0.08f);
    _fogNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    _fogNoise.SetFractalOctaves(2);
    _fogNoise.SetFractalLacunarity(2.0f);
    _fogNoise.SetFractalGain(0.5f);

    _edgeNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    _edgeNoise.SetFrequency(0.04f); // Low frequency for smooth top edge
    _edgeNoise.SetFractalOctaves(1);
}

void FogWeatherEffect::init(unsigned long currentTime) {
    _fogNoise.SetSeed(random(0, 10000));
    _noiseOffsets[0] = (float)random(0, 500);
    _noiseOffsets[1] = (float)random(1000, 1500);
    _noiseOffsets[2] = (float)random(2000, 2500);

    // Speeds for parallax effect - increased base speed
    _scrollSpeeds[0] = 0.02f; 
    _scrollSpeeds[1] = 0.032f;
    _scrollSpeeds[2] = 0.048f;

    _edgeNoise.SetSeed(random(10000, 20000));
    _edgeNoiseOffset = (float)random(0, 500);

    _isFadingIn = true;
    _isFadingOut = false;
    _fadeInStartTime = currentTime;
    _fadeInDuration = FADE_IN_DURATION_MS;
    _densityThreshold = FADE_IN_START_THRESHOLD;

    debugPrint("WEATHER", "FogWeatherEffect initialized for fade-in.");
}

void FogWeatherEffect::startFadeOut(unsigned long duration) {
    if (_isFadingOut) return;
    _isFadingOut = true;
    _isFadingIn = false; // Stop any ongoing fade-in
    _fadeOutStartTime = millis();
    _fadeOutDuration = duration;
    _fadeStartDensity = _densityThreshold; // Store current density to fade from
    debugPrintf("WEATHER", "Fog: Fade-out started. From density %.2f over %lu ms.", _fadeStartDensity, _fadeOutDuration);
}

void FogWeatherEffect::update(unsigned long currentTime) {
    if (_isFadingIn) {
        unsigned long elapsedTime = currentTime - _fadeInStartTime;
        if (elapsedTime >= _fadeInDuration) {
            _densityThreshold = BASE_DENSITY_THRESHOLD;
            _isFadingIn = false;
        } else {
            float progress = (float)elapsedTime / (float)_fadeInDuration;
            // Interpolate from high threshold (invisible) to base threshold (visible)
            _densityThreshold = FADE_IN_START_THRESHOLD + (BASE_DENSITY_THRESHOLD - FADE_IN_START_THRESHOLD) * progress;
        }
    } else if (_isFadingOut) {
        unsigned long elapsedTime = currentTime - _fadeOutStartTime;
        if (elapsedTime >= _fadeOutDuration) {
            _densityThreshold = FADE_OUT_END_THRESHOLD; // Effectively invisible
        } else {
            float progress = (float)elapsedTime / (float)_fadeOutDuration;
            // Interpolate from the density when fade-out started to high threshold (invisible)
            _densityThreshold = _fadeStartDensity + (FADE_OUT_END_THRESHOLD - _fadeStartDensity) * progress;
        }
    } else {
        _densityThreshold = BASE_DENSITY_THRESHOLD; // Maintain base density if not fading
    }

    // Fog scrolls with wind and on its own, with parallax effect on wind
    for (int i = 0; i < 3; ++i) {
        // Closer layers (higher index) are affected more by wind
        float windInfluence = 0.025f * (float)(i + 1);
        // Wind direction corrected: subtract wind factor to move fog with the wind
        _noiseOffsets[i] += (_scrollSpeeds[i] - _currentWindFactor * windInfluence);
    }
    // Update edge noise offset as well, so the top of the fog drifts correctly
    _edgeNoiseOffset += (0.04f - _currentWindFactor * 0.03f);
}

void FogWeatherEffect::drawBackground() {
    // Fog is drawn in the foreground to obscure objects
}

void FogWeatherEffect::drawForeground() {
    if (!_context.renderer || !_context.display || _densityThreshold >= 1.0f) return;
    U8G2* u8g2 = _context.display;
    Renderer& renderer = *_context.renderer;

    uint8_t originalColor = u8g2->getDrawColor();
    u8g2->setDrawColor(2); // Use XOR for transparency

    // Define the vertical zone where fog can appear and fade.
    const int baseFogY = renderer.getHeight() / 2 - 5;
    const int fogHeightVariation = 15;
    const int fogVerticalFadeHeight = 10;
    const int step = 2; // Use a 2x2 grid for dithering

    float scales[] = {0.04f, 0.08f, 0.12f};
    
    // Iterate by columns (x) first to determine the start Y for each column
    for (int x = 0; x < renderer.getWidth(); x += step) {
        // Calculate the wavy top edge of the fog for this column 'x'
        float edgeNoiseVal = (_edgeNoise.GetNoise(x * 0.05f + _edgeNoiseOffset, 0.0f) + 1.0f) * 0.5f; // Normalize to 0-1
        int columnStartY = baseFogY + (int)(edgeNoiseVal * fogHeightVariation);
        
        // Now iterate downwards from that start Y for this column
        for (int y = columnStartY; y < renderer.getHeight(); y += step) {
            if (y < 0) continue; 

            // Calculate a vertical fade factor based on distance from the wavy top.
            float verticalFadeFactor = 1.0f;
            int distanceFromWavyTop = y - columnStartY;
            if (distanceFromWavyTop < fogVerticalFadeHeight && fogVerticalFadeHeight > 0) {
                verticalFadeFactor = (float)distanceFromWavyTop / (float)fogVerticalFadeHeight;
            }

            // Combine noise from multiple layers for a more volumetric feel
            // NORMALIZE NOISE from [-1, 1] to [0, 1] range
            float n1 = (_fogNoise.GetNoise(x * scales[0] + _noiseOffsets[0], y * scales[0]) + 1.0f) * 0.5f;
            float n2 = (_fogNoise.GetNoise(x * scales[1] + _noiseOffsets[1], y * scales[1]) + 1.0f) * 0.5f;
            
            // Weighted average of noise values
            float combinedNoise = (n1 * 0.6f) + (n2 * 0.4f);
            
            if (combinedNoise > _densityThreshold) {
                // Calculate base intensity based on noise
                float baseIntensity = (combinedNoise - _densityThreshold) / (1.0f - _densityThreshold);
                
                // Modulate intensity with the vertical fade factor
                float finalIntensity = baseIntensity * verticalFadeFactor;
                finalIntensity = std::min(1.0f, finalIntensity);

                if (finalIntensity > 0.0f) {
                    // Determine number of random pixels to draw in this cell based on final intensity
                    int pixelsToDraw = static_cast<int>(round(finalIntensity * (step * step * 0.75f)));

                    // Draw the random pixels
                    for (int p = 0; p < pixelsToDraw; ++p) {
                        int px = x + random(0, step); 
                        int py = y + random(0, step); 
                        if (px < renderer.getWidth() && py < renderer.getHeight()) {
                            u8g2->drawPixel(renderer.getXOffset() + px, renderer.getYOffset() + py);
                        }
                    }
                }
            }
        }
    }
    
    u8g2->setDrawColor(originalColor);
}

WeatherType FogWeatherEffect::getType() const {
    return WeatherType::FOG;
}