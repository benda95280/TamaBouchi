#ifndef WEATHER_MANAGER_H
#define WEATHER_MANAGER_H

#include "Renderer.h" 
#include "WeatherTypes.h"             
#include <Arduino.h>
#include <memory> 
#include <vector>
#include "Effects/BirdManager.h" 
#include "../System/GameContext.h" 

// Forward declaration
class WeatherEffectBase; 

struct WeatherInfo {
    WeatherType type;
    uint8_t chance;
    unsigned long minDurationMs;
    unsigned long maxDurationMs;
};

class WeatherManager {
public:
    WeatherManager(GameContext& context);
    ~WeatherManager(); 

    void init();
    void update(unsigned long currentTime);
    void drawBackground(bool allowDrawing);
    void drawForeground(bool allowDrawing);
    void forceWeather(WeatherType type, unsigned long durationMs);
    void forceWind(float windFactor);
    void forceAddSecondaryEffect(const String& effectName);
    void forceSpawnBirds(int count);

    RainIntensityState getRainIntensityState() const;
    float getActualWindFactor() const;
    uint8_t getIntensityAdjustedDensity() const;
    String getActiveEffectsString() const;

    static const char* weatherTypeToString(WeatherType type);
    static const char* rainStateToString(RainIntensityState state);

    const uint8_t* getDefaultFont() const { return _defaultFont; }
    void setDefaultFont(const uint8_t* font) { _defaultFont = font; }


private:
    GameContext& _context;
    const uint8_t* _defaultFont = u8g2_font_5x7_tf;

    std::vector<std::unique_ptr<WeatherEffectBase>> _activeEffects;
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
    static const unsigned long WIND_TRANSITION_DURATION_MS = 15000;
    
    unsigned long _lastStatImpactTime = 0;
    static const unsigned long STAT_IMPACT_INTERVAL_MS = 5000;

    WeatherType _pendingNextWeatherType = WeatherType::NONE;
    bool _isFadingOut = false;
    static const unsigned long FADEOUT_DURATION_MS = 30000;

    void updateWeatherState(unsigned long currentTime);
    void updateWind(unsigned long currentTime);
    void selectNextWeather(unsigned long currentTime);
    void applyStatImpacts(unsigned long currentTime);
    void updateParticleDensity(unsigned long currentTime);
    void updateWeatherComposition(WeatherType type);
    WeatherType peekNextWeatherType() const;
};

#endif // WEATHER_MANAGER_H