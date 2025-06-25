#ifndef DEEP_SLEEP_CONTROLLER_H
#define DEEP_SLEEP_CONTROLLER_H

#include <Arduino.h>
#include "../DebugUtils.h"
#include "WakeUpInfo.h" // <<< INCLUDE NEW HEADER

// Forward declarations
class GameStats; 
struct GameContext; 
class U8G2;

// The WakeUpInfo struct is now defined in WakeUpInfo.h

class DeepSleepController {
public:
    DeepSleepController(U8G2* display); // U8G2 from context in Main.ino

    WakeUpInfo processWakeUp(GameStats* gameStats); // GameStats passed directly for now
    void goToSleep(bool forcedByCommand, int virtualDurationMin = 0);

private:
    U8G2* _display; 
};

#endif // DEEP_SLEEP_CONTROLLER_H