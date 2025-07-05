#ifndef FOG_WEATHER_EFFECT_H
#define FOG_WEATHER_EFFECT_H

#include "../WeatherEffectBase.h"
#include "FastNoiseLite.h"

class FogWeatherEffect : public WeatherEffectBase {
public:
    FogWeatherEffect(GameContext& context);
    ~FogWeatherEffect() override = default;

    void init(unsigned long currentTime) override;
    void update(unsigned long currentTime) override;
    void drawBackground() override;
    void drawForeground() override;
    WeatherType getType() const override;

    void startFadeOut(unsigned long duration) override;

private:
    FastNoiseLite _fogNoise;
    FastNoiseLite _edgeNoise; // For creating a soft top edge

    float _noiseOffsets[3]; // For 3 layers of fog
    float _edgeNoiseOffset = 0.0f;
    float _scrollSpeeds[3];
    float _densityThreshold;
    
    // Fade state
    bool _isFadingIn = false;
    bool _isFadingOut = false;
    unsigned long _fadeInStartTime = 0;
    unsigned long _fadeOutStartTime = 0;
    unsigned long _fadeInDuration = 0;
    unsigned long _fadeOutDuration = 0;
    float _fadeStartDensity = 0.0f;

    // Constants
    static constexpr float BASE_DENSITY_THRESHOLD = 0.45f;
    static constexpr float FADE_IN_START_THRESHOLD = 1.1f;
    static constexpr float FADE_OUT_END_THRESHOLD = 1.1f;
    static const unsigned long FADE_IN_DURATION_MS = 2000;
};

#endif // FOG_WEATHER_EFFECT_H