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
#include "Effects/Windy/WindyWeatherEffect.h"
#include "Effects/Fog/FogWeatherEffect.h"
#include "Effects/Aurora/AuroraWeatherEffect.h"
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

void WeatherManager::updateWeatherComposition(WeatherType primaryType) {
    _activeEffects.clear();
    debugPrintf("WEATHER", "Updating weather composition. Primary: %d", (int)primaryType);

    // Add primary weather effect
    switch (primaryType) {
        case WeatherType::SUNNY:       _activeEffects.emplace_back(new SunnyWeatherEffect(_context)); break;
        case WeatherType::CLOUDY:      _activeEffects.emplace_back(new CloudyWeatherEffect(_context)); break;
        case WeatherType::RAINY:       _activeEffects.emplace_back(new RainyWeatherEffect(_context, false)); break;
        case WeatherType::HEAVY_RAIN:  _activeEffects.emplace_back(new HeavyRainWeatherEffect(_context)); break;
        case WeatherType::SNOWY:       _activeEffects.emplace_back(new SnowyWeatherEffect(_context, false)); break;
        case WeatherType::HEAVY_SNOW:  _activeEffects.emplace_back(new HeavySnowWeatherEffect(_context)); break;
        case WeatherType::STORM:       _activeEffects.emplace_back(new StormWeatherEffect(_context)); break;
        case WeatherType::RAINBOW:     _activeEffects.emplace_back(new RainbowWeatherEffect(_context)); break;
        case WeatherType::NONE:
        default:                      _activeEffects.emplace_back(new NoneWeatherEffect(_context)); break;
    }

    // --- SECONDARY EFFECT LOGIC ---
    bool hasWind = false;
    if (primaryType == WeatherType::STORM) {
        hasWind = true;
    } else if (primaryType != WeatherType::RAINBOW && primaryType != WeatherType::SUNNY && primaryType != WeatherType::AURORA) {
        if (random(100) < 35) {
            hasWind = true;
        }
    }
    if (hasWind) {
        debugPrint("WEATHER", "Adding secondary WIND effect to composition.");
        _activeEffects.emplace_back(new WindyWeatherEffect(_context));
    }

    if (primaryType == WeatherType::CLOUDY || primaryType == WeatherType::RAINY || primaryType == WeatherType::STORM) {
        if (random(100) < 20) {
            debugPrint("WEATHER", "Adding secondary FOG effect to composition.");
            _activeEffects.emplace_back(new FogWeatherEffect(_context));
        }
    }

    if (primaryType == WeatherType::NONE && std::abs(_actualWindFactor) < 0.6f) {
        if (random(100) < 5) { // Very low chance
             debugPrint("WEATHER", "Adding secondary AURORA effect to composition.");
            _activeEffects.emplace_back(new AuroraWeatherEffect(_context));
        }
    }
    // --- END SECONDARY EFFECT LOGIC ---


    // Initialize all active effects
    unsigned long currentTime = millis();
    for(const auto& effect : _activeEffects) {
        effect->init(currentTime);
        effect->setWindFactor(_actualWindFactor);
        effect->setIntensityState(_rainIntensityState);
        effect->setParticleDensity(_currentParticleDensity);
    }

    if (_birdManager) {
        bool birdsActive = (primaryType != WeatherType::STORM && primaryType != WeatherType::HEAVY_RAIN && primaryType != WeatherType::HEAVY_SNOW) && (std::abs(_actualWindFactor) <= 0.9f);
        if (!birdsActive) {
            String reason;
            if (primaryType == WeatherType::STORM || primaryType == WeatherType::HEAVY_RAIN || primaryType == WeatherType::HEAVY_SNOW) {
                 reason += "Harsh weather; ";
            }
            if (std::abs(_actualWindFactor) > 0.9f) {
                 reason += "Strong wind; ";
            }
            _birdManager->setActive(birdsActive, reason.c_str());
        } else {
            _birdManager->setActive(birdsActive);
        }
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

    if ((uint8_t)_context.gameStats->currentWeather >= (uint8_t)WeatherType::UNKNOWN ) { 
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

        updateParticleDensity(currentTime);
        updateWeatherState(currentTime); 
        updateWeatherComposition(_context.gameStats->currentWeather); 
        
        debugPrintf("WEATHER", "WeatherManager: Loaded weather %d, Est Start: %lu, End: %lu, Est Dur: %lu\n", (int)_context.gameStats->currentWeather, _currentWeatherStartTime, _context.gameStats->nextWeatherChangeTime, _currentWeatherDuration);
    }

     if (_context.gameStats->currentWeather == WeatherType::STORM) {
         _actualWindFactor = (random(0,2) == 0 ? 1.0f : -1.0f) * ((float)random(10, 16) / 10.0f);
         _targetWindFactor = _actualWindFactor;
         _windChangeStartTime = currentTime;
         for(const auto& effect : _activeEffects) { effect->setWindFactor(_actualWindFactor); }
         if (_birdManager) _birdManager->setWindFactor(_actualWindFactor); 
         debugPrintf("WEATHER", "WeatherManager: Initial storm wind factor: %.1f\n", _actualWindFactor);
     } else {
         _actualWindFactor = 0.0f;
         _targetWindFactor = 0.0f;
         for(const auto& effect : _activeEffects) { effect->setWindFactor(_actualWindFactor); }
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
    updateParticleDensity(currentTime); 

    for(const auto& effect : _activeEffects) {
        effect->setWindFactor(_actualWindFactor);
        effect->setIntensityState(_rainIntensityState);
        effect->setParticleDensity(_currentParticleDensity);
        effect->update(currentTime);
    }
    if (_birdManager) { 
        _birdManager->setWindFactor(_actualWindFactor);
        if (std::abs(_actualWindFactor) > 0.9f) {
            _birdManager->setActive(false, "Strong wind in update");
        }
        _birdManager->update(currentTime);
    }
}

void WeatherManager::drawBackground(bool allowDrawing) {
    if (!allowDrawing || !_context.gameStats) return; 
    
    for(const auto& effect : _activeEffects) {
        effect->drawBackground();
    }
}

void WeatherManager::drawForeground(bool allowDrawing) {
    if (!allowDrawing || !_context.gameStats) return; 
    
    for(const auto& effect : _activeEffects) {
        effect->drawForeground();
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
    
    if(t != WeatherType::RAINY && t != WeatherType::HEAVY_RAIN && t != WeatherType::STORM && t != WeatherType::SNOWY && t != WeatherType::HEAVY_SNOW && t != WeatherType::SUNNY) {
        _rainIntensityState = RainIntensityState::NONE;
        _isFadingOut = false;
        return;
    }
    if (_currentWeatherDuration == 0) {
        _rainIntensityState = RainIntensityState::PEAK;
        _isFadingOut = false;
        return;
    }

    unsigned long timeUntilChange = _context.gameStats->nextWeatherChangeTime - currentTime;
    RainIntensityState previousState = _rainIntensityState;
    
    if (timeUntilChange <= FADEOUT_DURATION_MS) {
        _rainIntensityState = RainIntensityState::ENDING;
    } else if (currentTime - _currentWeatherStartTime < FADEOUT_DURATION_MS) {
        _rainIntensityState = RainIntensityState::STARTING;
    } else {
        _rainIntensityState = RainIntensityState::PEAK;
    }

    if (_rainIntensityState == RainIntensityState::ENDING && previousState != RainIntensityState::ENDING) {
        _pendingNextWeatherType = peekNextWeatherType();
        
        bool currentHasParticles = (t == WeatherType::RAINY || t == WeatherType::HEAVY_RAIN || t == WeatherType::STORM || t == WeatherType::SNOWY || t == WeatherType::HEAVY_SNOW);
        bool nextHasParticles = (_pendingNextWeatherType == WeatherType::RAINY || _pendingNextWeatherType == WeatherType::HEAVY_RAIN || _pendingNextWeatherType == WeatherType::STORM || _pendingNextWeatherType == WeatherType::SNOWY || _pendingNextWeatherType == WeatherType::HEAVY_SNOW);
        
        bool currentIsSunny = (t == WeatherType::SUNNY);
        bool nextIsSunny = (_pendingNextWeatherType == WeatherType::SUNNY);

        if ((currentHasParticles && !nextHasParticles) || (currentIsSunny && !nextIsSunny)) {
            _isFadingOut = true;
            for(const auto& effect : _activeEffects) {
                effect->startFadeOut(FADEOUT_DURATION_MS);
            }
            debugPrintf("WEATHER", "Fade-out initiated for %s. Next weather: %s", weatherTypeToString(t), weatherTypeToString(_pendingNextWeatherType));
        } else {
            _isFadingOut = false;
        }
    } else if (_rainIntensityState != RainIntensityState::ENDING) {
        _isFadingOut = false; 
    }
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
        
        float startWind = _actualWindFactor; // This logic might need adjustment if _actualWindFactor is constantly changing
        _actualWindFactor = startWind + (_targetWindFactor - startWind) * progress;

        if (progress >= 1.0f) {
            _actualWindFactor = _targetWindFactor;
            if (abs(startWind - _targetWindFactor) >= 0.05f) debugPrintf("WEATHER", "Wind: Reached target %.1f", _targetWindFactor);
        }
    }
}

void WeatherManager::updateParticleDensity(unsigned long currentTime) {
    WeatherType t = _context.gameStats->currentWeather;

    if (_isFadingOut) {
        long timeUntilEnd = _context.gameStats->nextWeatherChangeTime - currentTime;
        timeUntilEnd = std::max(0L, timeUntilEnd);
        float fadeProgress = 1.0f - ((float)timeUntilEnd / (float)FADEOUT_DURATION_MS);
        fadeProgress = std::min(1.0f, std::max(0.0f, fadeProgress));

        uint8_t baseDensity = 0;
        if (t == WeatherType::RAINY) baseDensity = 40;
        else if (t == WeatherType::HEAVY_RAIN || t == WeatherType::STORM) baseDensity = 80;
        else if (t == WeatherType::SNOWY) baseDensity = 30;
        else if (t == WeatherType::HEAVY_SNOW) baseDensity = 60;
        
        _currentParticleDensity = static_cast<uint8_t>(round((float)baseDensity * (1.0f - fadeProgress)));
    } else {
        if (t == WeatherType::RAINY) _currentParticleDensity = 40;
        else if (t == WeatherType::HEAVY_RAIN || t == WeatherType::STORM) _currentParticleDensity = 80;
        else if (t == WeatherType::SNOWY) _currentParticleDensity = 30;
        else if (t == WeatherType::HEAVY_SNOW) _currentParticleDensity = 60;
        else _currentParticleDensity = 0;
    }
}

void WeatherManager::selectNextWeather(unsigned long currentTime) {
    if (!_context.gameStats) return;
    WeatherType previousWeather = _context.gameStats->currentWeather;
    
    _pendingNextWeatherType = peekNextWeatherType();
    
    if ((previousWeather == WeatherType::STORM && _pendingNextWeatherType == WeatherType::RAINBOW) ||
        (previousWeather == WeatherType::RAINBOW && _pendingNextWeatherType == WeatherType::STORM)) {
        _pendingNextWeatherType = WeatherType::RAINY; 
        debugPrint("WEATHER", "WeatherManager: Transition Storm/Rainbow -> RAINY");
    } else if ((previousWeather == WeatherType::RAINY || previousWeather == WeatherType::HEAVY_RAIN) && _pendingNextWeatherType == WeatherType::SUNNY) {
        if (random(100) < 15) { 
            _pendingNextWeatherType = WeatherType::RAINBOW;
            debugPrint("WEATHER", "WeatherManager: Transition Rain -> RAINBOW (post-rain chance)");
        }
    }
    
    unsigned long minDur = 5 * 60000, maxDur = 15 * 60000;
    for (size_t i = 0; i < weatherDefinitionCount; ++i) {
        if (weatherDefinitions[i].type == _pendingNextWeatherType) {
            minDur = weatherDefinitions[i].minDurationMs; maxDur = weatherDefinitions[i].maxDurationMs; break;
        }
    }
    unsigned long duration = random(minDur, maxDur + 1);
    unsigned long nextChange = currentTime + duration;
    _context.gameStats->setWeather(_pendingNextWeatherType, nextChange);
    _currentWeatherStartTime = currentTime;
    _currentWeatherDuration = duration;

    _windChangeStartTime = currentTime;
    if (_pendingNextWeatherType == WeatherType::STORM) {
        _targetWindFactor = (random(0,2) == 0 ? -1.0f : 1.0f) * ((float)random(10, 18) / 10.0f);
        if (std::abs(_targetWindFactor) < 1.0f) {
            _targetWindFactor = (_targetWindFactor > 0) ? 1.0f : -1.0f;
        }
    } else if (previousWeather == WeatherType::STORM) {
        _targetWindFactor = 0.0f;
    } else {
        _targetWindFactor = (float)random(-8, 9) / 10.0f;
    }
    debugPrintf("WEATHER", "Wind target set to %.2f on weather change.", _targetWindFactor);

    updateWeatherComposition(_pendingNextWeatherType); 

    debugPrintf("WEATHER", "WeatherManager: Weather changed to %s (%d). Duration: %lu ms. Next change at: %lu\n", weatherTypeToString(_pendingNextWeatherType), (int)_pendingNextWeatherType, duration, nextChange);
}

WeatherType WeatherManager::peekNextWeatherType() const {
    uint16_t totalChance = 0;
    for (size_t i = 0; i < weatherDefinitionCount; ++i) totalChance += weatherDefinitions[i].chance;
    if (totalChance == 0) return WeatherType::NONE;

    uint16_t randomPick = random(0, totalChance);
    for (size_t i = 0; i < weatherDefinitionCount; ++i) {
        if (randomPick < weatherDefinitions[i].chance) {
            return weatherDefinitions[i].type;
        }
        randomPick -= weatherDefinitions[i].chance;
    }
    return WeatherType::NONE; // Fallback
}


void WeatherManager::forceWeather(WeatherType type, unsigned long durationMs) {
    if (!_context.gameStats) return;
    unsigned long currentTime = millis();
    unsigned long nextChange = currentTime + durationMs;
    _context.gameStats->setWeather(type, nextChange);
    _currentWeatherStartTime = currentTime;
    _currentWeatherDuration = durationMs;
    
    updateWeatherComposition(type); 

    debugPrintf("WEATHER", "WeatherManager: Weather FORCED to %s (%d). Duration: %lu ms. Next change at: %lu\n", weatherTypeToString(type), (int)type, durationMs, nextChange);
}

void WeatherManager::forceWind(float windFactor) {
    _targetWindFactor = windFactor;
    _windChangeStartTime = millis();
    _nextWindTargetTime = _windChangeStartTime + WIND_TRANSITION_DURATION_MS; 
    debugPrintf("WEATHER", "Wind manually forced to %.2f", windFactor);
}

void WeatherManager::forceAddSecondaryEffect(const String& effectName) {
    WeatherType effectTypeToAdd;
    if (effectName.equalsIgnoreCase("fog")) effectTypeToAdd = WeatherType::FOG;
    else if (effectName.equalsIgnoreCase("aurora")) effectTypeToAdd = WeatherType::AURORA;
    else if (effectName.equalsIgnoreCase("windy")) effectTypeToAdd = WeatherType::WINDY;
    else {
        debugPrintf("WEATHER", "Unknown effect to add: %s", effectName.c_str());
        return;
    }
    
    bool alreadyExists = false;
    for (const auto& effect : _activeEffects) {
        if (effect->getType() == effectTypeToAdd) {
            alreadyExists = true;
            break;
        }
    }

    if (alreadyExists) {
        debugPrintf("WEATHER", "Effect %s already active.", effectName.c_str());
        return;
    }

    std::unique_ptr<WeatherEffectBase> newEffect;
    switch (effectTypeToAdd) {
        case WeatherType::FOG:    newEffect.reset(new FogWeatherEffect(_context)); break;
        case WeatherType::AURORA: newEffect.reset(new AuroraWeatherEffect(_context)); break;
        case WeatherType::WINDY:  newEffect.reset(new WindyWeatherEffect(_context)); break;
        default: break;
    }
    
    if (newEffect) {
        debugPrintf("WEATHER", "Forcing add of secondary effect: %s", effectName.c_str());
        newEffect->init(millis());
        newEffect->setWindFactor(_actualWindFactor);
        newEffect->setIntensityState(_rainIntensityState);
        newEffect->setParticleDensity(_currentParticleDensity);
        _activeEffects.push_back(std::move(newEffect));
    }
}

void WeatherManager::forceSpawnBirds(int count) {
    if (_birdManager) {
        _birdManager->spawnBirdsOnDemand(count);
    } else {
        debugPrint("WEATHER", "Cannot force spawn birds, BirdManager is null.");
    }
}


String WeatherManager::getActiveEffectsString() const {
    if (_activeEffects.empty()) {
        return "None";
    }

    String effectsList;
    for (size_t i = 0; i < _activeEffects.size(); ++i) {
        if (i > 0) {
            effectsList += ", ";
        }
        effectsList += weatherTypeToString(_activeEffects[i]->getType());
    }
    return effectsList;
}

const char* WeatherManager::weatherTypeToString(WeatherType type) {
    static const std::map<WeatherType, StringKey> weatherMap = {
        {WeatherType::NONE, StringKey::WEATHER_NONE}, {WeatherType::SUNNY, StringKey::WEATHER_SUNNY},
        {WeatherType::CLOUDY, StringKey::WEATHER_CLOUDY}, {WeatherType::RAINY, StringKey::WEATHER_RAINY},
        {WeatherType::HEAVY_RAIN, StringKey::WEATHER_HEAVY_RAIN}, {WeatherType::SNOWY, StringKey::WEATHER_SNOWY},
        {WeatherType::HEAVY_SNOW, StringKey::WEATHER_HEAVY_SNOW}, {WeatherType::STORM, StringKey::WEATHER_STORM},
        {WeatherType::RAINBOW, StringKey::WEATHER_RAINBOW}, {WeatherType::WINDY, StringKey::WEATHER_WINDY},
        {WeatherType::FOG, StringKey::WEATHER_FOG}, {WeatherType::AURORA, StringKey::WEATHER_AURORA},
        {WeatherType::UNKNOWN, StringKey::WEATHER_UNKNOWN}
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