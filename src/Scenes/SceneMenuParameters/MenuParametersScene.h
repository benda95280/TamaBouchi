#pragma once

#include "Scene.h"
#include "GEM_u8g2.h"
#include "GEMSelect.h" // <-- Needed for GEMSelect
#include "espasyncbutton.hpp"
#include <memory>
#include <functional>
#include "Localization.h" // <-- Include Localization
#include "../../System/GameContext.h" 

// Forward declaration
class U8G2;
class Preferences;
class BluetoothManager;

class MenuParametersScene : public Scene {
public:
    MenuParametersScene();
    ~MenuParametersScene() override;

    // This is a scene-specific init, not an override of the base class.
    void init(GameContext &context);
    void onEnter() override;
    void onExit() override;
    void update(unsigned long deltaTime) override;
    void draw(Renderer& renderer) override;

    bool usesKeyQueue() const override { return true; }
    void processKeyPress(uint8_t keyCode) override;

    std::unique_ptr<GEM_u8g2> menu;

private:
    // This scene needs its own context pointer, set during init.
    GameContext* _gameContext = nullptr;

    // Removed direct manager pointers, as they are accessed via _gameContext
    // Preferences* _prefs = nullptr;
    // BluetoothManager* _btManager = nullptr;

    // Button Callbacks
    void onButtonUpClick();
    void onButtonDownClick();
    void onButtonOkClick();
    void onButtonOkLongPress();

    // Action Callbacks
    void onBtConnectClick();
    void onBtToggleClick();
    void onBtStackToggleClick(); 
    void onBtDisconnectClick();
    void onBtForgetClick();
    void onResetTamaClick();
    void onResetSettingsClick();
    void onResetAllClick();
    void onLanguageSelectSave();
    void onWifiToggleClick();

    // Static Callback Wrappers
    static void menuBtConnectCallbackWrapper();
    static void menuBtToggleCallbackWrapper();
    static void menuBtStackToggleCallbackWrapper(); 
    static void menuBtDisconnectCallbackWrapper();
    static void menuBtForgetCallbackWrapper();
    static void menuResetTamaCallbackWrapper();
    static void menuResetSettingsCallbackWrapper();
    static void menuResetAllCallbackWrapper();
    static void menuLanguageSelectCallbackWrapper();
    static void menuWifiToggleCallbackWrapper();

    // Status Update Helpers
    void updateWiFiStatus();
    void updateBluetoothStatus();
    void updateBluetoothStackStatus(); 

    U8G2* u8g2_ptr = nullptr;

    // --- Menu Pages ---
    GEMPage menuPageMain;
    GEMPage menuPageWifi;
    GEMPage menuPageBluetooth;
    GEMPage menuPageReset;
    GEMPage menuPageLanguage;

    // --- WiFi Submenu ---
    GEMItem menuItemWifiLink;
    char ssidBuffer[33];
    char passwordBuffer[16];
    char wifiStatusBuffer[40];
    char wifiToggleBuffer[20];
    GEMItem menuItemSsid;
    GEMItem menuItemPassword;
    GEMItem menuItemWifiStatus;
    GEMItem menuItemWifiToggle;

    // --- Bluetooth Submenu ---
    GEMItem menuItemBluetoothLink;
    char btStackStatusBuffer[24]; 
    char btStatusBuffer[20];
    char btConnectedDeviceBuffer[64];
    GEMItem menuItemBtStackStatus; 
    GEMItem menuItemBtStackToggle; 
    GEMItem menuItemBtStatus;
    GEMItem menuItemBtConnectedDevice;
    GEMItem menuItemBtConnect;
    GEMItem menuItemBtToggle;
    GEMItem menuItemBtDisconnect;
    GEMItem menuItemBtForget;

    // --- Reset Submenu ---
    GEMItem menuItemResetLink;
    GEMItem menuItemResetTama;
    GEMItem menuItemResetSettings;
    GEMItem menuItemResetAll;

    // --- Language Submenu ---
    GEMItem menuItemLanguageLink;
    GEMSelect languageSelect;
    Language languageVariable;
    GEMItem menuItemLanguageSelect;

    static MenuParametersScene* currentInstance;
};