#include "MenuParametersScene.h"
#include "SceneManager.h"
#include "InputManager.h"
#include "Renderer.h"
#include <memory>
#include "GEM_u8g2.h"
#include "GEMSelect.h"
#include "../../DebugUtils.h"
#include <uni.h>
#include <WiFi.h>
#include <Preferences.h>
#include "WiFiManager.h"
#include "BluetoothManager.h"
#include "GameStats.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <SerialForwarder.h>
#include "Localization.h"
#include "../../System/GameContext.h"
#include "../../GlobalMappings.h"
#include "esp_bt.h" // For esp_bt_controller_get_status

extern QueueHandle_t keyQueue;

MenuParametersScene *MenuParametersScene::currentInstance = nullptr;

void MenuParametersScene::menuBtConnectCallbackWrapper() { if (currentInstance) currentInstance->onBtConnectClick(); }
void MenuParametersScene::menuBtToggleCallbackWrapper() { if (currentInstance) currentInstance->onBtToggleClick(); }
void MenuParametersScene::menuBtStackToggleCallbackWrapper() { if (currentInstance) currentInstance->onBtStackToggleClick(); } 
void MenuParametersScene::menuBtDisconnectCallbackWrapper() { if (currentInstance) currentInstance->onBtDisconnectClick(); }
void MenuParametersScene::menuBtForgetCallbackWrapper() { if (currentInstance) currentInstance->onBtForgetClick(); }
void MenuParametersScene::menuResetTamaCallbackWrapper() { if (currentInstance) currentInstance->onResetTamaClick(); }
void MenuParametersScene::menuResetSettingsCallbackWrapper() { if (currentInstance) currentInstance->onResetSettingsClick(); }
void MenuParametersScene::menuResetAllCallbackWrapper() { if (currentInstance) currentInstance->onResetAllClick(); }
void MenuParametersScene::menuLanguageSelectCallbackWrapper() { if (currentInstance) currentInstance->onLanguageSelectSave(); }
void MenuParametersScene::menuWifiToggleCallbackWrapper() { if (currentInstance) currentInstance->onWifiToggleClick(); }

void MenuParametersScene::onWifiToggleClick() {
    if (_gameContext && _gameContext->wifiManager) {
        bool currentlyEnabled = _gameContext->wifiManager->isWiFiEnabledByUser();
        debugPrintf("SCENES", "WiFi: Toggling state. Currently: %s", currentlyEnabled ? "Enabled" : "Disabled");
        _gameContext->wifiManager->enableWiFi(!currentlyEnabled);
        updateWiFiStatus(); // Update the menu text to reflect the change
    } else {
        debugPrint("SCENES", "Error: WiFi Manager (via context) unavailable in onWifiToggleClick.");
    }
}


void MenuParametersScene::onBtConnectClick()
{
    if (!_gameContext || !_gameContext->bluetoothManager || !_gameContext->bluetoothManager->isStackActive()) {
        debugPrint("SCENES", "Bluetooth: Cannot connect, stack is disabled.");
        return;
    }
    debugPrint("SCENES", "Bluetooth: Enabling scanning...");
    _gameContext->bluetoothManager->enableBluetoothScanning(true);
    updateBluetoothStatus();
}

void MenuParametersScene::onBtToggleClick()
{
    if (!_gameContext || !_gameContext->bluetoothManager || !_gameContext->bluetoothManager->isStackActive()) {
        debugPrint("SCENES", "Bluetooth: Cannot toggle discovery, stack is disabled.");
        return;
    }
    bool currentlyEnabled = uni_bt_enable_new_connections_is_enabled();
    debugPrintf("SCENES", "Bluetooth: Toggling Discovery. Currently: %s", currentlyEnabled ? "Enabled" : "Disabled");
    _gameContext->bluetoothManager->enableBluetoothScanning(!currentlyEnabled);
    updateBluetoothStatus();
}

void MenuParametersScene::onBtStackToggleClick() {
    if (_gameContext && _gameContext->bluetoothManager) {
        if (_gameContext->bluetoothManager->isStackActive()) {
            _gameContext->bluetoothManager->fullyDisableStack();
        } else {
            _gameContext->bluetoothManager->fullyEnableStack();
        }
        updateBluetoothStackStatus(); // Update UI immediately
        updateBluetoothStatus(); // Also update discovery status which depends on stack
    } else {
        debugPrint("SCENES", "Error: BT Manager (via context) unavailable in onBtStackToggleClick.");
    }
}


void MenuParametersScene::onBtDisconnectClick()
{
    if (!_gameContext || !_gameContext->bluetoothManager || !_gameContext->bluetoothManager->isStackActive()) return;
    debugPrint("SCENES", "Bluetooth: Disconnecting first connected gamepad...");
    bool disconnected = false;
    for (int i = 0; i < CONFIG_BLUEPAD32_MAX_DEVICES; ++i)
    {
        uni_hid_device_t *d = uni_hid_device_get_instance_for_idx(i);
        if (d && uni_bt_conn_is_connected(&d->conn))
        {
            uni_bt_disconnect_device_safe(i); // Use the safe version
            snprintf(btConnectedDeviceBuffer, sizeof(btConnectedDeviceBuffer), "None");
            disconnected = true;
            debugPrintf("SCENES", "Disconnected controller at index %d", i);
            break;
        }
    }
    if (!disconnected)
        debugPrint("SCENES", "Bluetooth: No connected gamepad found to disconnect.");
    updateBluetoothStatus();
}

void MenuParametersScene::onBtForgetClick()
{
    if (!_gameContext || !_gameContext->bluetoothManager || !_gameContext->bluetoothManager->isStackActive()) return;
    debugPrint("SCENES", "Bluetooth: Forgetting all paired devices and disconnecting active...");
    
    // First, disconnect any active controllers
    for (int i = 0; i < CONFIG_BLUEPAD32_MAX_DEVICES; ++i)
    {
        uni_hid_device_t *d = uni_hid_device_get_instance_for_idx(i);
        if (d && uni_bt_conn_is_connected(&d->conn))
        {
            debugPrintf("SCENES", "  Forgetting: Disconnecting active controller at index %d (%s)\n", i, d->name);
            uni_bt_disconnect_device_safe(i); // Use the safe version
            delay(50);
        }
    }
    // Then, forget the keys
    _gameContext->bluetoothManager->forgetBluetoothKeys();
    snprintf(btConnectedDeviceBuffer, sizeof(btConnectedDeviceBuffer), "None");
    debugPrint("SCENES", "Bluetooth: All keys forgotten and active connections terminated.");
    
    updateBluetoothStatus();
}
void MenuParametersScene::onResetTamaClick()
{
    debugPrint("SCENES", "Resetting Tama Stats...");
    if (_gameContext && _gameContext->gameStats)
    {
        _gameContext->gameStats->clearPrefs();
        debugPrint("SCENES", "Tama Stats reset and saved to default.");
    }
    else
    {
        debugPrint("SCENES", "Error: GameStats (via context) unavailable in onResetTamaClick.");
    }
}
void MenuParametersScene::onResetSettingsClick()
{
    debugPrint("SCENES", "Resetting WiFi/BT Settings...");
    if (_gameContext && _gameContext->wifiManager)
        _gameContext->wifiManager->clearCredentials();
    else
        debugPrint("SCENES", "Error: WiFiManager (via context) unavailable for clearCreds.");

    if (_gameContext && _gameContext->bluetoothManager)
    {
        // Similar to onBtForgetClick, ensure disconnection before forgetting keys
        for (int i = 0; i < CONFIG_BLUEPAD32_MAX_DEVICES; ++i)
        {
            uni_hid_device_t *d = uni_hid_device_get_instance_for_idx(i);
            if (d && uni_bt_conn_is_connected(&d->conn))
            {
                debugPrintf("SCENES", "  Reset Settings: Disconnecting active controller at index %d (%s)\n", i, d->name);
                uni_bt_disconnect_device_safe(i);
                delay(50);
            }
        }
        _gameContext->bluetoothManager->forgetBluetoothKeys();
    }
    else
    {
        debugPrint("SCENES", "Error: BT Manager (via context) unavailable in onResetSettingsClick.");
    }

    debugPrint("SCENES", "Settings reset. Restart recommended.");
    updateWiFiStatus();
    updateBluetoothStatus();
}
void MenuParametersScene::onResetAllClick()
{
    debugPrint("SCENES", "Resetting ALL Data...");
    onResetTamaClick();
    onResetSettingsClick();
    debugPrint("SCENES", "All data reset. Restart recommended.");
}
void MenuParametersScene::onLanguageSelectSave()
{
    debugPrintf("SCENES", "Language selection saved. New lang enum: %d", (int)languageVariable);
    if (_gameContext && _gameContext->gameStats)
    {
        _gameContext->gameStats->setLanguage(languageVariable);
        _gameContext->gameStats->save();
    }
    else
    {
        debugPrint("SCENES", "Error: GameStats (via context) unavailable in onLanguageSelectSave.");
    }
    if (currentInstance && currentInstance->menu)
    {
        currentInstance->onEnter();
    }
}

SelectOptionByte languageOptions[] = {
    {"English", (byte)Language::ENGLISH},
    {"Francais", (byte)Language::FRENCH}};
const byte languageOptionsCount = sizeof(languageOptions) / sizeof(languageOptions[0]);

MenuParametersScene::MenuParametersScene()
    : menuPageMain(""),
      menuPageBluetooth(""),
      menuPageWifi(""),
      menuPageReset(""),
      menuPageLanguage(""),
      menuItemBluetoothLink("", menuPageBluetooth),
      menuItemWifiLink("", menuPageWifi),
      menuItemResetLink("", menuPageReset),
      menuItemLanguageLink("", menuPageLanguage),
      menuItemBtStackStatus("", btStackStatusBuffer), // NEW
      menuItemBtStackToggle("", menuBtStackToggleCallbackWrapper), // NEW
      menuItemBtStatus("", btStatusBuffer),
      menuItemBtConnectedDevice("", btConnectedDeviceBuffer),
      menuItemBtConnect("", menuBtConnectCallbackWrapper),
      menuItemBtToggle("", menuBtToggleCallbackWrapper),
      menuItemBtDisconnect("", menuBtDisconnectCallbackWrapper),
      menuItemBtForget("", menuBtForgetCallbackWrapper),
      menuItemSsid("", ssidBuffer),
      menuItemPassword("Password", passwordBuffer),
      menuItemWifiStatus("", wifiStatusBuffer),
      menuItemWifiToggle("", menuWifiToggleCallbackWrapper),
      menuItemResetTama("", menuResetTamaCallbackWrapper),
      menuItemResetSettings("", menuResetSettingsCallbackWrapper),
      menuItemResetAll("", menuResetAllCallbackWrapper),
      languageSelect(languageOptionsCount, languageOptions, GEM_LOOP),
      menuItemLanguageSelect("", reinterpret_cast<byte &>(languageVariable), languageSelect, menuLanguageSelectCallbackWrapper)
{
    strncpy(ssidBuffer, "N/A", sizeof(ssidBuffer) - 1);
    ssidBuffer[sizeof(ssidBuffer) - 1] = '\0';
    strncpy(passwordBuffer, "********", sizeof(passwordBuffer) - 1);
    passwordBuffer[sizeof(passwordBuffer) - 1] = '\0';
    strncpy(wifiStatusBuffer, "N/A", sizeof(wifiStatusBuffer) - 1);
    wifiStatusBuffer[sizeof(wifiStatusBuffer) - 1] = '\0';
    strncpy(btStackStatusBuffer, "N/A", sizeof(btStackStatusBuffer) - 1);
    btStackStatusBuffer[sizeof(btStackStatusBuffer) - 1] = '\0';
    strncpy(btStatusBuffer, "N/A", sizeof(btStatusBuffer) - 1);
    btStatusBuffer[sizeof(btStatusBuffer) - 1] = '\0';
    strncpy(btConnectedDeviceBuffer, "None", sizeof(btConnectedDeviceBuffer) - 1);
    btConnectedDeviceBuffer[sizeof(btConnectedDeviceBuffer) - 1] = '\0';
    managesOwnDrawing = true;
}

MenuParametersScene::~MenuParametersScene()
{
    debugPrint("SCENES", "MenuParametersScene destroyed.");
    if (currentInstance == this)
    {
        currentInstance = nullptr;
    }
}

void MenuParametersScene::init(GameContext &context)
{
    debugPrint("SCENES", "MenuParametersScene::init");
    Scene::init();
    _gameContext = &context;
    currentInstance = this;

    if (!_gameContext || !_gameContext->display)
    {
        debugPrint("SCENES", "ERROR: MenuParametersScene::init - Critical context member (display) is null!");
        currentInstance = nullptr;
        return;
    }
    u8g2_ptr = _gameContext->display;

    if (!menu)
    {
        debugPrint("SCENES", "MenuParametersScene: Initializing GEM object.");
        menu.reset(new GEM_u8g2(*u8g2_ptr, GEM_POINTER_ROW, GEM_ITEMS_COUNT_AUTO));
        menuPageMain.addMenuItem(menuItemBluetoothLink);
        menuPageMain.addMenuItem(menuItemWifiLink);
        menuPageMain.addMenuItem(menuItemResetLink);
        menuPageMain.addMenuItem(menuItemLanguageLink);
        
        menuPageWifi.addMenuItem(menuItemWifiToggle);
        menuItemWifiToggle.setReadonly();
        menuPageWifi.addMenuItem(menuItemWifiStatus);
        menuItemWifiStatus.setReadonly();
        menuPageWifi.addMenuItem(menuItemSsid);
        menuItemSsid.setReadonly();
        menuPageWifi.addMenuItem(menuItemPassword);
        menuItemPassword.setReadonly();
        menuPageWifi.setParentMenuPage(menuPageMain);

        menuPageBluetooth.addMenuItem(menuItemBtStackStatus); 
        menuItemBtStackStatus.setReadonly(); 
        menuPageBluetooth.addMenuItem(menuItemBtStackToggle); 
        menuPageBluetooth.addMenuItem(menuItemBtStatus);
        menuItemBtStatus.setReadonly();
        menuPageBluetooth.addMenuItem(menuItemBtConnectedDevice);
        menuItemBtConnectedDevice.setReadonly();
        menuPageBluetooth.addMenuItem(menuItemBtConnect);
        menuPageBluetooth.addMenuItem(menuItemBtToggle);
        menuPageBluetooth.addMenuItem(menuItemBtDisconnect);
        menuPageBluetooth.addMenuItem(menuItemBtForget);
        menuPageBluetooth.setParentMenuPage(menuPageMain);
        
        menuPageReset.addMenuItem(menuItemResetTama);
        menuPageReset.addMenuItem(menuItemResetSettings);
        menuPageReset.addMenuItem(menuItemResetAll);
        menuPageReset.setParentMenuPage(menuPageMain);
        
        menuPageLanguage.addMenuItem(menuItemLanguageSelect);
        menuPageLanguage.setParentMenuPage(menuPageMain);
        
        menu->setMenuPageCurrent(menuPageMain);
        menu->hideVersion();
        menu->setSplashDelay(0);
        menu->init();
    }
    else
    {
        debugPrint("SCENES", "MenuParametersScene: GEM object already initialized.");
        menu->reInit();
    }
    debugPrint("SCENES", "MenuParametersScene::RegisteringButtons");
    _gameContext->inputManager->registerButtonListener(EDGE_Button::LEFT, EDGE_Event::CLICK, this, [this]()
                          { this->onButtonUpClick(); });
    _gameContext->inputManager->registerButtonListener(EDGE_Button::RIGHT, EDGE_Event::CLICK, this, [this]()
                          { this->onButtonDownClick(); });
    _gameContext->inputManager->registerButtonListener(EDGE_Button::OK, EDGE_Event::CLICK, this, [this]()
                          { this->onButtonOkClick(); });
    _gameContext->inputManager->registerButtonListener(EDGE_Button::OK, EDGE_Event::LONG_PRESS, this, [this]()
                          { this->onButtonOkLongPress(); });
    debugPrint("SCENES", "MenuParametersScene Initialized");
}

void MenuParametersScene::onEnter()
{
    debugPrint("SCENES", "MenuParametersScene::onEnter");
    currentInstance = this;
    if (_gameContext && _gameContext->gameStats)
        languageVariable = _gameContext->gameStats->selectedLanguage;
    else
        debugPrint("SCENES", "Error: GameStats (via context) unavailable in onEnter");

    if (menu)
    {
        menuPageMain.setTitle(loc(StringKey::PARAMS_TITLE));
        menuPageBluetooth.setTitle(loc(StringKey::PARAMS_SUB_BLUETOOTH));
        menuPageWifi.setTitle(loc(StringKey::PARAMS_SUB_WIFI));
        menuPageReset.setTitle(loc(StringKey::PARAMS_SUB_RESET));
        menuPageLanguage.setTitle(loc(StringKey::PARAMS_SUB_LANGUAGE));
        menuItemBluetoothLink.setTitle(loc(StringKey::PARAMS_SUB_BLUETOOTH));
        menuItemWifiLink.setTitle(loc(StringKey::PARAMS_SUB_WIFI));
        menuItemResetLink.setTitle(loc(StringKey::PARAMS_SUB_RESET));
        menuItemLanguageLink.setTitle(loc(StringKey::PARAMS_SUB_LANGUAGE));
        menuItemBtStackStatus.setTitle("Stack Status"); 
        menuItemBtStatus.setTitle(loc(StringKey::PARAMS_BT_DISCOVERY));
        menuItemBtConnectedDevice.setTitle(loc(StringKey::PARAMS_BT_DEVICE));
        menuItemBtConnect.setTitle(loc(StringKey::PARAMS_BT_CONNECT_NEW));
        menuItemBtDisconnect.setTitle(loc(StringKey::PARAMS_BT_DISCONNECT));
        menuItemBtForget.setTitle(loc(StringKey::PARAMS_BT_FORGET));
        menuItemSsid.setTitle(loc(StringKey::PARAMS_WIFI_SSID));
        menuItemPassword.setTitle(loc(StringKey::PARAMS_WIFI_PASSWORD));
        menuItemWifiStatus.setTitle(loc(StringKey::PARAMS_WIFI_STATUS));
        menuItemResetTama.setTitle(loc(StringKey::PARAMS_RESET_TAMA));
        menuItemResetSettings.setTitle(loc(StringKey::PARAMS_RESET_SETTINGS));
        menuItemResetAll.setTitle(loc(StringKey::PARAMS_RESET_ALL));
        menuItemLanguageSelect.setTitle(loc(StringKey::PARAMS_LANG_SELECT));

        updateWiFiStatus();
        updateBluetoothStackStatus(); 
        updateBluetoothStatus();
        menu->setMenuPageCurrent(menuPageMain);
        menu->drawMenu();
    }
    else
    {
        debugPrint("SCENES", "MenuParametersScene::onEnter - Error: menu is null!");
    }
}
void MenuParametersScene::onExit() { 
    debugPrint("SCENES", "MenuParametersScene::onExit"); 
    if (_gameContext && _gameContext->inputManager) {
        _gameContext->inputManager->unregisterAllListenersForScene(this);
    }
}

void MenuParametersScene::processKeyPress(uint8_t keyCode)
{
    // This scene doesn't have a custom dialog box, so no handleDialogKeyPress is needed.
    if (menu)
    {
        if (menu->getCurrentMenuPage() == &menuPageWifi) {
            updateWiFiStatus();
        }
        if (menu->getCurrentMenuPage() == &menuPageBluetooth) {
            updateBluetoothStackStatus();
            updateBluetoothStatus();
        }
        if (menu->getCurrentMenuPage() == &menuPageLanguage && _gameContext && _gameContext->gameStats)
            languageVariable = _gameContext->gameStats->selectedLanguage;
        menu->registerKeyPress(keyCode);
    }
    else
    {
        debugPrint("SCENES", "MenuParametersScene::processKeyPress - Error: menu is null!");
    }
}

void MenuParametersScene::onButtonUpClick() {}
void MenuParametersScene::onButtonDownClick() {}
void MenuParametersScene::onButtonOkClick() {}
void MenuParametersScene::onButtonOkLongPress()
{
    if (menu)
    {
        if (menu->isEditMode() || menu->getCurrentMenuPage() != &menuPageMain)
        {
            uint8_t gemKeyCode = GEM_KEY_CANCEL;
            if (keyQueue != NULL)
            {
                xQueueSend(keyQueue, &gemKeyCode, 0);
            }
            else
            {
                debugPrint("SCENES", "Warning: keyQueue is NULL in MenuParametersScene::onButtonOkLongPress");
            }
        }
        else
        {
            debugPrint("SCENES", "MenuParametersScene: Long press on main page - Requesting exit.");
            if (_gameContext && _gameContext->sceneManager)
                _gameContext->sceneManager->requestSetCurrentScene("MAIN");
        }
    }
}
void MenuParametersScene::update(unsigned long deltaTime) {}

void MenuParametersScene::updateWiFiStatus()
{
    if (_gameContext && _gameContext->wifiManager)
    {
        bool enabledByUser = _gameContext->wifiManager->isWiFiEnabledByUser();
        if (enabledByUser) {
            snprintf(wifiToggleBuffer, sizeof(wifiToggleBuffer), "Status: Enabled");
        } else {
            snprintf(wifiToggleBuffer, sizeof(wifiToggleBuffer), "Status: Disabled");
        }
        menuItemWifiToggle.setTitle(wifiToggleBuffer);


        strncpy(ssidBuffer, _gameContext->wifiManager->getSSID().c_str(), sizeof(ssidBuffer) - 1);
        ssidBuffer[sizeof(ssidBuffer) - 1] = '\0';
        wl_status_t status = WiFi.status();
        if (!enabledByUser) {
            strncpy(wifiStatusBuffer, "Disabled", sizeof(wifiStatusBuffer) - 1);
        } else if (status == WL_CONNECTED) {
            snprintf(wifiStatusBuffer, sizeof(wifiStatusBuffer), "Conn: %s", WiFi.localIP().toString().c_str());
        } else if (status == WL_DISCONNECTED || status == WL_IDLE_STATUS) {
            strncpy(wifiStatusBuffer, "Disconnected", sizeof(wifiStatusBuffer) - 1);
        } else if (status == WL_CONNECT_FAILED) {
            strncpy(wifiStatusBuffer, "Connect Fail", sizeof(wifiStatusBuffer) - 1);
        } else if (status == WL_CONNECTION_LOST) {
            strncpy(wifiStatusBuffer, "Conn Lost", sizeof(wifiStatusBuffer) - 1);
        } else if (status == WL_NO_SSID_AVAIL) {
            strncpy(wifiStatusBuffer, "No SSID Avail", sizeof(wifiStatusBuffer) - 1);
        } else {
            strncpy(wifiStatusBuffer, "Connecting...", sizeof(wifiStatusBuffer) - 1);
        }
        wifiStatusBuffer[sizeof(wifiStatusBuffer) - 1] = '\0';
    }
    else
    {
        strncpy(wifiStatusBuffer, "Error", sizeof(wifiStatusBuffer) - 1);
        wifiStatusBuffer[sizeof(wifiStatusBuffer) - 1] = '\0';
        strncpy(wifiToggleBuffer, "WiFi N/A", sizeof(wifiToggleBuffer) - 1);
    }
}

void MenuParametersScene::updateBluetoothStackStatus() {
    if (_gameContext && _gameContext->bluetoothManager) {
        bool isActive = _gameContext->bluetoothManager->isStackActive();
        esp_bt_controller_status_t status = esp_bt_controller_get_status();
        const char* statusStr;
        switch(status) {
            case ESP_BT_CONTROLLER_STATUS_IDLE:     statusStr = "Idle"; break;
            case ESP_BT_CONTROLLER_STATUS_ENABLED:  statusStr = "Enabled"; break;
            case ESP_BT_CONTROLLER_STATUS_INITED:   statusStr = "Inited"; break;
            default:                                statusStr = "Unknown"; break;
        }
        snprintf(btStackStatusBuffer, sizeof(btStackStatusBuffer), "Stack: %s", statusStr);
        menuItemBtStackToggle.setTitle(isActive ? "Disable Stack" : "Enable Stack");
    } else {
        strncpy(btStackStatusBuffer, "Stack: N/A", sizeof(btStackStatusBuffer) - 1);
        menuItemBtStackToggle.setTitle("N/A");
    }
}


void MenuParametersScene::updateBluetoothStatus()
{
    if (!_gameContext || !_gameContext->bluetoothManager) {
        strncpy(btStatusBuffer, "N/A", sizeof(btStatusBuffer)-1);
        menuItemBtToggle.setTitle("N/A");
        strncpy(btConnectedDeviceBuffer, "N/A", sizeof(btConnectedDeviceBuffer)-1);
        return;
    }
    
    bool stackActive = _gameContext->bluetoothManager->isStackActive();

    if (!stackActive) {
        menuItemBtStatus.setTitle("(Disabled)");
        menuItemBtToggle.setTitle("(Disabled)");
        menuItemBtConnect.setTitle("(Disabled)");
        menuItemBtDisconnect.setTitle("(Disabled)");
        menuItemBtForget.setTitle("(Disabled)");
        strncpy(btConnectedDeviceBuffer, "None", sizeof(btConnectedDeviceBuffer) - 1);
        return;
    }
    
    menuItemBtStatus.setTitle(loc(StringKey::PARAMS_BT_DISCOVERY));
    menuItemBtConnect.setTitle(loc(StringKey::PARAMS_BT_CONNECT_NEW));
    menuItemBtDisconnect.setTitle(loc(StringKey::PARAMS_BT_DISCONNECT));
    menuItemBtForget.setTitle(loc(StringKey::PARAMS_BT_FORGET));

    bool scanning = uni_bt_enable_new_connections_is_enabled();
    strncpy(btStatusBuffer, scanning ? "Scanning (5min)" : "Disabled", sizeof(btStatusBuffer) - 1);
    btStatusBuffer[sizeof(btStatusBuffer) - 1] = '\0';

    if (scanning) {
        menuItemBtToggle.setTitle("Disable Discovery");
    } else {
        menuItemBtToggle.setTitle("Enable Discovery");
    }

    bool deviceConnected = false;
    for (int i = 0; i < CONFIG_BLUEPAD32_MAX_DEVICES; ++i)
    {
        uni_hid_device_t *d = uni_hid_device_get_instance_for_idx(i);
        if (d && uni_bt_conn_is_connected(&d->conn))
        {
            snprintf(btConnectedDeviceBuffer, sizeof(btConnectedDeviceBuffer), "%s", d->name);
            deviceConnected = true;
            break;
        }
    }
    if (!deviceConnected)
    {
        strncpy(btConnectedDeviceBuffer, "None", sizeof(btConnectedDeviceBuffer) - 1);
    }
    btConnectedDeviceBuffer[sizeof(btConnectedDeviceBuffer) - 1] = '\0';
}
void MenuParametersScene::draw(Renderer &renderer)
{
    if (menu)
        menu->drawMenu();
}