#ifndef STORM_WEATHER_EFFECT_H
#define STORM_WEATHER_EFFECT_H

#include "../WeatherEffectBase.h"
#include "../../../Animator.h" 
#include <vector>
#include <memory>
#include "../../../System/GameContext.h"

class StormWeatherEffect : public WeatherEffectBase {
public:
    StormWeatherEffect(GameContext& context); 
    ~StormWeatherEffect() override = default;

    void init(unsigned long currentTime) override;
    void update(unsigned long currentTime) override;
    void drawBackground() override; 
    void drawForeground() override;
    WeatherType getType() const override;

private:
    struct RainDrop { int x; int y; int speed; };
    static const int MAX_RAIN_DROPS_STORM = 30; 
    RainDrop _rainDrops[MAX_RAIN_DROPS_STORM];

    struct LightningStrike {
        std::unique_ptr<Animator> animator;
        int x = 0; int y = 0; StrikeType type; bool screenFlash = false;
    };
    static const int MAX_SIMULTANEOUS_STRIKES = 3;
    std::vector<LightningStrike> _activeStrikes;
    unsigned long _lastStrikeTriggerTime = 0;
    static const unsigned long MIN_TIME_BETWEEN_STRIKES_MS = 300;
    unsigned long _strikeCountLast10s = 0;
    unsigned long _strikeCountLastMinute = 0;
    unsigned long _last10sBoundary = 0;
    unsigned long _lastMinuteBoundary = 0;
    static const int MAX_STRIKES_PER_10S = 5;
    static const int MAX_STRIKES_PER_MINUTE = 8;
    unsigned long _fullscreenStrikeCountLastMinute = 0;
    unsigned long _lastMinuteBoundaryFullscreen = 0;
    static const int MAX_FULLSCREEN_STRIKES_PER_MINUTE = 2;
    static const unsigned long STRIKE_ANIM_FRAME_DURATION_MS = 80;

    void initRainDrops();
    void updateRainDrops();
    void drawRain();

    void triggerLightning(unsigned long currentTime);
    void updateLightningStrikes();
    void drawLightningStrikes();
    bool isFlashing() const; 
};

#endif // STORM_WEATHER_EFFECT_H