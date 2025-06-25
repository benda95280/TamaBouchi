#ifndef WAKE_UP_INFO_H
#define WAKE_UP_INFO_H

#include <stdint.h> // For bool

// This struct holds information about a device wake-up event.
// It's in its own header to be safely included by multiple system components
// without creating circular header dependencies.
struct WakeUpInfo {
    bool wasWakeUpFromDeepSleep = false;
    int sleepDurationMinutes = 0;
};

#endif // WAKE_UP_INFO_H