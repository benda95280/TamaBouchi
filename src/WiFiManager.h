#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <esp_wifi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <U8g2lib.h> // For drawing OTA status
#include <Preferences.h> // Include Preferences
#include "DebugUtils.h"   // Using debug system instead of SerialForwarder
#include <WiFi.h>
#include <ArduinoOTA.h>

// Forward declarations
class Print;
class GameStats; // Forward declare GameStats

// Define keys for WiFi Preferences
#define PREF_WIFI_NAMESPACE "WiFiConfig"
#define PREF_KEY_WIFI_SSID "ssid"
#define PREF_KEY_WIFI_PWD "password"
#define PREF_KEY_WIFI_ENABLED "wifi_enabled" // New key for user preference

class WiFiManager {
public:
    WiFiManager(Preferences& preferences);
    void init(const char* default_ssid, const char* default_password, AsyncWebServer* server, U8G2* u8g2, const char* ota_password, GameStats* gameStats);
    bool isOTAInProgress() const;
    void saveCredentials(const String& ssid, const String& password);
    void clearCredentials();
    String getSSID() const { return _loaded_ssid; }
    String getPassword() const { return _loaded_password; }

    void enableWiFi(bool enable);
    bool isWiFiEnabledByUser() const { return _wifiEnabledByUser; }
    void handleOTA();
    bool isRebootRequired() const { return _rebootOnUpdate; }


private:
    Preferences& _prefs; // Store reference to Preferences object
    GameStats* _gameStats = nullptr; // Member to hold the GameStats pointer

    String _loaded_ssid = "";
    String _loaded_password = "";
    const char* _default_ssid = nullptr;
    const char* _default_password = nullptr;
    const char* _ota_password = nullptr;
    bool _wifiEnabledByUser = true;
    bool _rebootOnUpdate = false;

    AsyncWebServer* _server = nullptr;
    U8G2* _u8g2 = nullptr;

    bool _otaInProgress = false;
    uint8_t _otaProgress = 0;
    unsigned long _ota_progress_millis = 0;

    static WiFiManager* _instance;

    static void WiFiGotIPWrapper(WiFiEvent_t event, WiFiEventInfo_t info);
    static void WiFiEventWrapper(WiFiEvent_t event, WiFiEventInfo_t info);
    static void onOTAStartWrapper_static();
    static void onOTAEndWrapper_static();
    static void onOTAProgressWrapper_static(unsigned int progress, unsigned int total);
    static void onOTAErrorWrapper_static(ota_error_t error);

    void handleWiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info);
    void handleWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info);
    void drawUpdateScreen(const char* message, int progress);
    void loadCredentials();
    void startConnection();
    void beginArduinoOTA();

    void handleUpdateStart();
    void handleUpdateEnd(bool success);
    void handleUpdateProgress(uint32_t current, uint32_t final);
    void handleUpdateError(String errorMsg);
};

#endif // WIFI_MANAGER_H