#include "GameStats.h"
#include "SerialForwarder.h" 
#include <cmath>             
#include "character/CharacterManager.h" 
#include "DebugUtils.h"

extern SerialForwarder* forwardedSerial_ptr;
extern CharacterManager* characterManager_ptr; 

const int MAX_EFFECTIVE_SLEEP_MINUTES = 8 * 60; // 8 hours in minutes

void GameStats::updateAgeBasedOnPoints() { 
    uint32_t calculatedAge = 0;
    for (int i = numAgeStages - 1; i >= 0; --i) { if (points >= ageStages[i].minPoints) { calculatedAge = ageStages[i].ageValue; break; } }
    if (age != calculatedAge) { age = calculatedAge; debugPrintf("GAME_STATS", "Age updated to: %u based on points %u", age, points); }
}

void GameStats::updateGlobalLanguage() { 
    extern Language currentLanguage; currentLanguage = selectedLanguage;
}

void GameStats::load() { 
    Preferences prefs; 
    if (prefs.begin(PREF_STATS_NAMESPACE, true)) { 
        runningTime = prefs.getULong64(PREF_KEY_RUN_TIME, 0); 
        playingTimeMinutes = prefs.getUInt(PREF_KEY_PLAY_TIME_MIN, 0); 
        weight = prefs.getUShort(PREF_KEY_WEIGHT, 1000); 
        health = prefs.getUChar(PREF_KEY_HEALTH, 100); 
        happiness = prefs.getUChar(PREF_KEY_HAPPY, 100); 
        money = prefs.getUInt(PREF_KEY_MONEY, 0); 
        points = prefs.getUInt(PREF_KEY_POINTS, 0); 
        dirty = prefs.getUChar(PREF_KEY_DIRTY, 0); 
        sickness = (Sickness)prefs.getUChar(PREF_KEY_SICKNESS, (uint8_t)Sickness::NONE); 
        sicknessEndTime = prefs.getULong64(PREF_KEY_SICKNESS_END_TIME, 0); 
        isSleeping = prefs.getBool(PREF_KEY_SLEEPING, false); 
        fatigue = prefs.getUChar(PREF_KEY_FATIGUE, 0); 
        sleepStartTime = prefs.getULong64(PREF_KEY_SLEEP_START_TIME, 0); 
        poopCount = prefs.getUChar(PREF_KEY_POOP_COUNT, 0); 
        hunger = prefs.getUChar(PREF_KEY_HUNGER, 0); 
        selectedLanguage = (Language)prefs.getUChar(PREF_KEY_LANGUAGE, (uint8_t)LANGUAGE_UNINITIALIZED); 
        currentWeather = (WeatherType)prefs.getUChar(PREF_KEY_WEATHER_TYPE, (uint8_t)WeatherType::NONE); 
        nextWeatherChangeTime = prefs.getULong(PREF_KEY_WEATHER_NEXT_CHANGE, 0); 
        completedPrequelStage = (PrequelStage)prefs.getUChar(PREF_KEY_PREQUEL_STAGE, (uint8_t)PrequelStage::NONE); 
        FlappyTuckCoins = prefs.getUInt(PREF_KEY_FLAPPY_COINS, 0);             // <<< LOAD NEW
        FlappyTuckHighScore = prefs.getUInt(PREF_KEY_FLAPPY_HIGH_SCORE, 0);     // <<< LOAD NEW
        prefs.end(); 
        updateAgeBasedOnPoints(); 
        if (selectedLanguage != LANGUAGE_UNINITIALIZED) { 
            updateGlobalLanguage(); 
        }
        updateSickness(millis()); 
        if (isSleeping && sleepStartTime == 0) { isSleeping = false; debugPrint("GAME_STATS", "Load Warning: Loaded sleeping state with invalid start time. Resetting."); } 
    } else { 
        debugPrintf("GAME_STATS", "Load Error: opening Preferences '%s'. Resetting stats.", PREF_STATS_NAMESPACE); 
        reset(); 
    }
}

void GameStats::save() { 
    Preferences prefs; 
    if (prefs.begin(PREF_STATS_NAMESPACE, false)) { 
        prefs.putULong64(PREF_KEY_RUN_TIME, runningTime); 
        prefs.putUInt(PREF_KEY_PLAY_TIME_MIN, playingTimeMinutes); 
        prefs.putUInt(PREF_KEY_AGE, age); 
        prefs.putUShort(PREF_KEY_WEIGHT, weight); 
        prefs.putUChar(PREF_KEY_HEALTH, health); 
        prefs.putUChar(PREF_KEY_HAPPY, happiness); 
        prefs.putUInt(PREF_KEY_MONEY, money); 
        prefs.putUInt(PREF_KEY_POINTS, points); 
        prefs.putUChar(PREF_KEY_DIRTY, dirty); 
        prefs.putUChar(PREF_KEY_SICKNESS, (uint8_t)sickness); 
        prefs.putULong64(PREF_KEY_SICKNESS_END_TIME, sicknessEndTime); 
        prefs.putBool(PREF_KEY_SLEEPING, isSleeping); 
        prefs.putUChar(PREF_KEY_FATIGUE, fatigue); 
        prefs.putULong64(PREF_KEY_SLEEP_START_TIME, sleepStartTime); 
        prefs.putUChar(PREF_KEY_POOP_COUNT, poopCount); 
        prefs.putUChar(PREF_KEY_HUNGER, hunger); 
        prefs.putUChar(PREF_KEY_LANGUAGE, (uint8_t)selectedLanguage); 
        prefs.putUChar(PREF_KEY_WEATHER_TYPE, (uint8_t)currentWeather); 
        prefs.putULong(PREF_KEY_WEATHER_NEXT_CHANGE, nextWeatherChangeTime); 
        prefs.putUChar(PREF_KEY_PREQUEL_STAGE, (uint8_t)completedPrequelStage); 
        prefs.putUInt(PREF_KEY_FLAPPY_COINS, FlappyTuckCoins);                 // <<< SAVE NEW
        prefs.putUInt(PREF_KEY_FLAPPY_HIGH_SCORE, FlappyTuckHighScore);         // <<< SAVE NEW
        prefs.end(); 
    } else { 
        debugPrintf("GAME_STATS", "Save Error: opening Preferences '%s'.", PREF_STATS_NAMESPACE); 
    }
}

void GameStats::reset() { 
    runningTime = 0; playingTimeMinutes = 0; points = 0; 
    updateAgeBasedOnPoints(); 
    weight = 1000; health = 100; happiness = 100; money = 0; dirty = 0; 
    sickness = Sickness::NONE; sicknessEndTime = 0; 
    isSleeping = false; fatigue = 0; sleepStartTime = 0; 
    poopCount = 0; hunger = 0; 
    selectedLanguage = LANGUAGE_UNINITIALIZED; 
    currentWeather = WeatherType::NONE; nextWeatherChangeTime = 0; 
    completedPrequelStage = PrequelStage::NONE; 
    FlappyTuckCoins = 0;         // <<< RESET NEW
    FlappyTuckHighScore = 0;     // <<< RESET NEW
}

void GameStats::clearPrefs() { 
     Preferences prefs; if (prefs.begin(PREF_STATS_NAMESPACE, false)) { prefs.clear(); prefs.end(); debugPrintf("GAME_STATS", "Preferences cleared for namespace: %s", PREF_STATS_NAMESPACE); } else { debugPrintf("GAME_STATS", "Error opening Preferences '%s' for clear.", PREF_STATS_NAMESPACE); } reset();
}

void GameStats::deepSleepWakeUp(int minutesSleptOriginal) { 
    debugPrintf("GAME_STATS", "deepSleepWakeUp - Received original minutesSlept: %d", minutesSleptOriginal);
    
    int minutesSlept = minutesSleptOriginal;
    if (minutesSlept > MAX_EFFECTIVE_SLEEP_MINUTES) {
        debugPrintf("GAME_STATS", "  Sleep duration %d mins capped to MAX_EFFECTIVE_SLEEP_MINUTES: %d mins.", minutesSlept, MAX_EFFECTIVE_SLEEP_MINUTES);
        minutesSlept = MAX_EFFECTIVE_SLEEP_MINUTES;
    }
    
    if (minutesSlept > 0) {
         unsigned long currentTime = millis(); 
         unsigned long sleepMillis = (unsigned long)minutesSlept * 60000UL; 
         int effectiveMinutesForRegen = minutesSlept; 
         int effectiveMinutesForNeeds = minutesSlept / 4; 

         uint8_t oldFatigue = fatigue;
         setFatigue(std::max(0, (int)fatigue - effectiveMinutesForRegen / 2)); 
         if (oldFatigue != fatigue) debugPrintf("GAME_STATS", "  Fatigue: %u -> %u", oldFatigue, fatigue);

         uint8_t oldHealth = health;
         setHealth(std::min((uint8_t)100, (uint8_t)(health + effectiveMinutesForRegen / 10))); 
         if (oldHealth != health) debugPrintf("GAME_STATS", "  Health: %u -> %u", oldHealth, health);

         uint8_t oldHappiness = happiness;
         setHappiness(std::min((uint8_t)100, (uint8_t)(happiness + effectiveMinutesForRegen / 15)));
         debugPrintf("GAME_STATS", "  Happiness: %u -> %u", oldHappiness, happiness);
         
         uint8_t oldDirty = dirty;
         setDirty(std::min((uint8_t)100, (uint8_t)(dirty + effectiveMinutesForNeeds / 20))); 
         debugPrintf("GAME_STATS", "  Dirty: %u -> %u", oldDirty, dirty);
         
         if (characterManager_ptr && characterManager_ptr->currentLevelCanBeHungry()) {
            uint8_t oldHunger = hunger;
            setHunger(std::min((uint8_t)100, (uint8_t)(hunger + effectiveMinutesForNeeds / 5)));
            debugPrintf("GAME_STATS", "  Hunger: %u -> %u", oldHunger, hunger);
         } else {
            debugPrint("GAME_STATS", "  Hunger not applicable for current level, no change.");
         }

         if (characterManager_ptr && characterManager_ptr->currentLevelCanPoop()) {
            uint8_t oldPoopCount = poopCount;
            int poopCheckIntervals = effectiveMinutesForNeeds; 
            for (int i = 0; i < poopCheckIntervals && poopCount < 3; ++i) { 
                int poopChance = (hunger / 20); 
                if (!characterManager_ptr->currentLevelCanBeHungry()) poopChance = 5; 
                if (random(1000) < poopChance) { setPoopCount(poopCount + 1); } 
            }
            debugPrintf("GAME_STATS", "  PoopCount: %u -> %u", oldPoopCount, poopCount);
         } else {
            debugPrint("GAME_STATS", "  Poop not applicable for current level, no change.");
         }

         unsigned long oldNextWeatherChangeTime = nextWeatherChangeTime;
         if (nextWeatherChangeTime > 0) { 
            long remainingWeatherTimeMillis = (long)nextWeatherChangeTime - (long)currentTime; 
            if (sleepMillis >= remainingWeatherTimeMillis && remainingWeatherTimeMillis > 0) { 
                nextWeatherChangeTime = currentTime; 
                debugPrint("GAME_STATS", "  Weather timer expired during sleep, reset to change now."); 
            } else if (nextWeatherChangeTime > sleepMillis) { 
                nextWeatherChangeTime -= sleepMillis; 
            }
         }
         debugPrintf("GAME_STATS", "  NextWeatherChangeTime: %lu -> %lu", oldNextWeatherChangeTime, nextWeatherChangeTime);


         unsigned long oldSicknessEndTime = sicknessEndTime;
         if (sicknessEndTime > 0) { 
            long remainingSicknessTimeMillis = (long)sicknessEndTime - (long)currentTime; 
            if (sleepMillis >= remainingSicknessTimeMillis && remainingSicknessTimeMillis > 0) { 
                sicknessEndTime = currentTime; 
                debugPrint("GAME_STATS", "  Sickness timer likely expired during sleep, reset to check now."); 
            } else if (sicknessEndTime > sleepMillis) { 
                sicknessEndTime -= sleepMillis; 
            }
         }
         debugPrintf("GAME_STATS", "  SicknessEndTime: %lu -> %lu", oldSicknessEndTime, sicknessEndTime);

         if (isSleeping) { 
            sleepStartTime = currentTime; 
         }
         debugPrint("GAME_STATS", "GameStats::deepSleepWakeUp - Stat adjustments complete.");
    } else {
        debugPrint("GAME_STATS", "GameStats::deepSleepWakeUp - minutesSlept was 0 or negative, no stat changes applied.");
    }
}

void GameStats::addPoints(uint32_t amount) { if (amount == 0) return; points += amount; updateAgeBasedOnPoints(); }
void GameStats::setHealth(uint8_t value) { health = std::min((uint8_t)100, std::max((uint8_t)0, value)); }
void GameStats::setHappiness(uint8_t value) { happiness = std::min((uint8_t)100, std::max((uint8_t)0, value)); }
void GameStats::setDirty(uint8_t value) { dirty = std::min((uint8_t)100, std::max((uint8_t)0, value)); }
void GameStats::setFatigue(uint8_t value) { fatigue = std::min((uint8_t)100, std::max((uint8_t)0, value)); }
void GameStats::setHunger(uint8_t value) { hunger = std::min((uint8_t)100, std::max((uint8_t)0, value)); }
void GameStats::setSickness(Sickness value, unsigned long durationMillis) { sickness = value; if (value != Sickness::NONE && durationMillis > 0) { sicknessEndTime = millis() + durationMillis; } else { sicknessEndTime = 0; } }
void GameStats::setIsSleeping(bool value) { isSleeping = value; if (isSleeping) { sleepStartTime = millis(); } else { sleepStartTime = 0; } }
void GameStats::setPoopCount(uint8_t value) { poopCount = std::min((uint8_t)3, std::max((uint8_t)0, value)); }
void GameStats::setLanguage(Language value) { if (selectedLanguage != value) { selectedLanguage = value; updateGlobalLanguage(); } }
void GameStats::setWeather(WeatherType type, unsigned long nextChange) { currentWeather = type; nextWeatherChangeTime = nextChange; }
void GameStats::setCompletedPrequelStage(PrequelStage stage) { completedPrequelStage = stage; }

uint8_t GameStats::getModifiedHappiness() const { 
    int modifiedHappy = happiness; 
    switch (currentWeather) { 
        case WeatherType::RAINY: modifiedHappy -= 20; break; 
        case WeatherType::HEAVY_RAIN: modifiedHappy -= 30; break; 
        case WeatherType::STORM: modifiedHappy -= 40; break; 
        case WeatherType::RAINBOW: modifiedHappy += 30; break; 
        case WeatherType::SUNNY: modifiedHappy += 20; break; 
        default: break; 
    } 
    if (isSick()) { modifiedHappy -= 25; } // Generic happiness penalty for any sickness
    if (characterManager_ptr && characterManager_ptr->currentLevelCanPoop() && poopCount > 0) { 
        modifiedHappy -= (poopCount * 15); 
    } 
    return (uint8_t)std::max(0, std::min(100, modifiedHappy));
}
uint8_t GameStats::getModifiedFatigue() const { return fatigue; }
uint8_t GameStats::getModifiedDirty() const { return dirty; }
uint8_t GameStats::getModifiedHealth() const { return health; }

const char* GameStats::getSicknessString() const { switch(sickness){ case Sickness::NONE: return loc(StringKey::SICKNESS_NONE); case Sickness::COLD: return loc(StringKey::SICKNESS_COLD); case Sickness::HOT: return loc(StringKey::SICKNESS_HOT); case Sickness::DIARRHEA: return loc(StringKey::SICKNESS_DIARRHEA); case Sickness::VOMIT: return loc(StringKey::SICKNESS_VOMIT); case Sickness::HEADACHE: return loc(StringKey::SICKNESS_HEADACHE); default: return loc(StringKey::SICKNESS_UNKNOWN); } }

bool GameStats::isSick() const { return sickness != Sickness::NONE; }
bool GameStats::updateSickness(unsigned long currentTime) { if (isSick() && sicknessEndTime > 0 && currentTime >= sicknessEndTime) { setSickness(Sickness::NONE); return true; } return false; }

bool GameStats::updateAccumulatedStats(unsigned long currentTime, unsigned long timeDeltaMinutes) {
    if (timeDeltaMinutes == 0 || !characterManager_ptr) return false;

    bool statsActuallyChanged = false;

    uint8_t initialHealth = health;
    uint8_t initialHappiness = happiness;
    uint8_t initialFatigue = fatigue;
    uint8_t initialHunger = hunger;
    uint8_t initialDirty = dirty;
    uint8_t initialPoopCount = poopCount;

    // --- Sleeping State Logic ---
    if (isSleeping) {
        int fatigueReduction = timeDeltaMinutes * 2; // Recovers 2 fatigue per minute slept
        setFatigue(std::max(0, (int)fatigue - fatigueReduction));

        if (characterManager_ptr->currentLevelCanBeHungry()) {
            int hungerIncreaseSleeping = timeDeltaMinutes / 4; // 1 hunger every 4 minutes
            setHunger(std::min((uint8_t)100, (uint8_t)(hunger + hungerIncreaseSleeping)));
        }
        int dirtyIncreaseSleeping = timeDeltaMinutes / 10; // 1 dirty every 10 minutes
        setDirty(std::min((uint8_t)100, (uint8_t)(dirty + dirtyIncreaseSleeping)));
        
        if (characterManager_ptr->currentLevelCanPoop() && poopCount < 3) {
            int poopChanceSleeping = timeDeltaMinutes * 1; // Low chance: 1/1000 per minute
            if (random(1000) < poopChanceSleeping) {
                setPoopCount(poopCount + 1);
                setDirty(std::min((uint8_t)100, (uint8_t)(dirty + 5)));
            }
        }
        // Minimal health/happiness regen while sleeping
        setHealth(std::min((uint8_t)100, (uint8_t)(health + (timeDeltaMinutes / 10)))); // 1 health every 10 min
        setHappiness(std::min((uint8_t)100, (uint8_t)(happiness + (timeDeltaMinutes / 15)))); // 1 happy every 15 min


        if (fatigue == 0) {
            setIsSleeping(false); 
            debugPrint("GAME_STATS", "GameStats AccumStats: Fatigue reached 0, Auto-waking character!");
        }
    } else { // --- Awake State Logic ---
        // Base fatigue increase
        int fatigueIncrease = timeDeltaMinutes * 1; // 1 fatigue per minute awake
        if (currentWeather == WeatherType::SUNNY || currentWeather == WeatherType::STORM) { fatigueIncrease += timeDeltaMinutes * 1; }
        setFatigue(std::min((uint8_t)100, (uint8_t)(fatigue + fatigueIncrease)));

        // Base dirty increase
        int dirtyIncrease = 0;
        if (currentWeather == WeatherType::RAINY) dirtyIncrease += timeDeltaMinutes * 1;
        else if (currentWeather == WeatherType::HEAVY_RAIN) dirtyIncrease += timeDeltaMinutes * 2;
        else if (currentWeather == WeatherType::STORM) dirtyIncrease += timeDeltaMinutes * 3;
        
        if (characterManager_ptr->currentLevelCanPoop() && poopCount > 0) {
            dirtyIncrease += timeDeltaMinutes * (5 * poopCount); // Poop makes it dirtier faster
        }
        setDirty(std::min((uint8_t)100, (uint8_t)(dirty + dirtyIncrease)));

        // Base hunger increase (if applicable)
        if (characterManager_ptr->currentLevelCanBeHungry()) {
            int hungerIncrease = timeDeltaMinutes * 1; // 1 hunger per minute
            setHunger(std::min((uint8_t)100, (uint8_t)(hunger + hungerIncrease)));
        }

        // Poop chance (if applicable)
        if (characterManager_ptr->currentLevelCanPoop() && poopCount < 3) {
            int poopChance = timeDeltaMinutes * (hunger / 15); // Higher hunger = higher chance
            if (!characterManager_ptr->currentLevelCanBeHungry()) { // If can't be hungry, use a base chance
                poopChance = timeDeltaMinutes * 2; // e.g. 2/1000 per minute of being awake
            }
            if (random(1000) < poopChance) {
                setPoopCount(poopCount + 1);
                setDirty(std::min((uint8_t)100, (uint8_t)(dirty + 15))); // Pooping makes it dirtier
                debugPrintf("GAME_STATS", "GameStats AccumStats: Character pooped! Count: %d", poopCount);
            }
        }
    }

    // --- Health penalties from severe needs (applied whether sleeping or awake) ---
    int healthDeltaFromNeeds = 0;
    if (characterManager_ptr->currentLevelCanBeHungry()) {
        if (hunger > 90) healthDeltaFromNeeds -= timeDeltaMinutes * 2; // Severe hunger
        else if (hunger > 75) healthDeltaFromNeeds -= timeDeltaMinutes * 1; // Moderate hunger
    }
    if (dirty > 95) healthDeltaFromNeeds -= timeDeltaMinutes * 2; // Very dirty
    else if (dirty > 80) healthDeltaFromNeeds -= timeDeltaMinutes * 1; // Dirty
    if (fatigue > 95 && !isSleeping) healthDeltaFromNeeds -= timeDeltaMinutes * 1; // Very tired and not sleeping
    
    if (healthDeltaFromNeeds != 0) {
        setHealth(std::max(0, std::min(100, (int)health + healthDeltaFromNeeds)));
    }

    // --- Sickness Effects (applied whether sleeping or awake) ---
    if (isSick()) {
        debugPrintf("GAME_STATS", "AccumStats: Applying sickness effects for: %s over %lu minutes", getSicknessString(), timeDeltaMinutes);

        float healthSicknessModifier = 0.0f;
        float happinessSicknessModifier = 0.0f;
        float fatigueSicknessModifier = 0.0f;
        float hungerSicknessModifier = 0.0f;    // Negative means less appetite (hunger stat decreases, or increases slower)
        float dirtySicknessModifier = 0.0f;

        switch (sickness) {
            case Sickness::COLD:
                healthSicknessModifier    -= 0.2f * timeDeltaMinutes;
                happinessSicknessModifier -= 0.3f * timeDeltaMinutes;
                fatigueSicknessModifier   += 0.4f * timeDeltaMinutes;
                break;
            case Sickness::HOT: // Fever
                healthSicknessModifier    -= 0.5f * timeDeltaMinutes;
                happinessSicknessModifier -= 0.6f * timeDeltaMinutes;
                fatigueSicknessModifier   += 0.8f * timeDeltaMinutes;
                if (characterManager_ptr->currentLevelCanBeHungry()) hungerSicknessModifier -= 0.1f * timeDeltaMinutes; // Reduced appetite
                dirtySicknessModifier     += 0.2f * timeDeltaMinutes;  // Sweating
                break;
            case Sickness::DIARRHEA:
                healthSicknessModifier    -= 0.7f * timeDeltaMinutes;
                happinessSicknessModifier -= 1.0f * timeDeltaMinutes;
                fatigueSicknessModifier   += 0.5f * timeDeltaMinutes;
                if (characterManager_ptr->currentLevelCanBeHungry()) hungerSicknessModifier -= 0.3f * timeDeltaMinutes; // Nausea
                if (characterManager_ptr->currentLevelCanPoop()) dirtySicknessModifier += 1.0f * timeDeltaMinutes;   // Accidents
                break;
            case Sickness::VOMIT:
                healthSicknessModifier    -= 0.8f * timeDeltaMinutes;
                happinessSicknessModifier -= 1.2f * timeDeltaMinutes;
                fatigueSicknessModifier   += 0.6f * timeDeltaMinutes;
                if (characterManager_ptr->currentLevelCanBeHungry()) hungerSicknessModifier -= 0.5f * timeDeltaMinutes; // Severe Nausea
                if (characterManager_ptr->currentLevelCanPoop()) dirtySicknessModifier += 1.2f * timeDeltaMinutes;   // Mess
                break;
            case Sickness::HEADACHE:
                healthSicknessModifier    -= 0.1f * timeDeltaMinutes;
                happinessSicknessModifier -= 0.5f * timeDeltaMinutes;
                fatigueSicknessModifier   += 0.2f * timeDeltaMinutes;
                break;
            case Sickness::NONE:
            default:
                break;
        }

        // Apply sickness modifiers
        if (healthSicknessModifier != 0.0f)   setHealth(std::max(0, std::min(100, (int)health + (int)round(healthSicknessModifier))));
        if (happinessSicknessModifier != 0.0f)setHappiness(std::max(0, std::min(100, (int)happiness + (int)round(happinessSicknessModifier))));
        if (fatigueSicknessModifier != 0.0f)  setFatigue(std::max(0, std::min(100, (int)fatigue + (int)round(fatigueSicknessModifier))));
        if (characterManager_ptr->currentLevelCanBeHungry() && hungerSicknessModifier != 0.0f) {
            setHunger(std::max(0, std::min(100, (int)hunger + (int)round(hungerSicknessModifier))));
        }
        if (dirtySicknessModifier != 0.0f)    setDirty(std::max(0, std::min(100, (int)dirty + (int)round(dirtySicknessModifier))));
    }
    
    // Determine if any stat actually changed
    statsActuallyChanged = (health != initialHealth || happiness != initialHappiness || fatigue != initialFatigue ||
                           hunger != initialHunger || dirty != initialDirty || poopCount != initialPoopCount);

    if (statsActuallyChanged) {
        debugPrint("GAME_STATS", "AccumStats: One or more stats changed.");
        debugPrintf("GAME_STATS", "  H:%u(%+d) Hap:%u(%+d) F:%u(%+d) Hun:%u(%+d) D:%u(%+d) P:%u(%+d)",
                                 health, (int)health - initialHealth,
                                 happiness, (int)happiness - initialHappiness,
                                 fatigue, (int)fatigue - initialFatigue,
                                 hunger, (int)hunger - initialHunger,
                                 dirty, (int)dirty - initialDirty,
                                 poopCount, (int)poopCount - initialPoopCount);
    }
    
    return statsActuallyChanged;
}
// --- END OF FILE src/GameStats.cpp ---