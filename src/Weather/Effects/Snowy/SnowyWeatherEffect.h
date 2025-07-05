#ifndef SNOWY_WEATHER_EFFECT_H
#define SNOWY_WEATHER_EFFECT_H

#include "../WeatherEffectBase.h"

class SnowyWeatherEffect : public WeatherEffectBase {
public:
    SnowyWeatherEffect(GameContext& context, bool isHeavy); 
    ~SnowyWeatherEffect() override = default;

    void init(unsigned long currentTime) override;
    void update(unsigned long currentTime) override;
    void drawBackground() override; 
    void drawForeground() override;
    WeatherType getType() const override;

private:
    struct SnowFlake {
        float x;
        float y;
        float speedY;
        float speedXFactor;
        char displayChar;
        // Growth properties
        bool canGrow;
        bool isGrowing;
        float currentSizeMultiplier;
        unsigned long nextSizeChangeTime;
    };
    
    static const int MAX_SNOW_FLAKES = 40;
    SnowFlake _snowFlakes[MAX_SNOW_FLAKES];
    bool _isHeavySnow;
    const uint8_t* _snowFont = nullptr;
    const uint8_t* _largeSnowFont = nullptr;

    void initSnowFlakes();
    void updateSnowFlakes(unsigned long currentTime);
    void drawSnow();
};

#endif // SNOWY_WEATHER_EFFECT_H