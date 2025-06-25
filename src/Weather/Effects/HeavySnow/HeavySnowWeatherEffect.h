#ifndef HEAVY_SNOW_WEATHER_EFFECT_H
#define HEAVY_SNOW_WEATHER_EFFECT_H

#include "../Snowy/SnowyWeatherEffect.h" 
// GameContext is included via WeatherEffectBase.h -> SnowyWeatherEffect.h

class HeavySnowWeatherEffect : public SnowyWeatherEffect {
public:
    // Constructor now takes GameContext
    HeavySnowWeatherEffect(GameContext& context); 
    ~HeavySnowWeatherEffect() override = default;
    WeatherType getType() const override;
};

#endif // HEAVY_SNOW_WEATHER_EFFECT_H   