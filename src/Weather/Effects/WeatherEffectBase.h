#ifndef WEATHER_EFFECT_BASE_H
#define WEATHER_EFFECT_BASE_H

// #include "../../../lib/EDGE/src/Renderer.h" // Renderer access via GameContext
// #include "../../../src/GameStats.h"         // GameStats access via GameContext
// #include "../../../src/SerialForwarder.h"   // SerialForwarder access via GameContext
#include "../WeatherTypes.h"               
#include "../../System/GameContext.h" // <<< NEW INCLUDE

class WeatherEffectBase {
public:
    // Constructor now takes GameContext
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

protected:
    GameContext& _context; // Store reference to GameContext
    // Renderer& _renderer; // Access via _context.renderer
    // GameStats* _gameStats_ptr; // Access via _context.gameStats
    // SerialForwarder* _logger; // Access via _context.serialForwarder
    
    RainIntensityState _intensityState; 
    uint8_t _particleDensity = 0;
    float _currentWindFactor = 0.0f;
};

#endif // WEATHER_EFFECT_BASE_H