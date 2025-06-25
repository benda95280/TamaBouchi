#ifndef CLOUDY_WEATHER_EFFECT_H
#define CLOUDY_WEATHER_EFFECT_H

#include "../WeatherEffectBase.h"
#include <vector>
// GameContext is included via WeatherEffectBase.h

class CloudyWeatherEffect : public WeatherEffectBase {
public:
    // Constructor now takes GameContext
    CloudyWeatherEffect(GameContext& context); 
    ~CloudyWeatherEffect() override = default;

    void init(unsigned long currentTime) override;
    void update(unsigned long currentTime) override;
    void drawBackground() override;
    void drawForeground() override;
    WeatherType getType() const override;

private:
    struct CloudCircle {
        float relativeX;
        float relativeY;
        float radius;
    };
    struct Cloud {
        float x;
        float y;
        float speed;
        std::vector<CloudCircle> circles;
        bool active;
        float minRelativeX;
        float maxRelativeX;
        float minRelativeY;
        float maxRelativeY;
    };

    static const int MAX_CLOUDS = 3;
    Cloud _clouds[MAX_CLOUDS];
    unsigned long _lastCloudUpdateTime = 0;

    static const int MIN_CIRCLES_PER_CLOUD = 4;
    static const int MAX_CIRCLES_PER_CLOUD = 8;
    static constexpr float MIN_BASE_CLOUD_RADIUS = 2.5f;
    static constexpr float MAX_BASE_CLOUD_RADIUS = 5.5f;
    static constexpr float MAX_CIRCLE_OFFSET_X_FACTOR = 1.5f;
    static constexpr float MAX_CIRCLE_OFFSET_Y_FACTOR_ABOVE = 2.0f;
    static constexpr float MAX_CIRCLE_OFFSET_Y_FACTOR_BELOW = 0.2f;
    static constexpr float MIN_CIRCLE_RADIUS_VARIATION_FACTOR = 0.7f;
    static constexpr float MAX_CIRCLE_RADIUS_VARIATION_FACTOR = 1.3f;
    static const int CLOUD_SPEED_MIN_TENTHS = 1;
    static const int CLOUD_SPEED_MAX_TENTHS = 4;
    static const unsigned long CLOUD_UPDATE_INTERVAL_MS = 50;
    static const int CLOUD_WRAP_BUFFER = 10;
    static const int CLOUD_VERTICAL_OFFSET = -2;
};

#endif // CLOUDY_WEATHER_EFFECT_H