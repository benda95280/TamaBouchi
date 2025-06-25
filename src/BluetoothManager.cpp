#include "BluetoothManager.h"
#include <Bluepad32.h>
#include <uni.h>
#include "esp_bt.h"                  
#include "System/GameContext.h"
#include "HardwareInputController.h"
#include "InputManager.h" 

extern Bluepad32 BP32;

BluetoothManager::BluetoothManager()
{
    for (int i = 0; i < BP32_MAX_GAMEPADS; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            _controllerButtonStates[i][j] = {};
        }
    }
}

void BluetoothManager::init(GameContext* context, ControllerPtr* controllers)
{
    _context = context;
    _controllers_ptr = controllers;
    debugPrint("BLUETOOTH", "BluetoothManager Initialized (Ready for Controllers).");
    _isDiscoveryEnabled = false;
    _discoveryStartTime = 0;
    _isStackActive = true; 
    _timeOfLastConnection = millis(); 
}

void BluetoothManager::update(unsigned long currentTime) {
    if (!_isStackActive) return;

    BP32.update();

    bool deviceConnected = false;
    if (_controllers_ptr) {
        for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
            if (_controllers_ptr[i] && _controllers_ptr[i]->isConnected()) {
                deviceConnected = true;
                processController(_controllers_ptr[i]);
            }
        }
    }

    if (_isDiscoveryEnabled && !deviceConnected && (currentTime - _discoveryStartTime > DISCOVERY_TIMEOUT_MS)) {
        debugPrint("BLUETOOTH", "5-minute discovery period ended with no connection. Disabling scanning.");
        enableBluetoothScanning(false);
    }
    
    if (deviceConnected) {
        _timeOfLastConnection = 0; 
    } else {
        if (_timeOfLastConnection == 0) { 
            _timeOfLastConnection = currentTime;
            debugPrint("BLUETOOTH", "No active connection. Starting 10-minute inactivity timer.");
        } else if (currentTime - _timeOfLastConnection > BT_INACTIVITY_TIMEOUT_MS) {
            debugPrint("BLUETOOTH", "10 minutes of inactivity. Disabling Bluetooth stack.");
            fullyDisableStack();
            _timeOfLastConnection = 0; 
        }
    }
}


void BluetoothManager::onConnect(ControllerPtr ctl)
{
    if (!ctl)
        return;
    int idx = ctl->index();
    if (idx >= 0 && idx < BP32_MAX_GAMEPADS)
    {
        debugPrintf("BLUETOOTH", "BluetoothManager: Controller connected (Index %d, Name: %s)\n", idx, ctl->getModelName().c_str());
        for (int j = 0; j < 3; ++j)
        {
            _controllerButtonStates[idx][j] = {};
        }
        debugPrintf("BLUETOOTH", " Reset button state for index %d\n", idx);
        _isDiscoveryEnabled = false; 
        _timeOfLastConnection = 0; 
    }
    else
    {
        debugPrintf("BLUETOOTH", "BluetoothManager Error: Invalid index %d obtained for connected controller.\n", idx);
    }
}

void BluetoothManager::onDisconnect(ControllerPtr ctl)
{
    if (!ctl)
        return;
    int idx = ctl->index();
    debugPrintf("BLUETOOTH", "BluetoothManager: Controller disconnected (Index %d, Name: %s)\n", idx, ctl->getModelName().c_str());
    if (idx >= 0 && idx < BP32_MAX_GAMEPADS)
    {
        for (int j = 0; j < 3; ++j)
        {
            _controllerButtonStates[idx][j] = {};
        }
        debugPrintf("BLUETOOTH", " Reset button state for index %d\n", idx);
    }
    else
    {
        debugPrintf("BLUETOOTH", " Info: Disconnected controller index %d potentially invalid.\n", idx);
    }

    bool anyConnected = false;
    if (_controllers_ptr) {
        for (int i=0; i < BP32_MAX_GAMEPADS; ++i) {
            if (_controllers_ptr[i] && _controllers_ptr[i]->isConnected()) {
                anyConnected = true;
                break;
            }
        }
    }
    if (!anyConnected) {
        _timeOfLastConnection = millis(); // Start inactivity timer
        debugPrint("BLUETOOTH", "Last controller disconnected. Starting inactivity timer.");
    }
}

void BluetoothManager::processController(ControllerPtr ctl)
{
    if (!ctl || !ctl->isConnected())
    {
        return;
    }

    int idx = ctl->index();
    if (idx < 0 || idx >= BP32_MAX_GAMEPADS)
    {
        debugPrintf("BLUETOOTH", " Error: Received data for invalid controller index %d\n", idx);
        return;
    }

    if (ctl->isGamepad())
    {
        uni_hid_device_t *device = uni_hid_device_get_instance_for_idx(idx);
        if (device)
        {
            processGamepad(&device->controller.gamepad, idx);
        }
        else
        {
            debugPrintf("BLUETOOTH", " Error: Could not get uni_hid_device for controller index %d\n", idx);
        }
    }
}

void BluetoothManager::enableBluetoothScanning(bool enable) {
    if (!_isStackActive) {
        debugPrint("BLUETOOTH", "Cannot enable scanning, BT stack is disabled.");
        return;
    }
    if (enable) {
        debugPrint("BLUETOOTH", "Bluetooth discovery ENABLED (5 min timer started).");
        _isDiscoveryEnabled = true;
        _discoveryStartTime = millis();
    } else {
        debugPrint("BLUETOOTH", "Bluetooth discovery DISABLED.");
        _isDiscoveryEnabled = false;
        _discoveryStartTime = 0;
    }
    BP32.enableNewBluetoothConnections(enable);
}

void BluetoothManager::fullyEnableStack() {
    if (_isStackActive) {
        debugPrint("BLUETOOTH", "BT stack is already enabled.");
        return;
    }
    debugPrint("BLUETOOTH", "Enabling Bluetooth stack...");
    if (esp_bt_controller_enable(ESP_BT_MODE_BTDM) == ESP_OK) {
        _isStackActive = true;
        _timeOfLastConnection = millis(); 
    } else {
        debugPrint("BLUETOOTH", "Error: Failed to enable BT controller.");
    }
}

void BluetoothManager::fullyDisableStack() {
    if (!_isStackActive) {
        debugPrint("BLUETOOTH", "BT stack is already disabled.");
        return;
    }
    debugPrint("BLUETOOTH", "Disabling Bluetooth stack...");
    for (int i = 0; i < BP32_MAX_GAMEPADS; ++i) {
        if (_controllers_ptr && _controllers_ptr[i] && _controllers_ptr[i]->isConnected()) {
            _controllers_ptr[i]->disconnect();
        }
    }
    BP32.enableNewBluetoothConnections(false); 
    vTaskDelay(pdMS_TO_TICKS(500)); 
    
    if (esp_bt_controller_disable() == ESP_OK) {
        _isStackActive = false;
        _isDiscoveryEnabled = false;
    } else {
        debugPrint("BLUETOOTH", "Error: Failed to disable BT controller.");
    }
}

bool BluetoothManager::isStackActive() const {
    return _isStackActive;
}

void BluetoothManager::forgetBluetoothKeys()
{
    debugPrint("BLUETOOTH", "Forgetting Bluetooth keys...");
    BP32.forgetBluetoothKeys();
}

void BluetoothManager::processGamepad(const uni_gamepad_t *gp, int controllerIndex)
{
    if (!gp) return;

    unsigned long now = millis();
    processSingleButtonInput(_controllerButtonStates[controllerIndex][0], (gp->misc_buttons & MISC_BUTTON_SYSTEM), EDGE_Button::OK, now);
    processSingleButtonInput(_controllerButtonStates[controllerIndex][1], (gp->dpad & DPAD_LEFT), EDGE_Button::LEFT, now);
    processSingleButtonInput(_controllerButtonStates[controllerIndex][2], (gp->dpad & DPAD_RIGHT), EDGE_Button::RIGHT, now);
}

void BluetoothManager::processSingleButtonInput(ButtonState &buttonState, bool isCurrentlyPressed, EDGE_Button engineButton, unsigned long currentTime)
{
    if (!_context || !_context->inputManager) return;
    InputManager& inputMgr = *_context->inputManager;

    if (isCurrentlyPressed && !buttonState.isPressed)
    {
        buttonState.isPressed = true;
        buttonState.pressStartTime = currentTime;
        buttonState.longPressTriggered = false;
        inputMgr.processButtonEvent(engineButton, EDGE_Event::PRESS);
    }
    else if (!isCurrentlyPressed && buttonState.isPressed)
    {
        buttonState.isPressed = false;
        unsigned long pressDuration = currentTime - buttonState.pressStartTime;
        inputMgr.processButtonEvent(engineButton, EDGE_Event::RELEASE);
        if (!buttonState.longPressTriggered && pressDuration <= CLICK_MAX_DURATION_MS)
        {
            inputMgr.processButtonEvent(engineButton, EDGE_Event::CLICK);
        }
        buttonState.longPressTriggered = false;
    }
    else if (isCurrentlyPressed && buttonState.isPressed && !buttonState.longPressTriggered)
    {
        if (currentTime - buttonState.pressStartTime >= LONG_PRESS_MIN_DURATION_MS)
        {
            buttonState.longPressTriggered = true;
            inputMgr.processButtonEvent(engineButton, EDGE_Event::LONG_PRESS);
        }
    }
}