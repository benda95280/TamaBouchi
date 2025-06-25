#ifndef BLUETOOTH_MANAGER_H
#define BLUETOOTH_MANAGER_H

#include <Arduino.h>
#include <Bluepad32.h>
#include <uni.h>
#include "esp_event.h"        
#include "espasyncbutton.hpp" 
#include "DebugUtils.h"
#include "System/GameContext.h" 
#include "InputManager.h" // For EDGE_Button, EDGE_Event

// Forward declarations

// Structure to track button state for press/long press/click detection
struct ButtonState
{
    bool isPressed = false;
    unsigned long pressStartTime = 0;
    bool longPressTriggered = false;
};

class BluetoothManager
{
public:
    BluetoothManager();
    void init(GameContext *context = nullptr, ControllerPtr* controllers = nullptr); 
    void update(unsigned long currentTime); 

    void onConnect(ControllerPtr ctl);
    void onDisconnect(ControllerPtr ctl);
    void processController(ControllerPtr ctl);

    void enableBluetoothScanning(bool enable);
    void forgetBluetoothKeys();

    void fullyEnableStack();
    void fullyDisableStack();
    bool isStackActive() const;

private:
    GameContext *_context = nullptr; 
    ControllerPtr* _controllers_ptr = nullptr; 

    ButtonState _controllerButtonStates[BP32_MAX_GAMEPADS][3];
    bool _isDiscoveryEnabled = false;
    unsigned long _discoveryStartTime = 0;
    bool _isStackActive = true; 
    unsigned long _timeOfLastConnection = 0;

    static const unsigned long DISCOVERY_TIMEOUT_MS = 5 * 60 * 1000; // 5 minutes
    static const unsigned long BT_INACTIVITY_TIMEOUT_MS = 10 * 60 * 1000; // 10 minutes
    static const unsigned long CLICK_MAX_DURATION_MS = 300;
    static const unsigned long LONG_PRESS_MIN_DURATION_MS = 750;

    void processGamepad(const uni_gamepad_t *gp, int controllerIndex);
    // Method signature updated to use EDGE_Button
    void processSingleButtonInput(ButtonState &buttonState, bool isCurrentlyPressed, EDGE_Button engineButton, unsigned long currentTime);
};

#endif // BLUETOOTH_MANAGER_H