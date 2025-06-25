#ifndef WEATHER_MANAGER_H
#define WEATHER_MANAGER_H

#include "Renderer.h" 
#include "WeatherTypes.h"             
#include <Arduino.h>
#include <memory> 
#include <vector>
#include "Effects/BirdManager.h" 
#include "../System/GameContext.h" // <<< NEW INCLUDE

// Forward declaration
// struct GameStats; // Now in GameContext
// class Renderer;   // Now in GameContext
class WeatherEffectBase; 
// class SerialForwarder; // Now in GameContext

struct WeatherInfo {
    WeatherType type;
    uint8_t chance;
    unsigned long minDurationMs;
    unsigned long maxDurationMs;
};

class WeatherManager {
public:
    WeatherManager(GameContext& context); // Takes GameContext
    ~WeatherManager(); 

    void init();
    void update(unsigned long currentTime);
    void drawBackground(bool allowDrawing);
    void drawForeground(bool allowDrawing);
    void forceWeather(WeatherType type, unsigned long durationMs);

    RainIntensityState getRainIntensityState() const;
    float getActualWindFactor() const;
    uint8_t getIntensityAdjustedDensity() const;

    static const char* weatherTypeToString(WeatherType type);
    static const char* rainStateToString(RainIntensityState state);

    const uint8_t* getDefaultFont() const { return _defaultFont; }
    void setDefaultFont(const uint8_t* font) { _defaultFont = font; }


private:
    GameContext& _context; // Store reference to GameContext
    // Renderer& _renderer; // Access via _context.renderer
    // GameStats* _gameStats_ptr; // Access via _context.gameStats
    const uint8_t* _defaultFont = u8g2_font_5x7_tf;

    std::unique_ptr<WeatherEffectBase> _currentWeatherEffect;
    std::unique_ptr<BirdManager> _birdManager; 

    unsigned long _currentWeatherStartTime = 0;
    unsigned long _currentWeatherDuration = 0;
    RainIntensityState _rainIntensityState = RainIntensityState::NONE;
    uint8_t _currentParticleDensity = 0;

    float _actualWindFactor = 0.0f;
    float _targetWindFactor = 0.0f;
    unsigned long _windChangeStartTime = 0;
    unsigned long _nextWindTargetTime = 0;
    static const unsigned long WIND_TARGET_INTERVAL_MS = 60000;
    static const unsigned long WIND_TRANSITION_DURATION_MS = 10000;
    
    unsigned long _lastStatImpactTime = 0;
    static const unsigned long STAT_IMPACT_INTERVAL_MS = 5000;


    void updateWeatherState(unsigned long currentTime);
    void updateWind(unsigned long currentTime);
    void selectNextWeather(unsigned long currentTime);
    void applyStatImpacts(unsigned long currentTime);
    void updateParticleDensity(WeatherType type);
    void createWeatherEffect(WeatherType type);
};

#endif // WEATHER_MANAGER_H