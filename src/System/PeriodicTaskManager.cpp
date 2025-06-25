#include "PeriodicTaskManager.h"
#include "GameStats.h"
#include "character/CharacterManager.h"
#include "SerialForwarder.h"
#include <algorithm> 
#include "../System/GameContext.h" // <<< NEW INCLUDE

PeriodicTaskManager::PeriodicTaskManager(GameContext& context) : // Takes GameContext
    _context(context), // Store context reference
    _statSaveCounter(0)
{
    debugPrint("TASK","PeriodicTaskManager Initialized with GameContext.");
}

void PeriodicTaskManager::init() {
    unsigned long currentTime = millis();
    _lastMinuteCheckTime = currentTime;
    _lastSicknessCheckTime = currentTime;
    _lastStatAccumulationTime = currentTime;
    debugPrint("TASK","PeriodicTaskManager: Timers initialized.");
}

void PeriodicTaskManager::update(unsigned long currentTime, unsigned long lastActivityTime) {
    if (!_context.gameStats || !_context.characterManager) { // Access via context
        debugPrint("TASK","PeriodicTaskManager Error: GameStats or CharacterManager (via context) is null!");
        return;
    }
    GameStats* gameStats = _context.gameStats;
    CharacterManager* characterManager = _context.characterManager;


    if (currentTime - _lastMinuteCheckTime >= ONE_MINUTE_MILLIS) {
        unsigned long minutesPassed = (currentTime - _lastMinuteCheckTime) / ONE_MINUTE_MILLIS;
        _lastMinuteCheckTime += minutesPassed * ONE_MINUTE_MILLIS;

        if (currentTime - lastActivityTime < (minutesPassed + 1) * ONE_MINUTE_MILLIS) { 
            gameStats->playingTimeMinutes += minutesPassed;
            debugPrintf("TASK","PeriodicTask: playingTimeMinutes updated by %lu to %u\n", minutesPassed, gameStats->playingTimeMinutes);
        }
    }

    if (currentTime - _lastStatAccumulationTime >= STAT_ACCUMULATION_INTERVAL_MS) {
        unsigned long deltaMinutes = (currentTime - _lastStatAccumulationTime) / STAT_ACCUMULATION_INTERVAL_MS; // Use STAT_ACCUMULATION_INTERVAL_MS for delta calc too
        _lastStatAccumulationTime += deltaMinutes * STAT_ACCUMULATION_INTERVAL_MS; 

        if (deltaMinutes > 0) {
            bool needsSave = gameStats->updateAccumulatedStats(currentTime, deltaMinutes);
            if (needsSave) {
                _statSaveCounter++;
                if (_statSaveCounter >= 5) { 
                    gameStats->save();
                    _statSaveCounter = 0;
                    debugPrint("TASK","PeriodicTask: Saving accumulated stats due to changes.");
                }
            }
        }
    }

    if (currentTime - _lastSicknessCheckTime >= SICKNESS_CHECK_INTERVAL_MS) {
        _lastSicknessCheckTime = currentTime;
        
        bool sicknessCleared = gameStats->updateSickness(currentTime);
        if (sicknessCleared) {
            debugPrint("TASK","PeriodicTask: Sickness duration ended.");
            gameStats->save(); 
        }
        if (!gameStats->isSick()) {
            checkAndApplySickness(currentTime); 
        }
    }
}

void PeriodicTaskManager::checkAndApplySickness(unsigned long currentTime) {
    if (!_context.gameStats || !_context.characterManager || _context.gameStats->isSick()) { // Access via context
        return;
    }
    GameStats* gameStats = _context.gameStats;
    CharacterManager* characterManager = _context.characterManager;

    int baseChance = 2; 
    int probability = baseChance;

    if (gameStats->health < 50) probability += 10;
    if (gameStats->health < 20) probability += 15;
    if (gameStats->getModifiedDirty() > 70) probability += 15;
    if (gameStats->getModifiedHappiness() < 30) probability += 10;
    if (gameStats->getModifiedFatigue() > 80) probability += 10;

    switch (gameStats->currentWeather) {
        case WeatherType::RAINY:       probability += 3; break;
        case WeatherType::HEAVY_RAIN:  probability += 8; break;
        case WeatherType::STORM:       probability += 15; break;
        default: break;
    }
    probability = std::min(75, probability); 

    if (random(100) < probability) {
        const auto& availableSicknesses = characterManager->getAvailableSicknesses();
        if (availableSicknesses.empty()) {
            debugPrint("TASK","PeriodicTask Sickness Check: No sicknesses available for current level.");
            return;
        }
        Sickness newSickness = availableSicknesses[random(availableSicknesses.size())];
        unsigned long durationHours = random(1, 9); 
        unsigned long durationMillis = durationHours * 60 * 60 * 1000UL;

        gameStats->setSickness(newSickness, durationMillis);
        gameStats->save(); 
        debugPrintf("TASK","PeriodicTask: !!! Became Sick: %s (%d) for %lu hours !!!\n", gameStats->getSicknessString(), (int)newSickness, durationHours);
    }
}