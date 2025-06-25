#ifndef HEAVY_RAIN_WEATHER_EFFECT_H
#define HEAVY_RAIN_WEATHER_EFFECT_H

#include "../Rainy/RainyWeatherEffect.h" 
// GameContext is included via WeatherEffectBase.h -> RainyWeatherEffect.h

class HeavyRainWeatherEffect : public RainyWeatherEffect {
public:
    // Constructor now takes GameContext
    HeavyRainWeatherEffect(GameContext& context); 
    ~HeavyRainWeatherEffect() override = default;

    // Overrides are not strictly necessary if RainyWeatherEffect handles "isHeavy" correctly
    // void init(unsigned long currentTime) override; // Already handled by RainyWeatherEffect if isHeavy logic is there
    // void update(unsigned long currentTime) override; // Already handled by RainyWeatherEffect
    // void drawForeground() override; // Already handled by RainyWeatherEffect
    WeatherType getType() const override;
};

#endif // HEAVY_RAIN_WEATHER_EFFECT_H