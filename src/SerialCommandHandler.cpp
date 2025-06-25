#include "SerialCommandHandler.h"
#include "WiFiManager.h"
#include "BluetoothManager.h" 
#include "GameStats.h"
#include <WiFi.h>
#include <SerialForwarder.h> 
#include "Weather/WeatherManager.h" 
#include "EDGE.h"           
#include "Scene.h"          
#include "Scenes/SceneMain/MainScene.h" 
#include "Weather/WeatherTypes.h" 
#include "character/CharacterManager.h" 
#include "DialogBox/DialogBox.h" 
#include "Scenes/SceneSleeping/SleepingScene.h" 
#include "System/DeepSleepController.h" 
#include <sys/time.h> 
#include "esp_heap_caps.h" 
#include <freertos/FreeRTOS.h> 
#include "DebugUtils.h" 
#include <freertos/task.h>     
#include "GlobalMappings.h" 
#include "System/GameContext.h" // Ensure GameContext is included
#include "esp_wifi.h" // For esp_wifi_set_ps
#include "esp_bt.h"   // For esp_bt_controller_get_status

extern unsigned long lastActivityTime; 

extern String nextSceneName; 
extern bool replaceSceneStack;
extern bool sceneChangeRequested;


#define PROMPT_STR "Tama> " 

SerialCommandHandler::SerialCommandHandler(GameContext& context) 
    : _context(context) // Store context reference
{
}

void SerialCommandHandler::init() {
    _context.serialForwarder->print(PROMPT_STR); 
}

void SerialCommandHandler::handleSerialInput() { 
    while (Serial.available() > 0) { 
        char receivedChar = Serial.read(); 
        if (receivedChar == '\n' || receivedChar == '\r') { 
            if (_commandBuffer.length() > 0) { 
                _context.serialForwarder->println(); 
                processSerialCommand(_commandBuffer); 
                _commandBuffer = ""; 
                _context.serialForwarder->print(PROMPT_STR); 
            } else { 
                _context.serialForwarder->println(); 
                _context.serialForwarder->print(PROMPT_STR); 
            } 
        } else if (receivedChar == '\b' || receivedChar == 127) { 
            if (_commandBuffer.length() > 0) { 
                _commandBuffer.remove(_commandBuffer.length() - 1); 
                Serial.write('\b'); Serial.write(' '); Serial.write('\b'); 
            } 
        } else if (isPrintable(receivedChar)) { 
            if (_commandBuffer.length() < 127) { 
                _commandBuffer += receivedChar; 
                Serial.write(receivedChar); 
            } 
        } 
    }
}

void SerialCommandHandler::processSerialCommand(String command) {
    command.trim();
    updateLastActivityTime(); 

    if (!_context.gameStats) { _context.serialForwarder->println("Error: GameStats object not ready."); return; }

    if (command.equalsIgnoreCase("help")) {
        printHelp();
    } else if (command.equalsIgnoreCase("get_system_info")) { 
        printSystemInfo();
    } else if (command.startsWith("set_wifi_ssid ")) {
        String ssid = command.substring(14); ssid.trim(); 
        if (ssid.length() > 0) { 
            _context.preferences->begin(PREF_WIFI_NAMESPACE, false); 
            _context.preferences->putString("temp_ssid", ssid); 
            _context.preferences->end(); 
            _context.serialForwarder->printf("Temporary SSID set to: '%s'. Use 'save_wifi' to apply.\n", ssid.c_str()); 
        } else { _context.serialForwarder->println("Error: SSID cannot be empty."); }
    } else if (command.startsWith("set_wifi_pwd ")) {
        String pwd = command.substring(13); pwd.trim(); 
        _context.preferences->begin(PREF_WIFI_NAMESPACE, false); 
        _context.preferences->putString("temp_pwd", pwd); 
        _context.preferences->end(); 
        _context.serialForwarder->println("Temporary Password set. Use 'save_wifi' to apply.");
    } else if (command.equalsIgnoreCase("save_wifi")) {
        _context.preferences->begin(PREF_WIFI_NAMESPACE, false); 
        String temp_ssid = _context.preferences->getString("temp_ssid", ""); 
        String temp_pwd = _context.preferences->getString("temp_pwd", ""); 
        _context.preferences->remove("temp_ssid"); _context.preferences->remove("temp_pwd"); 
        _context.preferences->end(); 
        if (temp_ssid.length() > 0) { 
            _context.wifiManager->saveCredentials(temp_ssid, temp_pwd); 
            _context.serialForwarder->println("WiFi credentials saved. Restarting WiFi connection..."); 
            WiFi.disconnect(); 
            _context.serialForwarder->println("Note: WiFi connection might require manual restart or device reboot."); 
        } else { _context.serialForwarder->println("Error: No temporary SSID set."); }
    } else if (command.startsWith("enable_bt_scan ")) {
         String val = command.substring(15); val.trim(); 
         if (val == "1") { _context.bluetoothManager->enableBluetoothScanning(true); } 
         else if (val == "0") { _context.bluetoothManager->enableBluetoothScanning(false); } 
         else { _context.serialForwarder->println("Error: Use 1 to enable or 0 to disable."); }
    } else if (command.equalsIgnoreCase("bt_stack_on")) {
        _context.bluetoothManager->fullyEnableStack();
        _context.serialForwarder->println("Bluetooth stack enabled.");
    } else if (command.equalsIgnoreCase("bt_stack_off")) {
        _context.bluetoothManager->fullyDisableStack();
        _context.serialForwarder->println("Bluetooth stack disabled.");
    } else if (command.equalsIgnoreCase("toggle_wifi_ps")) {
        if (!_context.bluetoothManager->isStackActive()) {
            bool currentSleepState = WiFi.getSleep();
            bool newSleepState = !currentSleepState;
            WiFi.setSleep(newSleepState);
            bool finalState = WiFi.getSleep();
            _context.serialForwarder->printf("WiFi power saving toggled. New state: %s\n", finalState ? "ENABLED" : "DISABLED");
        } else {
            _context.serialForwarder->println("Error: Cannot toggle WiFi power saving while Bluetooth stack is active.");
        }
    } else if (command.equalsIgnoreCase("forget_bt")) {
        _context.bluetoothManager->forgetBluetoothKeys(); 
        _context.serialForwarder->println("Forgetting Bluetooth devices.");
    } else if (command.startsWith("reset ")) {
        String target = command.substring(6); target.trim(); 
        if (target.equalsIgnoreCase("tama")) { 
            _context.gameStats->clearPrefs(); 
            _context.serialForwarder->println("Tama stats reset."); 
        } else if (target.equalsIgnoreCase("settings")) { 
            _context.wifiManager->clearCredentials(); 
            _context.bluetoothManager->forgetBluetoothKeys(); 
            _context.serialForwarder->println("Settings reset. Restart recommended."); 
        } else if (target.equalsIgnoreCase("all")) { 
            _context.gameStats->clearPrefs(); 
            _context.wifiManager->clearCredentials(); 
            _context.bluetoothManager->forgetBluetoothKeys(); 
            _context.serialForwarder->println("ALL data reset. Restart recommended."); 
        } else { _context.serialForwarder->println("Error: Invalid reset target. Use 'tama', 'settings', or 'all'."); }
    } else if (command.equalsIgnoreCase("get_stats")) {
        _context.serialForwarder->println("\nCurrent Game Stats:"); 
        _context.serialForwarder->printf("  Running Time: %lu ms\n", (unsigned long)_context.gameStats->runningTime); 
        _context.serialForwarder->printf("  Playing Time: %u min\n", _context.gameStats->playingTimeMinutes); 
        _context.serialForwarder->printf("  Points: %u\n", _context.gameStats->points); 
        _context.serialForwarder->printf("  Age: %u\n", _context.gameStats->age); 
        _context.serialForwarder->printf("  Weight: %u\n", _context.gameStats->weight); 
        _context.serialForwarder->printf("  Health: %u\n", _context.gameStats->health); 
        _context.serialForwarder->printf("  Happiness: %u\n", _context.gameStats->happiness); 
        _context.serialForwarder->printf("  Dirty: %u\n", _context.gameStats->dirty); 
        _context.serialForwarder->printf("  Hunger: %u\n", _context.gameStats->hunger); 
        _context.serialForwarder->printf("  Fatigue: %u\n", _context.gameStats->fatigue); 
        _context.serialForwarder->printf("  Sickness: %s\n", _context.gameStats->getSicknessString()); 
        _context.serialForwarder->printf("  Sleeping: %s\n", _context.gameStats->isSleeping ? "Yes" : "No"); 
        _context.serialForwarder->printf("  Poop Count: %u\n", _context.gameStats->poopCount); 
        _context.serialForwarder->printf("  Money: %u\n", _context.gameStats->money); 
        _context.serialForwarder->printf("  Language: %d\n", (int)_context.gameStats->selectedLanguage); 
        _context.serialForwarder->printf("  Prequel Stage: %d\n", (int)_context.gameStats->completedPrequelStage);
    } else if (command.startsWith("add_points ")) {
        String amountStr = command.substring(11); int amount = amountStr.toInt(); 
        if (amount > 0) { 
            _context.gameStats->addPoints(amount); 
            _context.serialForwarder->printf("Added %d points. Total points: %u, New Age: %u\n", amount, _context.gameStats->points, _context.gameStats->age); 
        } else { _context.serialForwarder->println("Error: Invalid amount."); }
    } else if (command.equalsIgnoreCase("get_weather")) {
        if (!_context.weatherManager) { _context.serialForwarder->println("Error: WeatherManager not ready."); return; } 
        long remainingMs = (long)_context.gameStats->nextWeatherChangeTime - (long)millis(); if (remainingMs < 0) remainingMs = 0; 
        _context.serialForwarder->printf("Current Weather: %s (%d)\n", WeatherManager::weatherTypeToString(_context.gameStats->currentWeather), (int)_context.gameStats->currentWeather); 
        _context.serialForwarder->printf("Next Change In: %ld ms (~%ld mins)\n", remainingMs, remainingMs / 60000); 
        _context.serialForwarder->printf("Rain Intensity State: %s\n", WeatherManager::rainStateToString(_context.weatherManager->getRainIntensityState())); 
        _context.serialForwarder->printf("Actual Wind Factor: %.2f\n", _context.weatherManager->getActualWindFactor()); 
        _context.serialForwarder->printf("Rain Drop Density: %d%%\n", _context.weatherManager->getIntensityAdjustedDensity());
    } else if (command.startsWith("set_weather ")) {
        if (!_context.weatherManager) { _context.serialForwarder->println("Error: WeatherManager not ready."); return; } 
        String args = command.substring(12); args.trim(); int spaceIndex = args.indexOf(' '); 
        String typeStr; String durationStr = ""; 
        if (spaceIndex != -1) { typeStr = args.substring(0, spaceIndex); durationStr = args.substring(spaceIndex + 1); } 
        else { typeStr = args; } 
        typeStr.toLowerCase(); WeatherType newType; 
        if (!stringToWeatherType(typeStr, newType)) { _context.serialForwarder->println("Error: Invalid weather type."); } 
        else { 
            unsigned long durationMs = 5 * 60000; 
            if (durationStr.length() > 0) { 
                long durationMin = durationStr.toInt(); 
                if (durationMin > 0) { durationMs = durationMin * 60000UL; } 
                else { _context.serialForwarder->println("Warning: Invalid duration, using 5 mins."); } 
            } 
            _context.weatherManager->forceWeather(newType, durationMs); 
            _context.serialForwarder->printf("Weather forced to %s for %lu ms.\n", WeatherManager::weatherTypeToString(newType), durationMs); 
        }
    } else if (command.startsWith("set_sickness ")) {
         if (!_context.characterManager) { _context.serialForwarder->println("Error: CharacterManager not ready."); return; } 
         String args = command.substring(13); args.trim(); int spaceIndex = args.indexOf(' '); 
         String typeStr; String durationStr = ""; 
         if (spaceIndex != -1) { typeStr = args.substring(0, spaceIndex); durationStr = args.substring(spaceIndex + 1); } 
         else { typeStr = args; } 
         typeStr.toLowerCase(); Sickness newSickness; unsigned long durationMillis = 0;
         if (!stringToSickness(typeStr, newSickness)) { _context.serialForwarder->println("Error: Invalid sickness type name."); }
         else if (newSickness != Sickness::NONE && !_context.characterManager->isSicknessAvailable(newSickness)) { 
             _context.serialForwarder->printf("Error: Sickness type '%s' is not available for the current character level (%u).\n", typeStr.c_str(), _context.characterManager->getCurrentManagedLevel()); 
         } else { 
             if (durationStr.length() > 0) { 
                 long durationHrs = durationStr.toInt(); 
                 if (durationHrs > 0) { durationMillis = durationHrs * 3600000UL; } 
                 else if (newSickness != Sickness::NONE) { _context.serialForwarder->println("Warning: Invalid duration, sickness indefinite."); } 
             } 
             _context.gameStats->setSickness(newSickness, durationMillis); 
             _context.gameStats->save(); 
             _context.serialForwarder->printf("Sickness set to %s", _context.gameStats->getSicknessString()); 
             if (durationMillis > 0) { _context.serialForwarder->printf(" for %lu hours.", durationMillis / 3600000UL); } 
             else if (newSickness != Sickness::NONE) { _context.serialForwarder->print(" (indefinite)"); } _context.serialForwarder->println(); 
         }
    }
    else if (command.startsWith("set_fatigue ")) {
        String valueStr = command.substring(12);
        valueStr.trim();
        int value = valueStr.toInt();
        if (value >= 0 && value <= 100) {
            _context.gameStats->setFatigue((uint8_t)value);
            _context.gameStats->save(); 
            _context.serialForwarder->printf("Fatigue set to %u\n", _context.gameStats->fatigue);
        } else {
            _context.serialForwarder->println("Error: Fatigue value must be between 0 and 100.");
        }
    }
    else if (command.startsWith("force_sleep")) { 
        String args = command.substring(11); 
        args.trim();                         
        handleForceSleep(args);              
    }
    else if (command.startsWith("anim_")) {
         if (!_context.sceneManager) { _context.serialForwarder->println("Error: SceneManager (via context) not available."); } 
         else { 
             Scene* currentScene = _context.sceneManager->getCurrentScene(); 
             if (currentScene && currentScene->getSceneType() == SceneType::MAIN) { 
                 MainScene* mainScene = static_cast<MainScene*>(currentScene); 
                 bool success = false; 
                 if (command.equalsIgnoreCase("anim_snooze")) { success = mainScene->triggerSnoozeAnimation(); } 
                 else if (command.equalsIgnoreCase("anim_lean")) { success = mainScene->triggerLeanAnimation(); } 
                 else if (command.equalsIgnoreCase("anim_fly") || command.equalsIgnoreCase("anim_path")) { success = mainScene->triggerPathAnimation(); } 
                 else { _context.serialForwarder->println("Error: Unknown animation command (use anim_snooze, anim_lean, anim_fly)."); } 
                 if (success) { _context.serialForwarder->println("Animation triggered."); } 
                 else { _context.serialForwarder->println("Failed to trigger animation."); } 
             } else { _context.serialForwarder->println("Error: MainScene not active for animation commands."); } 
         }
    }
    else if (command.startsWith("dialog_show ")) {
        handleDialogShow(command.substring(12));
    } else if (command.startsWith("dialog_temp ")) {
        handleDialogTemp(command.substring(12));
    }
    else if (command.equalsIgnoreCase("list_scenes")) {
        listScenes();
    } else if (command.startsWith("set_scene ")) {
        setScene(command.substring(10));
    }
    else if (command.startsWith("set_prequel_stage ")) {
        String stageValueStr = command.substring(18);
        stageValueStr.trim();
        PrequelStage stage;
        if (stringToPrequelStage(stageValueStr, stage)) {
            _context.gameStats->setCompletedPrequelStage(stage);
            _context.gameStats->save();
            _context.serialForwarder->printf("Prequel stage set to: %d\n", (int)stage);
        } else {
            _context.serialForwarder->println("Error: Invalid prequel stage value. Use 'NONE', 'LANG_SEL', 'S1', 'S2', 'S3', 'S4', 'FINISHED' or 0-6.");
        }
    }
    else if (command.equalsIgnoreCase("reboot")) {
        _context.serialForwarder->println("Rebooting..."); delay(100); ESP.restart();
    }
    else if (command.equalsIgnoreCase("debug_status")) {
        _context.serialForwarder->println(getDebugConfig().c_str());
    }
    else if (command.startsWith("debug_enable ")) {
        String feature = command.substring(13);
        feature.trim();
        feature.toUpperCase();
        if (setDebugFlag(feature.c_str(), true)) { _context.serialForwarder->printf("Debug enabled for feature: %s\n", feature.c_str()); } 
        else { _context.serialForwarder->printf("Error: Unknown feature '%s'. Use debug_status to see available features.\n", feature.c_str()); }
    }
    else if (command.startsWith("debug_disable ")) {
        String feature = command.substring(14);
        feature.trim();
        feature.toUpperCase();
        if (setDebugFlag(feature.c_str(), false)) { _context.serialForwarder->printf("Debug disabled for feature: %s\n", feature.c_str()); } 
        else { _context.serialForwarder->printf("Error: Unknown feature '%s'. Use debug_status to see available features.\n", feature.c_str()); }
    }
    else if (command.equalsIgnoreCase("debug_enable_all")) {
        setDebugFlag("MASTER", true); setDebugFlag("ANIMATOR", true); setDebugFlag("GAME_STATS", true); setDebugFlag("DIALOG_BOX", true); setDebugFlag("PATH_GENERATOR", true); setDebugFlag("CHARACTER_MANAGER", true); setDebugFlag("SCENES", true); setDebugFlag("WIFI_MANAGER", true); setDebugFlag("BLUETOOTH", true); setDebugFlag("SLEEP_CONTROLLER", true); setDebugFlag("HARDWARE_INPUT", true); setDebugFlag("SYSTEM", true);
        _context.serialForwarder->println("All debug features enabled.");
    }
    else if (command.equalsIgnoreCase("debug_disable_all")) {
        setDebugFlag("MASTER", false);
        _context.serialForwarder->println("All debug features disabled (master switch off).");
    }
    else {
        _context.serialForwarder->println("Error: Unknown command. Enter 'help' for list.");
    }
}

void SerialCommandHandler::printHelp() {
    _context.serialForwarder->println("\nAvailable Commands:");
    _context.serialForwarder->println("  help                      - Shows this help message.");
    _context.serialForwarder->println("  get_system_info           - Displays various system metrics."); 
    _context.serialForwarder->println("  set_wifi_ssid <ssid>    - Sets the WiFi SSID (requires save_wifi).");
    _context.serialForwarder->println("  set_wifi_pwd <password> - Sets the WiFi password (requires save_wifi).");
    _context.serialForwarder->println("  save_wifi                 - Saves SSID/Password and restarts WiFi.");
    _context.serialForwarder->println("  toggle_wifi_ps            - Toggles WiFi power saving (only if BT is off).");
    _context.serialForwarder->println("  enable_bt_scan <1|0>    - Enables (1) or disables (0) Bluetooth scanning.");
    _context.serialForwarder->println("  bt_stack_on               - Enables the entire Bluetooth stack.");
    _context.serialForwarder->println("  bt_stack_off              - Disables the entire Bluetooth stack.");
    _context.serialForwarder->println("  forget_bt                 - Forgets all paired Bluetooth devices.");
    _context.serialForwarder->println("  reset <tama|settings|all> - Resets Tama stats, WiFi/BT settings, or all.");
    _context.serialForwarder->println("  get_stats                 - Displays current game stats.");
    _context.serialForwarder->println("  add_points <amount>       - Adds points to the tama.");
    _context.serialForwarder->println("  get_weather               - Shows current weather, next change, intensity, wind.");
    _context.serialForwarder->println("  set_weather <type> [dur]  - Sets weather (none,sunny,cloudy,rainy,heavy_rain,snowy,heavy_snow,storm,rainbow) [opt. duration mins]."); 
    _context.serialForwarder->println("  set_sickness <type> [dur] - Sets sickness (none,cold,hot,diarrhea,headache) [opt. duration hours].");
    _context.serialForwarder->println("  set_fatigue <0-100>       - Sets current fatigue level.");
    _context.serialForwarder->println("  force_sleep [minutes]     - Forces deep sleep. Opt. sets virtual sleep duration.");
    _context.serialForwarder->println("  set_prequel_stage <stage> - Sets prequel stage (NONE, LANG_SEL, S1, S2, S3, S4, FINISHED or 0-6).");
    _context.serialForwarder->println("  dialog_show <message>     - Shows a permanent message in active scene's dialog.");
    _context.serialForwarder->println("  dialog_temp <ms> <msg>    - Shows a temporary message in active scene's dialog.");
    _context.serialForwarder->println("  anim_snooze               - Triggers the Snooze animation (if MainScene active).");
    _context.serialForwarder->println("  anim_lean                 - Triggers the Lean animation (if MainScene active).");
    _context.serialForwarder->println("  anim_fly                  - Triggers a dynamic flying animation (if MainScene active).");
    _context.serialForwarder->println("  list_scenes               - Lists available scene names.");
    _context.serialForwarder->println("  set_scene <name>          - Sets the current scene by its registered name.");
    _context.serialForwarder->println("  reboot                    - Reboots the device.");
    _context.serialForwarder->println("\nDebugging Commands:");
    _context.serialForwarder->println("  debug_status              - Shows current debug configuration.");
    _context.serialForwarder->println("  debug_enable <feature>    - Enables debugging for a specific feature.");
    _context.serialForwarder->println("  debug_disable <feature>   - Disables debugging for a specific feature.");
    _context.serialForwarder->println("  debug_enable_all          - Enables all debugging features.");
    _context.serialForwarder->println("  debug_disable_all         - Disables all debugging features.");
}

void SerialCommandHandler::printSystemInfo() {
    _context.serialForwarder->println("\n--- System Information ---");
    _context.serialForwarder->printf("Heap Free: %u bytes\n", esp_get_free_heap_size());
    _context.serialForwarder->printf("Heap Min Free: %u bytes\n", esp_get_minimum_free_heap_size());
    _context.serialForwarder->printf("Heap Largest Free Block: %u bytes\n", heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT));
    _context.serialForwarder->printf("Serial (UART0) Baud Rate: %lu\n", Serial.baudRate());
    _context.serialForwarder->printf("Serial (UART0) TX Buffer Free: %d bytes\n", Serial.availableForWrite());
    _context.serialForwarder->println("Serial (UART0) Pins (Default): TX=GPIO1, RX=GPIO3");
    _context.serialForwarder->println("Serial (UART0) Config (Default): 8N1 (8 data bits, No parity, 1 stop bit)");
    _context.serialForwarder->printf("Current Task Name: %s\n", pcTaskGetName(NULL));
    _context.serialForwarder->printf("Current Task Priority: %u\n", (unsigned int)uxTaskPriorityGet(NULL));
    _context.serialForwarder->printf("Number of Tasks: %u\n", (unsigned int)uxTaskGetNumberOfTasks());
    _context.serialForwarder->print("WiFi Status: ");
    switch (WiFi.status()) {
        case WL_IDLE_STATUS: _context.serialForwarder->println("IDLE"); break;
        case WL_NO_SSID_AVAIL: _context.serialForwarder->println("No SSID Available"); break;
        case WL_SCAN_COMPLETED: _context.serialForwarder->println("Scan Completed"); break;
        case WL_CONNECTED: _context.serialForwarder->println("Connected"); break;
        case WL_CONNECT_FAILED: _context.serialForwarder->println("Connect Failed"); break;
        case WL_CONNECTION_LOST: _context.serialForwarder->println("Connection Lost"); break;
        case WL_DISCONNECTED: _context.serialForwarder->println("Disconnected"); break;
        default: _context.serialForwarder->println("Unknown"); break;
    }
    if (WiFi.status() == WL_CONNECTED) {
        _context.serialForwarder->printf("  SSID: %s\n", WiFi.SSID().c_str());
        _context.serialForwarder->printf("  IP: %s\n", WiFi.localIP().toString().c_str());
        _context.serialForwarder->printf("  RSSI: %d dBm\n", WiFi.RSSI());
        _context.serialForwarder->printf("  Sleep: %s\n", WiFi.getSleep() ? "Enabled" : "Disabled");
    } else {
        _context.serialForwarder->printf("  Configured SSID: %s\n", _context.wifiManager->getSSID().c_str());
    }
    _context.serialForwarder->print("Bluetooth Discovery: ");
    if (uni_bt_enable_new_connections_is_enabled()) { _context.serialForwarder->println("Enabled (Scanning)"); } 
    else { _context.serialForwarder->println("Disabled"); }
    _context.serialForwarder->print("Bluetooth Stack Status: ");
    esp_bt_controller_status_t bt_status = esp_bt_controller_get_status();
    switch(bt_status) {
        case ESP_BT_CONTROLLER_STATUS_IDLE: _context.serialForwarder->println("IDLE (Off)"); break;
        case ESP_BT_CONTROLLER_STATUS_ENABLED: _context.serialForwarder->println("ENABLED (On)"); break;
        case ESP_BT_CONTROLLER_STATUS_INITED: _context.serialForwarder->println("INITED (On)"); break;
        default: _context.serialForwarder->println("UNKNOWN"); break;
    }
    _context.serialForwarder->print("Bluetooth Connected Devices: ");
    int connected_devices = 0;
    for (int i = 0; i < BP32_MAX_GAMEPADS; ++i) {
        uni_hid_device_t* d = uni_hid_device_get_instance_for_idx(i);
        if (d && uni_bt_conn_is_connected(&d->conn)) {
            if (connected_devices > 0) _context.serialForwarder->print(", ");
            _context.serialForwarder->printf("%s (Idx %d)", d->name, i);
            connected_devices++;
        }
    }
    if (connected_devices == 0) _context.serialForwarder->println("None"); else _context.serialForwarder->println();
    _context.serialForwarder->printf("Uptime: %lu ms\n", millis());
    _context.serialForwarder->printf("Last User Activity: %lu ms ago\n", millis() - lastActivityTime); 
    
    if (_context.sceneManager) {
        Scene* currentScene = _context.sceneManager->getCurrentScene();
        String currentSceneNameStr = _context.sceneManager->getCurrentSceneName(); // Renamed local var
        if (currentScene) {
             _context.serialForwarder->printf("Current Scene: %s (Type: %d)\n",
                           currentSceneNameStr.c_str(),
                           (int)currentScene->getSceneType());
        } else {
            _context.serialForwarder->println("Current Scene: None (SceneManager active but no scene set)");
        }
    } else {
        _context.serialForwarder->println("Current Scene: SceneManager not available via context.");
    }
    _context.serialForwarder->println("--- End System Information ---");
}

void SerialCommandHandler::handleDialogShow(const String& args) {
    if (!_context.sceneManager) { _context.serialForwarder->println("Error: SceneManager not ready for dialog."); return; }
    Scene* currentScene = _context.sceneManager->getCurrentScene();
    DialogBox* dialog = nullptr;
    if (currentScene) { dialog = currentScene->getDialogBox(); }
    if (dialog) {
        String processedArgs = args;
        processedArgs.replace("\\r\\n", "\n"); processedArgs.replace("\\n", "\n"); processedArgs.replace("\\r", "\n");
        _context.serialForwarder->printf("Showing permanent dialog: %s\n", processedArgs.c_str());
        dialog->show(processedArgs.c_str());
    } else { _context.serialForwarder->println("Error: Current scene does not support dialogs or dialog not initialized."); }
}

void SerialCommandHandler::handleDialogTemp(const String& args) {
    if (!_context.sceneManager) { _context.serialForwarder->println("Error: SceneManager not ready for dialog."); return; }
    int firstSpace = args.indexOf(' ');
    if (firstSpace == -1) { _context.serialForwarder->println("Error: Usage: dialog_temp <duration_ms> <message>"); return; }
    String durationStr = args.substring(0, firstSpace);
    String message = args.substring(firstSpace + 1);
    durationStr.trim(); message.trim();
    unsigned long durationMs = durationStr.toInt();
    if (durationMs == 0 && durationStr != "0") { _context.serialForwarder->println("Error: Invalid duration specified."); return; }
    if (message.length() == 0) { _context.serialForwarder->println("Error: Message cannot be empty."); return; }
    Scene* currentScene = _context.sceneManager->getCurrentScene();
    DialogBox* dialog = nullptr;
    if (currentScene) { dialog = currentScene->getDialogBox(); }
    if (dialog) {
        String processedMessage = message;
        processedMessage.replace("\\r\\n", "\n"); processedMessage.replace("\\n", "\n"); processedMessage.replace("\\r", "\n");
        _context.serialForwarder->printf("Showing temporary dialog for %lu ms: %s\n", durationMs, processedMessage.c_str());
        dialog->showTemporary(processedMessage.c_str(), durationMs);
    } else { _context.serialForwarder->println("Error: Current scene does not support dialogs or dialog not initialized."); }
}

void SerialCommandHandler::handleForceSleep(const String& args) {
    if (!_context.deepSleepController) { _context.serialForwarder->println("Error: DeepSleepController not ready for force_sleep."); return; }
    int forcedDurationMinutes = 0; bool durationProvided = false;
    if (args.length() > 0) {
        forcedDurationMinutes = args.toInt();
        if (forcedDurationMinutes > 0) { durationProvided = true; } 
        else { _context.serialForwarder->println("Warning: Invalid duration for force_sleep. Sleeping without virtual duration."); forcedDurationMinutes = 0; }
    }
    _context.deepSleepController->goToSleep(true, forcedDurationMinutes);
}

void SerialCommandHandler::listScenes() {
    _context.serialForwarder->println("Registered Scene Names:");
    if (!_context.sceneManager) { _context.serialForwarder->println("  Error: SceneManager not available to list scenes."); return; }
    std::vector<String> registeredSceneNames = _context.sceneManager->getRegisteredSceneNames();
    if (registeredSceneNames.empty()) { _context.serialForwarder->println("  No scenes registered with SceneManager."); } 
    else { for (const String& name : registeredSceneNames) { _context.serialForwarder->printf("  %s\n", name.c_str()); } }
}

void SerialCommandHandler::setScene(const String& sceneName) { 
    if (!_context.sceneManager) { _context.serialForwarder->println("Error: SceneManager not ready to set scene."); return; }
    if (_context.sceneManager->getFactoryByName(sceneName)) {
        _context.sceneManager->requestSetCurrentScene(sceneName); // Use the new method
        // The following confirmation message is now redundant as SceneManager logs the request.
        // _context.serialForwarder->printf("Scene change requested to: %s\n", sceneName.c_str());
    } else {
        _context.serialForwarder->printf("Error: No scene registered with name '%s'. Use 'list_scenes'.\n", sceneName.c_str());
    }
}


bool SerialCommandHandler::stringToWeatherType(const String& s, WeatherType& outType) {
    if (s.equalsIgnoreCase("none")) { outType = WeatherType::NONE; return true; } 
    if (s.equalsIgnoreCase("sunny")) { outType = WeatherType::SUNNY; return true; } 
    // ... (rest of the comparisons, unchanged) ...
    if (s.equalsIgnoreCase("cloudy")) { outType = WeatherType::CLOUDY; return true; } 
    if (s.equalsIgnoreCase("rainy")) { outType = WeatherType::RAINY; return true; } 
    if (s.equalsIgnoreCase("heavy_rain") || s.equalsIgnoreCase("heavyrain") || s.equalsIgnoreCase("heavy")) { outType = WeatherType::HEAVY_RAIN; return true; } 
    if (s.equalsIgnoreCase("snowy")) { outType = WeatherType::SNOWY; return true; }         
    if (s.equalsIgnoreCase("heavy_snow") || s.equalsIgnoreCase("heavysnow")) { outType = WeatherType::HEAVY_SNOW; return true; } 
    if (s.equalsIgnoreCase("storm")) { outType = WeatherType::STORM; return true; } 
    if (s.equalsIgnoreCase("rainbow")) { outType = WeatherType::RAINBOW; return true; } 
    return false;
}

bool SerialCommandHandler::stringToSickness(const String& s, Sickness& outType) {
    if (s.equalsIgnoreCase("none")) { outType = Sickness::NONE; return true; } 
    if (s.equalsIgnoreCase("cold")) { outType = Sickness::COLD; return true; } 
    // ... (rest of the comparisons, unchanged) ...
    if (s.equalsIgnoreCase("hot")) { outType = Sickness::HOT; return true; } 
    if (s.equalsIgnoreCase("diarrhea")) { outType = Sickness::DIARRHEA; return true; } 
    if (s.equalsIgnoreCase("vomit")) { outType = Sickness::VOMIT; return true; } 
    if (s.equalsIgnoreCase("headache")) { outType = Sickness::HEADACHE; return true; } 
    return false;
}

bool SerialCommandHandler::stringToPrequelStage(const String& s, PrequelStage& outStage) {
    String upperS = s; upperS.toUpperCase();
    if (upperS == "NONE") { outStage = PrequelStage::NONE; return true; }
    // ... (rest of the comparisons, unchanged) ...
    if (upperS == "LANGUAGE_SELECTED" || upperS == "LANG_SEL" || upperS == "LANG") { outStage = PrequelStage::LANGUAGE_SELECTED; return true; }
    if (upperS == "STAGE_1_AWAKENING_COMPLETE" || upperS == "S1_AWAKENING" || upperS == "S1") { outStage = PrequelStage::STAGE_1_AWAKENING_COMPLETE; return true; }
    if (upperS == "STAGE_2_CONGLOMERATE_COMPLETE" || upperS == "S2_CONGLOMERATE" || upperS == "S2") { outStage = PrequelStage::STAGE_2_CONGLOMERATE_COMPLETE; return true; }
    if (upperS == "STAGE_3_JOURNEY_COMPLETE" || upperS == "S3_JOURNEY" || upperS == "S3") { outStage = PrequelStage::STAGE_3_JOURNEY_COMPLETE; return true; }
    if (upperS == "STAGE_4_SHELLWEAVE_COMPLETE" || upperS == "S4_SHELLWEAVE" || upperS == "S4") { outStage = PrequelStage::STAGE_4_SHELLWEAVE_COMPLETE; return true; }
    if (upperS == "PREQUEL_FINISHED" || upperS == "FINISHED" || upperS == "FIN") { outStage = PrequelStage::PREQUEL_FINISHED; return true; }
    char* endptr; long val = strtol(s.c_str(), &endptr, 10);
    if (*endptr == '\0' && endptr != s.c_str()) { 
        if (val >= static_cast<long>(PrequelStage::NONE) && val <= static_cast<long>(PrequelStage::PREQUEL_FINISHED)) {
            outStage = static_cast<PrequelStage>(val); return true;
        }
    }
    return false;
}