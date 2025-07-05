#ifndef WINDY_WEATHER_EFFECT_H
#define WINDY_WEATHER_EFFECT_H

#include "../WeatherEffectBase.h"

class WindyWeatherEffect : public WeatherEffectBase {
public:
    WindyWeatherEffect(GameContext& context);
    ~WindyWeatherEffect() override = default;

    void init(unsigned long currentTime) override;
    void update(unsigned long currentTime) override;
    void drawBackground() override;
    void drawForeground() override;
    WeatherType getType() const override;

private:
    struct WindLine { int x1, y1, x2, y2; };
    static const int MAX_WIND_LINES = 10;
    WindLine _windLines[MAX_WIND_LINES];

    void initWindLines();
    void updateWindLines();
    void drawWindLines();
};

#endif // WINDY_WEATHER_EFFECT_H