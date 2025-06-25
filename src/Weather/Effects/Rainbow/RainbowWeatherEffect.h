#ifndef RAINBOW_WEATHER_EFFECT_H
#define RAINBOW_WEATHER_EFFECT_H

#include "../WeatherEffectBase.h"
#include <vector> // For cloud circles
#include "../../../ParticleSystem.h" // For sparkle effect
#include <memory> // For unique_ptr

class RainbowWeatherEffect : public WeatherEffectBase {
public:
    RainbowWeatherEffect(GameContext& context);
    ~RainbowWeatherEffect() override = default;

    void init(unsigned long currentTime) override;
    void update(unsigned long currentTime) override;
    void drawBackground() override;
    void drawForeground() override;
    WeatherType getType() const override;

private:
    // Rainbow specific members
    int _rainbowCenterX;
    int _rainbowCenterY;
    int _rainbowStartRadius;
    int _rainbowBandThickness;
    int _numRainbowBands;
    int _rainbowBandSpacing; 

    // Cloud specific members
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

    static const int MAX_CLOUDS_RAINBOW = 8; 
    static const int MIN_ACTIVE_CLOUDS_RAINBOW = 3; 
    Cloud _clouds[MAX_CLOUDS_RAINBOW];
    unsigned long _lastCloudUpdateTimeRainbow = 0;

    // Cloud generation parameters
    static const int MIN_CIRCLES_PER_CLOUD_RB = 3;
    static const int MAX_CIRCLES_PER_CLOUD_RB = 5;
    static constexpr float MIN_BASE_CLOUD_RADIUS_RB = 2.0f;
    static constexpr float MAX_BASE_CLOUD_RADIUS_RB = 4.0f;
    static constexpr float MAX_CIRCLE_OFFSET_X_FACTOR_RB = 1.3f;
    static constexpr float MAX_CIRCLE_OFFSET_Y_FACTOR_ABOVE_RB = 1.5f;
    static constexpr float MAX_CIRCLE_OFFSET_Y_FACTOR_BELOW_RB = 0.1f;
    static constexpr float MIN_CIRCLE_RADIUS_VARIATION_FACTOR_RB = 0.8f;
    static constexpr float MAX_CIRCLE_RADIUS_VARIATION_FACTOR_RB = 1.2f;
    static const int CLOUD_SPEED_MIN_TENTHS_RB = 1;
    static const int CLOUD_SPEED_MAX_TENTHS_RB = 3;
    static const unsigned long CLOUD_UPDATE_INTERVAL_MS_RB = 70;
    static const int CLOUD_WRAP_BUFFER_RB = 10;
    static const int CLOUD_VERTICAL_OFFSET_RB = 2;

    // Sparkle Effect
    std::unique_ptr<ParticleSystem> _sparkleParticleSystem;
    unsigned long _lastSparkleSpawnTime = 0;
    static const unsigned long SPARKLE_SPAWN_INTERVAL_MS = 600; // Slower spawn rate
    static const int MAX_SPARKLES_PER_BURST = 2; // Fewer sparkles per burst


    void initCloudsRainbow(unsigned long currentTime);
    void updateCloudsRainbow(unsigned long currentTime);
    void drawCloudsRainbow();
    void drawProgrammaticRainbow();
    void updateSparkles(unsigned long currentTime);
    bool isCoordOverRainbow(int x, int y) const; 
};

#endif // RAINBOW_WEATHER_EFFECT_H