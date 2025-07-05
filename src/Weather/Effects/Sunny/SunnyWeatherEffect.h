#ifndef SUNNY_WEATHER_EFFECT_H
#define SUNNY_WEATHER_EFFECT_H

#include "../WeatherEffectBase.h"
// GameContext will be included via WeatherEffectBase.h

class SunnyWeatherEffect : public WeatherEffectBase {
public:
    // Constructor now takes GameContext
    SunnyWeatherEffect(GameContext& context);
    ~SunnyWeatherEffect() override = default;

    void init(unsigned long currentTime) override;
    void update(unsigned long currentTime) override;
    void drawBackground() override;
    void drawForeground() override;
    WeatherType getType() const override;

    void startFadeOut(unsigned long duration) override;

private:
    float _sunCenterX;
    float _sunCenterY;
    int _sunStartRadius;
    bool _sunIsLeft = true;
    float _sunRayAnimationPhase = 0.0f;
    unsigned long _lastSunRayUpdateTime = 0;

    // Fade out members
    bool _isFadingOut = false;
    unsigned long _fadeOutStartTime = 0;
    unsigned long _fadeOutDuration = 0;
    float _initialFadeX = 0;
    float _targetFadeX = 0;


    static const int SUN_INNER_RADIUS = 7;
    static const int SUN_OUTER_RADIUS = 15;
    static const int SUN_NUM_RAYS = 8;
    static constexpr float SUN_RAY_PULSATION_SPEED = 0.025f;
    static constexpr float SUN_RAY_LENGTH_VARIATION = 2.0f;
    static const int SUN_X_OFFSET_FROM_EDGE = 0;
    static const int SUN_Y_OFFSET_FROM_TOP = 5;
};

#endif // SUNNY_WEATHER_EFFECT_H