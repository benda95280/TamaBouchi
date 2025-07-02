#include "WeatherManager.h"
#include "../GameStats.h"
#include "../DebugUtils.h"
#include "WeatherTypes.h"
#include "../SerialForwarder.h"
#include <algorithm> 
#include <cmath>     
#include <U8g2lib.h> 
#include "Effects/None/NoneWeatherEffect.h"
#include "Effects/Sunny/SunnyWeatherEffect.h"
#include "Effects/Cloudy/CloudyWeatherEffect.h"
#include "Effects/Rainy/RainyWeatherEffect.h"
#include "Effects/HeavyRain/HeavyRainWeatherEffect.h"
#include "Effects/Snowy/SnowyWeatherEffect.h"
#include "Effects/HeavySnow/HeavySnowWeatherEffect.h"
#include "Effects/Storm/StormWeatherEffect.h"
#include "Effects/Rainbow/RainbowWeatherEffect.h"
#include "Effects/BirdManager.h" 
#include "../System/GameContext.h"
#include <map>


static const WeatherInfo weatherDefinitions[] = {
    {WeatherType::NONE,        60, 5 * 60000, 15 * 60000},
    {WeatherType::SUNNY,       15, 3 * 60000, 10 * 60000},
    {WeatherType::CLOUDY,      15, 4 * 60000, 12 * 60000}, 
    {WeatherType::RAINY,       10, 2 * 60000, 6 * 60000},
    {WeatherType::HEAVY_RAIN,   5, 2 * 60000, 5 * 60000},
    {WeatherType::SNOWY,       10, 3 * 60000, 8 * 60000},   
    {WeatherType::HEAVY_SNOW,   5, 2 * 60000, 6 * 60000},   
    {WeatherType::STORM,        5, 2 * 60000, 5 * 60000},
    {WeatherType::RAINBOW,      5, 1 * 60000, 2 * 60000}
};
static const size_t weatherDefinitionCount = sizeof(weatherDefinitions) / sizeof(weatherDefinitions[0]);

WeatherManager::WeatherManager(GameContext& context) :
    _context(context),
    _currentWeatherEffect(nullptr), 
    _birdManager(std::unique_ptr<BirdManager>(new BirdManager(*_context.renderer))), 
    _rainIntensityState(RainIntensityState::NONE),
    _actualWindFactor(0.0f),
    _targetWindFactor(0.0f),
    _windChangeStartTime(0),
    _nextWindTargetTime(0),
    _currentWeatherStartTime(0),
    _currentWeatherDuration(0),
    _defaultFont(u8g2_font_5x7_tf)
{
    debugPrint("WEATHER", "WeatherManager constructed with GameContext.");
}

WeatherManager::~WeatherManager() {
    debugPrint("WEATHER", "WeatherManager destructed.");
}

void WeatherManager::createWeatherEffect(WeatherType type) {
    debugPrintf("WEATHER", "WeatherManager: Creating effect for type %d\n", (int)type);
    switch (type) {
        case WeatherType::SUNNY:       _currentWeatherEffect.reset(new SunnyWeatherEffect(_context)); break;
        case WeatherType::CLOUDY:      _currentWeatherEffect.reset(new CloudyWeatherEffect(_context)); break;
        case WeatherType::RAINY:       _currentWeatherEffect.reset(new RainyWeatherEffect(_context, false)); break;
        case WeatherType::HEAVY_RAIN:  _currentWeatherEffect.reset(new HeavyRainWeatherEffect(_context)); break;
        case WeatherType::SNOWY:       _currentWeatherEffect.reset(new SnowyWeatherEffect(_context, false)); break;
        case WeatherType::HEAVY_SNOW:  _currentWeatherEffect.reset(new HeavySnowWeatherEffect(_context)); break;
        case WeatherType::STORM:       _currentWeatherEffect.reset(new StormWeatherEffect(_context)); break;
        case WeatherType::RAINBOW:     _currentWeatherEffect.reset(new RainbowWeatherEffect(_context)); break;
        case WeatherType::NONE:
        default:                      _currentWeatherEffect.reset(new NoneWeatherEffect(_context)); break;
    }
    if (_currentWeatherEffect) {
        unsigned long currentTime = millis();
        _currentWeatherEffect->init(currentTime);
        _currentWeatherEffect->setWindFactor(_actualWindFactor);
        _currentWeatherEffect->setIntensityState(_rainIntensityState);
        _currentWeatherEffect->setParticleDensity(_currentParticleDensity);
    } else {
        debugPrintf("WEATHER", "WeatherManager: ERROR - Failed to create weather effect for type %d\n", (int)type);
    }

    if (_birdManager) {
        bool birdsActive = false;
        switch(type) {
            case WeatherType::NONE: case WeatherType::CLOUDY: case WeatherType::RAINY: 
            case WeatherType::SNOWY: case WeatherType::SUNNY: birdsActive = true; break;
            default: birdsActive = false; break;
        }
        _birdManager->setActive(birdsActive);
    }
}


void WeatherManager::init() {
    if (!_context.gameStats) {
        debugPrint("WEATHER", "ERROR: WeatherManager init failed - GameStats (via context) is null.");
        return;
    }
    unsigned long currentTime = millis();
    _lastStatImpactTime = currentTime;
    _nextWindTargetTime = currentTime + random(WIND_TARGET_INTERVAL_MS/2, WIND_TARGET_INTERVAL_MS);

    if (_birdManager) _birdManager->init(currentTime); 

    if ((uint8_t)_context.gameStats->currentWeather >= weatherDefinitionCount ) {
        _context.gameStats->currentWeather = WeatherType::NONE;
        _context.gameStats->nextWeatherChangeTime = 0;
    }

    if (_context.gameStats->nextWeatherChangeTime == 0 || _context.gameStats->nextWeatherChangeTime <= currentTime) {
        debugPrint("WEATHER", "WeatherManager: Initializing first/next weather state.");
        selectNextWeather(currentTime);
    } else {
        unsigned long minDuration = 0, maxDuration = 0;
        for (size_t i = 0; i < weatherDefinitionCount; ++i) {
            if (weatherDefinitions[i].type == _context.gameStats->currentWeather) {
                minDuration = weatherDefinitions[i].minDurationMs;
                maxDuration = weatherDefinitions[i].maxDurationMs;
                break;
            }
        }
        unsigned long estimatedTotalDuration = (minDuration + maxDuration) / 2;
        if (estimatedTotalDuration == 0) estimatedTotalDuration = 5 * 60000;
        _currentWeatherDuration = estimatedTotalDuration;
        _currentWeatherStartTime = _context.gameStats->nextWeatherChangeTime - estimatedTotalDuration;
        if (_currentWeatherStartTime > currentTime) _currentWeatherStartTime = currentTime;

        updateParticleDensity(_context.gameStats->currentWeather);
        updateWeatherState(currentTime); 
        createWeatherEffect(_context.gameStats->currentWeather); 
        
        debugPrintf("WEATHER", "WeatherManager: Loaded weather %d, Est Start: %lu, End: %lu, Est Dur: %lu\n", (int)_context.gameStats->currentWeather, _currentWeatherStartTime, _context.gameStats->nextWeatherChangeTime, _currentWeatherDuration);
    }

     if (_context.gameStats->currentWeather == WeatherType::STORM) {
         _actualWindFactor = (float)random(-15, 16) / 10.0f;
         _targetWindFactor = _actualWindFactor;
         _windChangeStartTime = currentTime;
         if (_currentWeatherEffect) _currentWeatherEffect->setWindFactor(_actualWindFactor);
         if (_birdManager) _birdManager->setWindFactor(_actualWindFactor); 
         debugPrintf("WEATHER", "WeatherManager: Initial storm wind factor: %.1f\n", _actualWindFactor);
     } else {
         _actualWindFactor = 0.0f;
         _targetWindFactor = 0.0f;
         if (_currentWeatherEffect) _currentWeatherEffect->setWindFactor(_actualWindFactor);
         if (_birdManager) _birdManager->setWindFactor(_actualWindFactor); 
     }
     debugPrintf("WEATHER", "WeatherManager Initialized. Current weather: %d, Next change at: %lu\n", (int)_context.gameStats->currentWeather, _context.gameStats->nextWeatherChangeTime);
}

void WeatherManager::update(unsigned long currentTime) {
    if (!_context.gameStats) return;

    if (currentTime >= _context.gameStats->nextWeatherChangeTime) {
        selectNextWeather(currentTime);
    }

    updateWeatherState(currentTime); 
    updateWind(currentTime);         
    updateParticleDensity(_context.gameStats->currentWeather); 

    if (_currentWeatherEffect) {
        _currentWeatherEffect->setWindFactor(_actualWindFactor);
        _currentWeatherEffect->setIntensityState(_rainIntensityState);
        _currentWeatherEffect->setParticleDensity(_currentParticleDensity);
        _currentWeatherEffect->update(currentTime);
    }
    if (_birdManager) { 
        _birdManager->setWindFactor(_actualWindFactor);
        _birdManager->update(currentTime);
    }
}

void WeatherManager::drawBackground(bool allowDrawing) {
    if (!allowDrawing || !_context.gameStats) return; 
    
    if (_currentWeatherEffect) {
        _currentWeatherEffect->drawBackground();
    }
}

void WeatherManager::drawForeground(bool allowDrawing) {
    if (!allowDrawing || !_context.gameStats) return; 
    
    if (_currentWeatherEffect) {
        _currentWeatherEffect->drawForeground();
    }
    if (_birdManager) { 
        _birdManager->draw();
    }
}

RainIntensityState WeatherManager::getRainIntensityState() const { return _rainIntensityState; }
float WeatherManager::getActualWindFactor() const { return _actualWindFactor; }
uint8_t WeatherManager::getIntensityAdjustedDensity() const {
    uint8_t d = _currentParticleDensity;
    if (_rainIntensityState == RainIntensityState::STARTING || _rainIntensityState == RainIntensityState::ENDING) d /= 2;
    if (_currentParticleDensity > 0 && d == 0 && (_rainIntensityState == RainIntensityState::STARTING || _rainIntensityState == RainIntensityState::ENDING)) d = 1;
    return d;
}

void WeatherManager::updateWeatherState(unsigned long currentTime) {
    if (!_context.gameStats) return;
    WeatherType t = _context.gameStats->currentWeather;
    if(t != WeatherType::RAINY && t != WeatherType::HEAVY_RAIN && t != WeatherType::STORM && t != WeatherType::SNOWY && t != WeatherType::HEAVY_SNOW) {
        _rainIntensityState = RainIntensityState::NONE;
        return;
    }
    if (_currentWeatherDuration == 0) {
        _rainIntensityState = RainIntensityState::PEAK;
        return;
    }
    unsigned long e = currentTime - _currentWeatherStartTime;
    unsigned long d3 = _currentWeatherDuration / 3;
    RainIntensityState n;
    if (e < d3) n = RainIntensityState::STARTING;
    else if (e < d3 * 2) n = RainIntensityState::PEAK;
    else n = RainIntensityState::ENDING;
    if (_rainIntensityState != n) _rainIntensityState = n;
}

void WeatherManager::updateWind(unsigned long currentTime) {
    if (currentTime >= _nextWindTargetTime) {
        float prevTarget = _targetWindFactor;
        _targetWindFactor = (float)random(-20, 21) / 10.0f; 
        _windChangeStartTime = currentTime;
        _nextWindTargetTime = currentTime + random(WIND_TARGET_INTERVAL_MS / 2, WIND_TARGET_INTERVAL_MS);
        if (abs(prevTarget - _targetWindFactor) > 0.05) debugPrintf("WEATHER", "Wind: New target %.1f", _targetWindFactor);
    }

    if (abs(_actualWindFactor - _targetWindFactor) > 0.01f) {
        unsigned long timeSinceTransitionStart = currentTime - _windChangeStartTime;
        float progress = (float)timeSinceTransitionStart / (float)WIND_TRANSITION_DURATION_MS;
        progress = std::min(1.0f, std::max(0.0f, progress));
        
        float prevActualWind = _actualWindFactor;
        _actualWindFactor = (float)((double)prevActualWind + ((double)_targetWindFactor - (double)prevActualWind) * (double)progress * 0.1); 

        if (progress >= 1.0f) {
            _actualWindFactor = _targetWindFactor;
            if (abs(prevActualWind - _targetWindFactor) >= 0.05f) debugPrintf("WEATHER", "Wind: Reached target %.1f", _targetWindFactor);
        }
    }
}

void WeatherManager::updateParticleDensity(WeatherType type) {
    if (type == WeatherType::RAINY) _currentParticleDensity = 40;
    else if (type == WeatherType::HEAVY_RAIN || type == WeatherType::STORM) _currentParticleDensity = 80;
    else if (type == WeatherType::SNOWY) _currentParticleDensity = 30;
    else if (type == WeatherType::HEAVY_SNOW) _currentParticleDensity = 60;
    else _currentParticleDensity = 0;
}

void WeatherManager::selectNextWeather(unsigned long currentTime) {
    if (!_context.gameStats) return;
    WeatherType previousWeather = _context.gameStats->currentWeather;
    uint16_t totalChance = 0;
    for (size_t i = 0; i < weatherDefinitionCount; ++i) totalChance += weatherDefinitions[i].chance;
    if (totalChance == 0) { 
        _context.gameStats->setWeather(WeatherType::NONE, currentTime + 5 * 60000); 
        createWeatherEffect(WeatherType::NONE); return; 
    }

    uint16_t randomPick = random(0, totalChance);
    WeatherType selectedType = WeatherType::NONE;
    for (size_t i = 0; i < weatherDefinitionCount; ++i) {
        if (randomPick < weatherDefinitions[i].chance) {
            selectedType = weatherDefinitions[i].type;
            if ((previousWeather == WeatherType::STORM && selectedType == WeatherType::RAINBOW) ||
                (previousWeather == WeatherType::RAINBOW && selectedType == WeatherType::STORM)) {
                selectedType = WeatherType::RAINY; 
                debugPrint("WEATHER", "WeatherManager: Transition Storm/Rainbow -> RAINY");
            } else if ((previousWeather == WeatherType::RAINY || previousWeather == WeatherType::HEAVY_RAIN) && selectedType == WeatherType::SUNNY) {
                if (random(100) < 15) { 
                    selectedType = WeatherType::RAINBOW;
                    debugPrint("WEATHER", "WeatherManager: Transition Rain -> RAINBOW (post-rain chance)");
                }
            }
            break;
        }
        randomPick -= weatherDefinitions[i].chance;
    }

    unsigned long minDur = 5 * 60000, maxDur = 15 * 60000;
    for (size_t i = 0; i < weatherDefinitionCount; ++i) {
        if (weatherDefinitions[i].type == selectedType) {
            minDur = weatherDefinitions[i].minDurationMs; maxDur = weatherDefinitions[i].maxDurationMs; break;
        }
    }
    unsigned long duration = random(minDur, maxDur + 1);
    unsigned long nextChange = currentTime + duration;
    _context.gameStats->setWeather(selectedType, nextChange);
    _currentWeatherStartTime = currentTime;
    _currentWeatherDuration = duration;

    createWeatherEffect(selectedType); 

    debugPrintf("WEATHER", "WeatherManager: Weather changed to %s (%d). Duration: %lu ms. Next change at: %lu\n", weatherTypeToString(selectedType), (int)selectedType, duration, nextChange);
}

void WeatherManager::forceWeather(WeatherType type, unsigned long durationMs) {
    if (!_context.gameStats) return;
    unsigned long currentTime = millis();
    unsigned long nextChange = currentTime + durationMs;
    _context.gameStats->setWeather(type, nextChange);
    _currentWeatherStartTime = currentTime;
    _currentWeatherDuration = durationMs;
    
    createWeatherEffect(type); 

    debugPrintf("WEATHER", "WeatherManager: Weather FORCED to %s (%d). Duration: %lu ms. Next change at: %lu\n", weatherTypeToString(type), (int)type, durationMs, nextChange);
}

const char* WeatherManager::weatherTypeToString(WeatherType type) {
    static const std::map<WeatherType, StringKey> weatherMap = {
        {WeatherType::NONE, StringKey::WEATHER_NONE}, {WeatherType::SUNNY, StringKey::WEATHER_SUNNY},
        {WeatherType::CLOUDY, StringKey::WEATHER_CLOUDY}, {WeatherType::RAINY, StringKey::WEATHER_RAINY},
        {WeatherType::HEAVY_RAIN, StringKey::WEATHER_HEAVY_RAIN}, {WeatherType::SNOWY, StringKey::WEATHER_SNOWY},
        {WeatherType::HEAVY_SNOW, StringKey::WEATHER_HEAVY_SNOW}, {WeatherType::STORM, StringKey::WEATHER_STORM},
        {WeatherType::RAINBOW, StringKey::WEATHER_RAINBOW}
    };
    auto it = weatherMap.find(type);
    if (it != weatherMap.end()) {
        return loc(it->second);
    }
    return loc(StringKey::WEATHER_UNKNOWN);
}

const char* WeatherManager::rainStateToString(RainIntensityState state) {
     switch (state) {
        case RainIntensityState::NONE: return "None";
        case RainIntensityState::STARTING: return "Starting";
        case RainIntensityState::PEAK: return "Peak";
        case RainIntensityState::ENDING: return "Ending";
        default: return "Unknown";
     }
}