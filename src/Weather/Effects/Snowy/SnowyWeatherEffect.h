// In src/Weather/Effects/Snowy/SnowyWeatherEffect.h
#ifndef SNOWY_WEATHER_EFFECT_H
#define SNOWY_WEATHER_EFFECT_H

#include "../WeatherEffectBase.h"
// GameContext is included via WeatherEffectBase.h

class SnowyWeatherEffect : public WeatherEffectBase {
public:
    // Constructor now takes GameContext
    SnowyWeatherEffect(GameContext& context, bool isHeavy); 
    ~SnowyWeatherEffect() override = default;

    void init(unsigned long currentTime) override;
    void update(unsigned long currentTime) override;
    void drawBackground() override; 
    void drawForeground() override;
    WeatherType getType() const override;

private:
    struct SnowFlake {
        float x; float y; float speedY; float speedXFactor; char displayChar;
    };
    static const int MAX_SNOW_FLAKES = 40;
    SnowFlake _snowFlakes[MAX_SNOW_FLAKES];
    bool _isHeavySnow;
    const uint8_t* _snowFont = nullptr;

    void initSnowFlakes();
    void updateSnowFlakes();
    void drawSnow(float speed_factor); // speed_factor might be unused if density controls everything
};

#endif // SNOWY_WEATHER_EFFECT_H