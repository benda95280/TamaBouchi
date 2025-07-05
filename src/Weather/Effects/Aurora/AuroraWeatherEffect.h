#ifndef AURORA_WEATHER_EFFECT_H
#define AURORA_WEATHER_EFFECT_H

#include "../WeatherEffectBase.h"

class AuroraWeatherEffect : public WeatherEffectBase {
public:
    AuroraWeatherEffect(GameContext& context);
    ~AuroraWeatherEffect() override = default;

    void init(unsigned long currentTime) override;
    void update(unsigned long currentTime) override;
    void drawBackground() override;
    void drawForeground() override;
    WeatherType getType() const override;

    void startFadeOut(unsigned long duration) override;

private:
    struct Curtain {
        float baseY; // Changed from baseX
        float amplitude;
        float frequency;
        float speed;
        float timeOffset;
        float thickness;
    };

    static const int MAX_CURTAINS = 3;
    Curtain _curtains[MAX_CURTAINS];
    
    float _globalAlpha; // 0.0 (transparent) to 1.0 (opaque)
    bool _isFadingOut = false;
    unsigned long _fadeOutStartTime = 0;
    unsigned long _fadeOutDuration = 0;
};

#endif // AURORA_WEATHER_EFFECT_H