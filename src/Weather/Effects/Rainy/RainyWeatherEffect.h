#ifndef RAINY_WEATHER_EFFECT_H
#define RAINY_WEATHER_EFFECT_H

#include "../WeatherEffectBase.h"
// GameContext is included via WeatherEffectBase.h

class RainyWeatherEffect : public WeatherEffectBase {
public:
    // Constructor now takes GameContext
    RainyWeatherEffect(GameContext& context, bool isHeavy); 
    ~RainyWeatherEffect() override;

    void init(unsigned long currentTime) override;
    void update(unsigned long currentTime) override;
    void drawBackground() override; 
    void drawForeground() override;
    WeatherType getType() const override;

private:
    struct RainDrop { int x; int y; int speed; };
    static const int MAX_RAIN_DROPS = 30; 
    RainDrop* _rainDrops = nullptr; // MODIFIED: Changed from static array to pointer
    bool _isHeavyRain;
    
    void initRainDrops();
    void updateRainDrops();
    void drawRain(uint8_t speed_factor);
};

#endif // RAINY_WEATHER_EFFECT_H