#include "DeepSleepController.h"
#include "GameStats.h" // <<< ADDED THIS INCLUDE
#include "esp_sleep.h"
#include "esp_timer.h"
#include <sys/time.h> 
#include "GlobalMappings.h" 
#include <U8g2lib.h> 
#include "../System/GameContext.h" // <<< NEW INCLUDE (though not directly used by DeepSleepController methods yet)

extern RTC_DATA_ATTR uint32_t rtc_sleep_entry_epoch_sec;
extern RTC_DATA_ATTR bool wokeFromDeepSleep; 
extern RTC_DATA_ATTR int rtc_forced_sleep_duration_minutes;
extern RTC_DATA_ATTR bool rtc_was_forced_sleep_with_duration;

DeepSleepController::DeepSleepController(U8G2* display) : // U8G2 is from GameContext in main
    _display(display) {}

WakeUpInfo DeepSleepController::processWakeUp(GameStats* gameStats) { // GameStats still direct for now
    WakeUpInfo info;
    info.wasWakeUpFromDeepSleep = false;
    info.sleepDurationMinutes = 0;

    debugPrint("DEEP_SLEEP","DeepSleepController: --- Wake Up Processing START ---");
    debugPrintf("DEEP_SLEEP","  RTC values PRE-check: wokeFromDeepSleep=%d, rtc_sleep_entry_epoch_sec=%u, rtc_was_forced_sleep_with_duration=%d, rtc_forced_sleep_duration_minutes=%d\n",
                        ::wokeFromDeepSleep, ::rtc_sleep_entry_epoch_sec, ::rtc_was_forced_sleep_with_duration, ::rtc_forced_sleep_duration_minutes);
    

    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    debugPrintf("DEEP_SLEEP","  Wakeup Cause: %d\n", wakeup_reason);

    if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
        if (::wokeFromDeepSleep) { 
            info.wasWakeUpFromDeepSleep = true;
            debugPrint("DEEP_SLEEP","  Determined: Woke from deep sleep (EXT0 + RTC flag was true).");

            timeval tv_now;
            gettimeofday(&tv_now, nullptr);
            uint32_t current_epoch_sec = tv_now.tv_sec;
            int actualSleepSeconds = 0;

            if (::rtc_sleep_entry_epoch_sec > 0 && ::rtc_sleep_entry_epoch_sec <= current_epoch_sec) {
                actualSleepSeconds = current_epoch_sec - ::rtc_sleep_entry_epoch_sec;
                debugPrintf("DEEP_SLEEP","  Epoch times: Entry=%u, Current=%u. Calculated actual sleep: %d seconds.\n", ::rtc_sleep_entry_epoch_sec, current_epoch_sec, actualSleepSeconds);
            } else {
                debugPrintf("DEEP_SLEEP","  Sleep entry epoch time invalid or not sensible. RTC rtc_sleep_entry_epoch_sec: %u, current_epoch_sec: %u. Defaulting actual sleep to 0 sec.\n", ::rtc_sleep_entry_epoch_sec, current_epoch_sec);
                actualSleepSeconds = 0;
            }
            int actualSleepMinutes = actualSleepSeconds / 60;

            int effectiveSleepMinutes = actualSleepMinutes;
            if (::rtc_was_forced_sleep_with_duration) {
                debugPrintf("DEEP_SLEEP","  Forced sleep duration override. Virtual: %d min, Actual: %d min.\n", ::rtc_forced_sleep_duration_minutes, actualSleepMinutes);
                effectiveSleepMinutes = std::max(actualSleepMinutes, ::rtc_forced_sleep_duration_minutes);
            }
            info.sleepDurationMinutes = effectiveSleepMinutes;
            debugPrintf("DEEP_SLEEP","  Effective sleep minutes for stats: %d\n", effectiveSleepMinutes);

            if (gameStats) {
                debugPrint("DEEP_SLEEP","  Calling gameStats->deepSleepWakeUp()...");
                gameStats->deepSleepWakeUp(effectiveSleepMinutes);
            } else {
                debugPrint("DEEP_SLEEP","  Warning: gameStats NULL during deep sleep wake up handling.");
            }
        } else {
            debugPrint("DEEP_SLEEP","  Determined: Woke via EXT0, but RTC wokeFromDeepSleep flag was false. Treating as normal boot/reset during sleep attempt.");
        }
    } else {
        debugPrintf("DEEP_SLEEP","  Determined: Boot reason %d (Not an EXT0 deep sleep reset)\n", wakeup_reason);
    }

    ::wokeFromDeepSleep = false;
    ::rtc_sleep_entry_epoch_sec = 0;
    ::rtc_was_forced_sleep_with_duration = false;
    ::rtc_forced_sleep_duration_minutes = 0;
    debugPrint("DEEP_SLEEP","  RTC sleep flags reset for next cycle.");
    debugPrint("DEEP_SLEEP","DeepSleepController: --- Wake Up Processing END ---");
    
    return info;
}

void DeepSleepController::goToSleep(bool forcedByCommand, int virtualDurationMin) {
    debugPrintf("DEEP_SLEEP","DeepSleepController: goToSleep called. Forced: %s, Virtual Duration: %d min\n",
                        forcedByCommand ? "Yes" : "No", virtualDurationMin);

    if (_display) {
        _display->setPowerSave(1); 
        debugPrint("DEEP_SLEEP","  Display power save ON.");
    }
    delay(100); 

    timeval tv_now_sleep;
    gettimeofday(&tv_now_sleep, nullptr);
    ::rtc_sleep_entry_epoch_sec = tv_now_sleep.tv_sec;
    debugPrintf("DEEP_SLEEP","  Setting RTC sleep entry epoch: %u\n", ::rtc_sleep_entry_epoch_sec);

    if (forcedByCommand && virtualDurationMin > 0) {
        ::rtc_forced_sleep_duration_minutes = virtualDurationMin;
        ::rtc_was_forced_sleep_with_duration = true;
        debugPrintf("DEEP_SLEEP","  Forced sleep with duration: %d minutes.\n", virtualDurationMin);
    } else {
        ::rtc_forced_sleep_duration_minutes = 0;
        ::rtc_was_forced_sleep_with_duration = false;
    }
    
    ::wokeFromDeepSleep = true; 

    esp_sleep_enable_ext0_wakeup(WAKEUP_BUTTON_PIN, 0); 
    debugPrint("DEEP_SLEEP","  Configured wake-up on GPIO 0 (LOW). Going to sleep NOW.");
    
    esp_deep_sleep_start();
}