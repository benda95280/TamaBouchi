#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <esp_wifi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <PrettyOTA.h>
#include <U8g2lib.h> // For drawing OTA status
#include <Preferences.h> // Include Preferences
#include "DebugUtils.h"   // Using debug system instead of SerialForwarder

// Forward declarations
class Print;

// Define keys for WiFi Preferences
#define PREF_WIFI_NAMESPACE "WiFiConfig"
#define PREF_KEY_WIFI_SSID "ssid"
#define PREF_KEY_WIFI_PWD "password"
#define PREF_KEY_WIFI_ENABLED "wifi_enabled" // New key for user preference

class WiFiManager {
public:
    WiFiManager(Preferences& preferences); // Add Preferences reference
    void init(const char* default_ssid, const char* default_password, AsyncWebServer* server, PrettyOTA* ota, U8G2* u8g2);
    bool isOTAInProgress() const;
    void saveCredentials(const String& ssid, const String& password); // Method to save credentials
    void clearCredentials(); // Method to clear credentials
    String getSSID() const { return _loaded_ssid; } // Getter for loaded SSID
    String getPassword() const { return _loaded_password; } // Getter for loaded password (masked?)

    // New public methods to control WiFi state
    void enableWiFi(bool enable);
    bool isWiFiEnabledByUser() const { return _wifiEnabledByUser; }


private:
    Preferences& _prefs; // Store reference to Preferences object

    String _loaded_ssid = "";        // Store loaded SSID
    String _loaded_password = "";    // Store loaded Password
    const char* _default_ssid = nullptr; // Store default SSID
    const char* _default_password = nullptr; // Store default Password
    bool _wifiEnabledByUser = true; // Store user's preference

    AsyncWebServer* _server = nullptr;
    PrettyOTA* _ota = nullptr;
    U8G2* _u8g2 = nullptr; // Pointer to the U8G2 instance for drawing updates

    bool _otaInProgress = false;
    uint8_t _otaProgress = 0;
    unsigned long _ota_progress_millis = 0;

    // Static members for callbacks because C++ doesn't easily allow member functions as C-style callbacks
    static WiFiManager* _instance; // Static pointer to the single instance

    // Static callback wrappers
    static void WiFiGotIPWrapper(WiFiEvent_t event, WiFiEventInfo_t info);
    static void WiFiEventWrapper(WiFiEvent_t event);
    static void drawUpdateScreenWrapper(const char* message, int progress);
    static void onOTAStartWrapper(NSPrettyOTA::UPDATE_MODE updateMode);
    static void onOTAEndWrapper(bool success);
    static void onOTAProgressWrapper(uint32_t current, uint32_t final);

    // Member function implementations of callbacks
    void handleWiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info);
    void handleWiFiEvent(WiFiEvent_t event);
    void drawUpdateScreen(const char* message, int progress);
    void handleOTAStart(NSPrettyOTA::UPDATE_MODE updateMode);
    void handleOTAEnd(bool success);
    void handleOTAProgress(uint32_t current, uint32_t final);
    void loadCredentials(); // Private method to load credentials
    void startConnection(); // Private method to initiate WiFi connection
};

#endif // WIFI_MANAGER_H