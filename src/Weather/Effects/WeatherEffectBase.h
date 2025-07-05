#ifndef WEATHER_EFFECT_BASE_H
#define WEATHER_EFFECT_BASE_H

#include "../WeatherTypes.h"               
#include "../../System/GameContext.h"

class WeatherEffectBase {
public:
    WeatherEffectBase(GameContext& context);
    virtual ~WeatherEffectBase() = default;

    virtual void init(unsigned long currentTime) = 0;
    virtual void update(unsigned long currentTime) = 0;
    virtual void drawBackground() = 0;
    virtual void drawForeground() = 0;
    virtual WeatherType getType() const = 0;

    virtual void setWindFactor(float windFactor) { _currentWindFactor = windFactor; }
    virtual void setIntensityState(RainIntensityState intensityState) { _intensityState = intensityState; }
    virtual void setParticleDensity(uint8_t density) { _particleDensity = density; }

    // New virtual method for fade-out transitions
    virtual void startFadeOut(unsigned long duration) {}

protected:
    GameContext& _context;
    
    RainIntensityState _intensityState; 
    uint8_t _particleDensity = 0;
    float _currentWindFactor = 0.0f;
};

#endif // WEATHER_EFFECT_BASE_H