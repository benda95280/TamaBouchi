#include "WiFiManager.h"
#include "SerialForwarder.h"
#include "DebugUtils.h" // Ensure DebugUtils is included for debugPrint/debugPrintf

// Initialize static member
WiFiManager* WiFiManager::_instance = nullptr;

WiFiManager::WiFiManager(Preferences& preferences) :
    _prefs(preferences) // Initialize preferences reference
{
    _instance = this; // Set the static instance pointer
}

void WiFiManager::loadCredentials() {
    if (_prefs.begin(PREF_WIFI_NAMESPACE, true)) { // read-only = true
        _wifiEnabledByUser = _prefs.getBool(PREF_KEY_WIFI_ENABLED, true); // Default to enabled
        _loaded_ssid = _prefs.getString(PREF_KEY_WIFI_SSID, "");
        _loaded_password = _prefs.getString(PREF_KEY_WIFI_PWD, "");
        _prefs.end();
        if (_loaded_ssid.length() > 0) {
            debugPrint("WIFI_MANAGER","WiFi credentials loaded from Preferences.");
        } else {
            debugPrint("WIFI_MANAGER","No WiFi credentials found in Preferences. Using defaults.");
        }
        debugPrintf("WIFI_MANAGER", "User WiFi preference is: %s", _wifiEnabledByUser ? "ENABLED" : "DISABLED");
    } else {
        debugPrint("WIFI_MANAGER","Failed to open WiFi preferences for reading. Using defaults.");
    }

    // Use defaults if nothing was loaded
    if (_loaded_ssid.length() == 0) {
        _loaded_ssid = _default_ssid ? _default_ssid : "";
    }
    if (_loaded_password.length() == 0) {
         _loaded_password = _default_password ? _default_password : "";
    }
}

void WiFiManager::enableWiFi(bool enable) {
    _wifiEnabledByUser = enable;
    if (_prefs.begin(PREF_WIFI_NAMESPACE, false)) {
        _prefs.putBool(PREF_KEY_WIFI_ENABLED, _wifiEnabledByUser);
        _prefs.end();
        debugPrintf("WIFI_MANAGER", "Set WiFi preference to %s and saved.", enable ? "ENABLED" : "DISABLED");
    } else {
        debugPrintf("WIFI_MANAGER", "Failed to save WiFi preference to %s.", enable ? "ENABLED" : "DISABLED");
    }

    if (enable) {
        startConnection();
    } else {
        WiFi.disconnect(true);
        debugPrint("WIFI_MANAGER", "WiFi disconnected by user setting.");
    }
}


void WiFiManager::saveCredentials(const String& ssid, const String& password) {
    if (_prefs.begin(PREF_WIFI_NAMESPACE, false)) { // read-only = false
        _prefs.putString(PREF_KEY_WIFI_SSID, ssid);
        _prefs.putString(PREF_KEY_WIFI_PWD, password);
        _prefs.end();
        debugPrint("WIFI_MANAGER","WiFi credentials saved to Preferences.");
        _loaded_ssid = ssid; // Update loaded credentials
        _loaded_password = password;
        // Saving credentials implies the user wants WiFi on.
        enableWiFi(true);
    } else {
        debugPrint("WIFI_MANAGER","Failed to open WiFi preferences for writing.");
    }
}

void WiFiManager::clearCredentials() {
    if (_prefs.begin(PREF_WIFI_NAMESPACE, false)) { // read-only = false
        _prefs.remove(PREF_KEY_WIFI_SSID);
        _prefs.remove(PREF_KEY_WIFI_PWD);
        // _prefs.clear(); // Alternative: clear the whole namespace
        _prefs.end();
        debugPrint("WIFI_MANAGER","WiFi credentials cleared from Preferences.");
        _loaded_ssid = _default_ssid ? _default_ssid : ""; // Revert to defaults
        _loaded_password = _default_password ? _default_password : "";
        // Clearing credentials also implies disabling WiFi until it's re-configured.
        enableWiFi(false);
    } else {
        debugPrint("WIFI_MANAGER","Failed to open WiFi preferences for clearing.");
    }
}

void WiFiManager::startConnection() {
    if (!_wifiEnabledByUser) {
        debugPrint("WIFI_MANAGER","Cannot start WiFi connection: Disabled by user preference.");
        return;
    }
    if (_loaded_ssid.length() == 0) {
        debugPrint("WIFI_MANAGER","Cannot start WiFi connection: SSID is empty.");
        // Disable for next boot since it can't work anyway
        if (_wifiEnabledByUser) {
            enableWiFi(false);
        }
        return;
    }
    debugPrintf("WIFI_MANAGER","Connecting to WiFi SSID: %s\n", _loaded_ssid.c_str());
    WiFi.begin(_loaded_ssid.c_str(), _loaded_password.c_str());
}


void WiFiManager::init(const char* default_ssid, const char* default_password, AsyncWebServer* server, PrettyOTA* ota, U8G2* u8g2) {

    _default_ssid = default_ssid;
    _default_password = default_password;
    _server = server;
    _ota = ota;
    _u8g2 = u8g2;

    if (!_server || !_ota || !_u8g2) {
        debugPrint("WIFI_MANAGER","ERROR: WiFiManager requires valid server, OTA, and U8G2 pointers!");
        return;
    }

    debugPrint("WIFI_MANAGER","Initializing WiFiManager...");

    loadCredentials(); // Load credentials AND user's enabled preference

    WiFi.disconnect(true);
    WiFi.mode(WIFI_STA);
    esp_wifi_set_protocol(WIFI_IF_AP, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G);

    // Use static wrappers for WiFi events
    WiFi.onEvent(WiFiGotIPWrapper, ARDUINO_EVENT_WIFI_STA_GOT_IP);
    WiFi.onEvent(WiFiEventWrapper); // Generic event handler

    // Start connection only if conditions are met
    startConnection();

    // Configure OTA server routes and callbacks
    _server->on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "Hi! This is OTA AsyncDemo for Flappy Bird.");
    });

    _ota->Begin(_server, "", ""); // Use the passed server pointer
    // Use static wrappers for OTA callbacks
    _ota->OnStart(onOTAStartWrapper);
    _ota->OnProgress(onOTAProgressWrapper);
    _ota->OnEnd(onOTAEndWrapper);

    // Set unique Hardware-ID for your hardware/board
    _ota->SetHardwareID("UniqueBoard1");

    // Set current build time and date
    PRETTY_OTA_SET_CURRENT_BUILD_TIME_AND_DATE();


    // Server is already begun in Main.ino, no need to begin it here again

    debugPrint("WIFI_MANAGER","WiFiManager Initialized.");
}

bool WiFiManager::isOTAInProgress() const {
    return _otaInProgress;
}

// --- Static Callback Wrappers ---
// (Keep these as they are)
void WiFiManager::WiFiGotIPWrapper(WiFiEvent_t event, WiFiEventInfo_t info) {
    if (_instance) {
        _instance->handleWiFiGotIP(event, info);
    }
}

void WiFiManager::WiFiEventWrapper(WiFiEvent_t event) {
    if (_instance) {
        _instance->handleWiFiEvent(event);
    }
}

void WiFiManager::onOTAStartWrapper(NSPrettyOTA::UPDATE_MODE updateMode) {
    if (_instance) {
        _instance->handleOTAStart(updateMode);
    }
}

void WiFiManager::onOTAEndWrapper(bool success) {
    if (_instance) {
        _instance->handleOTAEnd(success);
    }
}

void WiFiManager::onOTAProgressWrapper(uint32_t current, uint32_t final) {
    if (_instance) {
        _instance->handleOTAProgress(current, final);
    }
}
// --- End Static Callback Wrappers ---


// --- Member Function Callback Implementations ---
void WiFiManager::handleWiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info) {
    debugPrint("WIFI_MANAGER","WiFi connected");
    debugPrint("WIFI_MANAGER","IP address: ");
    debugPrint("WIFI_MANAGER",IPAddress(info.got_ip.ip_info.ip.addr).toString().c_str());
    debugPrint("WIFI_MANAGER","RRSI: ");
    debugPrint("WIFI_MANAGER",String(WiFi.RSSI()).c_str());
}

// WARNING: This function is called from a separate FreeRTOS task (thread)!
void WiFiManager::handleWiFiEvent(WiFiEvent_t event) {
     debugPrintf("WIFI_MANAGER","[WiFi-event] event: %d\n", event);
     // Add specific handling if needed, like GOT_IP which is handled separately
     switch (event) {
        case SYSTEM_EVENT_STA_DISCONNECTED:
            debugPrint("WIFI_MANAGER","WiFi Disconnected. Trying to reconnect...");
            // Don't call WiFi.reconnect() here directly, it can cause issues.
            // Let the main loop or a dedicated task handle reconnect logic if needed.
            // For now, it will likely just sit disconnected until explicitly told to connect again.
            break;
        case SYSTEM_EVENT_STA_START:
             debugPrint("WIFI_MANAGER","WiFi Station Started.");
             break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
             // Already handled by specific callback, but log here if desired
             debugPrint("WIFI_MANAGER","Obtained IP address via generic handler: ");
             debugPrint("WIFI_MANAGER",String(WiFi.localIP()).c_str());
             break;
       default: break;
     }
}

void WiFiManager::drawUpdateScreen(const char* message, int progress) {
    if (!_u8g2) return; 
    _u8g2->setFont(u8g2_font_6x12_tf); // Set font once, outside the loop

    _u8g2->firstPage();
    do {
      // Display SSID
      _u8g2->setCursor(0, 12); 
      _u8g2->print("SSID: ");   
      _u8g2->print(_loaded_ssid.c_str()); 

      // Display IP address
      _u8g2->setCursor(0, 24); 
      _u8g2->print("Ip: ");     
      if (WiFi.status() == WL_CONNECTED) {
        WiFi.localIP().printTo(*_u8g2); // Efficiently print IP
      } else {
        _u8g2->print("Connecting...");
      }

      // Display message
      _u8g2->setCursor(0, 36); 
      _u8g2->print(message);

      // Display progress percentage
      char progressStr[20]; 
      snprintf(progressStr, sizeof(progressStr), "Progress: %d%%", progress);
      _u8g2->setCursor(0, 48); 
      _u8g2->print(progressStr);

      // Draw progress bar
      int barWidth = map(progress, 0, 100, 0, 120); 
      _u8g2->drawFrame(4, 55, 120, 8); 
      _u8g2->drawBox(4, 55, barWidth, 8); 
    } while (_u8g2->nextPage());
    // delay(10); // Removed delay as it's generally not good in callbacks from other tasks
}

void WiFiManager::handleOTAStart(NSPrettyOTA::UPDATE_MODE updateMode) {
    Serial.println("OTA update started!");
    _otaInProgress = true;
    _otaProgress = 0;
    drawUpdateScreen("Update Started", _otaProgress);
}

void WiFiManager::handleOTAEnd(bool success) {
    if (success) {
      Serial.println("OTA update finished successfully!");
      drawUpdateScreen("Update finished", 100);
      delay(1000); 
      ESP.restart(); 
    } else {
      Serial.println("There was an error during OTA update!");
      drawUpdateScreen("Update error", 100);
      delay(2000); 
    }
     _otaInProgress = false; 
}

void WiFiManager::handleOTAProgress(uint32_t current, uint32_t final) {
    uint8_t newProgress = (current * 100) / final;
    // Update screen only if progress percentage changes OR if it's the final update OR if 1 second passed
    if (newProgress > _otaProgress || current == final || (millis() - _ota_progress_millis > 1000)) {
        _ota_progress_millis = millis();
        _otaProgress = newProgress;
        Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes (%d%%)\n", current, final, _otaProgress);
        drawUpdateScreen("Updating...", _otaProgress);
    }
}