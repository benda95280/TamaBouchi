#ifndef PERIODIC_TASK_MANAGER_H
#define PERIODIC_TASK_MANAGER_H

#include <Arduino.h>
#include "../DebugUtils.h"   
#include "../System/GameContext.h" // <<< NEW INCLUDE

// Forward declarations
// class GameStats; // Now in GameContext
// class CharacterManager; // Now in GameContext
// class SerialForwarder; // Now in GameContext

class PeriodicTaskManager {
public:
    PeriodicTaskManager(GameContext& context); // Takes GameContext

    void init(); 
    void update(unsigned long currentTime, unsigned long lastActivityTime); 

private:
    GameContext& _context; // Store reference to GameContext
    // GameStats* _gameStats; // Access via _context.gameStats
    // CharacterManager* _characterManager; // Access via _context.characterManager
    // SerialForwarder* _logger; // Access via _context.serialForwarder

    unsigned long _lastMinuteCheckTime = 0;
    unsigned long _lastSicknessCheckTime = 0;
    unsigned long _lastStatAccumulationTime = 0;
    
    static const unsigned long ONE_MINUTE_MILLIS = 60000UL;
    static const unsigned long SICKNESS_CHECK_INTERVAL_MS = 10 * ONE_MINUTE_MILLIS; 
    static const unsigned long STAT_ACCUMULATION_INTERVAL_MS = 1 * ONE_MINUTE_MILLIS;

    void checkAndApplySickness(unsigned long currentTime);
    int _statSaveCounter = 0; 
};

#endif // PERIODIC_TASK_MANAGER_H