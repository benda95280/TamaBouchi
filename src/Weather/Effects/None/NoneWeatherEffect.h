#ifndef NONE_WEATHER_EFFECT_H
#define NONE_WEATHER_EFFECT_H

#include "../WeatherEffectBase.h"
// GameContext is included via WeatherEffectBase.h

class NoneWeatherEffect : public WeatherEffectBase
{
public:
    // Constructor now takes GameContext
    NoneWeatherEffect(GameContext &context);
    ~NoneWeatherEffect() override = default;

    void init(unsigned long currentTime) override;
    void update(unsigned long currentTime) override;
    void drawBackground() override;
    void drawForeground() override;
    WeatherType getType() const override;
};

#endif // NONE_WEATHER_EFFECT_H