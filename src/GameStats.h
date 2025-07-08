#ifndef GAMESTATS_H
#define GAMESTATS_H

#include <Arduino.h>
#include <Preferences.h>
#include <algorithm>
#include "Localization.h"
#include "Weather/WeatherTypes.h" // Include WeatherTypes enum

// Define keys for Preferences
#define PREF_STATS_NAMESPACE "TamaStats"
#define PREF_KEY_RUN_TIME "runTime"
#define PREF_KEY_PLAY_TIME_MIN "playTimeMin"
#define PREF_KEY_AGE "age"
#define PREF_KEY_WEIGHT "weight"
#define PREF_KEY_HEALTH "health"
#define PREF_KEY_HAPPY "happy"
#define PREF_KEY_MONEY "money"
#define PREF_KEY_POINTS "points"
#define PREF_KEY_DIRTY "dirty"
#define PREF_KEY_SICKNESS "sickness"
#define PREF_KEY_SICKNESS_END_TIME "sicknessEnd"
#define PREF_KEY_SLEEPING "sleeping"
#define PREF_KEY_FATIGUE "fatigue"
#define PREF_KEY_SLEEP_START_TIME "sleepStart"
#define PREF_KEY_POOP_COUNT "poopCount" 
#define PREF_KEY_HUNGER "hunger"
#define PREF_KEY_LANGUAGE "language"
#define PREF_KEY_WEATHER_TYPE "weather"
#define PREF_KEY_WEATHER_NEXT_CHANGE "weatherNext"
#define PREF_KEY_PREQUEL_STAGE "prequelStage" 
#define PREF_KEY_FLAPPY_COINS "flappyCoins"         // <<< NEW KEY
#define PREF_KEY_FLAPPY_HIGH_SCORE "flappyHiScore"  // <<< NEW KEY


// --- Game Logic Constants ---
const uint8_t SLEEP_FATIGUE_THRESHOLD = 80; 
const uint8_t WAKE_FATIGUE_THRESHOLD = 50;  

// --- Age Stages Definition ---
struct AgeStage { uint32_t minPoints; uint32_t ageValue; };
static const AgeStage ageStages[] = { {0, 0}, {1000, 1}, {5000, 2} };
static const size_t numAgeStages = sizeof(ageStages) / sizeof(ageStages[0]);

// --- Sickness Enum ---
enum class Sickness : uint8_t { NONE = 0, COLD, HOT, DIARRHEA, VOMIT, HEADACHE };

// --- Prequel Stage Enum ---
enum class PrequelStage : uint8_t {
    NONE = 0,                           
    LANGUAGE_SELECTED,                  
    STAGE_1_AWAKENING_COMPLETE,
    STAGE_2_CONGLOMERATE_COMPLETE,      
    STAGE_3_JOURNEY_COMPLETE,           
    STAGE_4_SHELLWEAVE_COMPLETE,        // <<< NEW STAGE COMPLETION FLAG
    PREQUEL_FINISHED                    
};
const Language LANGUAGE_UNINITIALIZED = static_cast<Language>(255); 


struct GameStats {
    // --- Member Variables ---
    unsigned long runningTime = 0;
    uint32_t playingTimeMinutes = 0;
    uint32_t age = 0;
    uint16_t weight = 1000;
    uint8_t health = 100;
    uint8_t happiness = 100;
    uint32_t money = 0;
    uint32_t points = 0;
    uint8_t dirty = 0;
    Sickness sickness = Sickness::NONE;
    unsigned long sicknessEndTime = 0;
    bool isSleeping = false;
    uint8_t fatigue = 0;
    unsigned long sleepStartTime = 0;
    uint8_t poopCount = 0;
    uint8_t hunger = 0;
    Language selectedLanguage = LANGUAGE_UNINITIALIZED; 
    WeatherType currentWeather = WeatherType::NONE;
    unsigned long nextWeatherChangeTime = 0;
    PrequelStage completedPrequelStage = PrequelStage::NONE; 
    uint32_t FlappyTuckCoins = 0;         // <<< NEW MEMBER
    uint32_t FlappyTuckHighScore = 0;     // <<< NEW MEMBER
    // --- End Member Variables ---

private:
    // --- Private Helpers ---
    void updateAgeBasedOnPoints();
    void updateGlobalLanguage();
    void checkAndApplyConsequences(); // <<< NEW
    // --- End Private Helpers ---

public:
    // --- Lifecycle & Persistence ---
    void load();
    void save();
    void reset();
    void clearPrefs();
    void deepSleepWakeUp(int minutesSlept);
    // --- End Lifecycle & Persistence ---

    // --- Stat Modifiers ---
    void addPoints(uint32_t amount);
    void setHealth(uint8_t value);
    void setHappiness(uint8_t value);
    void setDirty(uint8_t value);
    void setFatigue(uint8_t value);
    void setHunger(uint8_t value);
    void setSickness(Sickness value, unsigned long durationMillis = 0);
    void setIsSleeping(bool value);
    void setPoopCount(uint8_t value);
    void setLanguage(Language value);
    void setWeather(WeatherType type, unsigned long nextChange);
    void setCompletedPrequelStage(PrequelStage stage); 
    // --- End Stat Modifiers ---

    // --- Getters with Instantaneous Modifiers ---
    uint8_t getModifiedHappiness() const;
    uint8_t getModifiedFatigue() const;
    uint8_t getModifiedDirty() const;
    uint8_t getModifiedHealth() const;
    // --- End Getters ---

    // --- Status Checks & Info ---
    const char* getSicknessString() const;
    bool isSick() const;
    bool updateSickness(unsigned long currentTime);
    // --- End Status Checks & Info ---

    // --- Accumulated Stat Update Function ---
    bool updateAccumulatedStats(unsigned long currentTime, unsigned long timeDeltaMinutes);

};

#endif // GAMESTATS_H