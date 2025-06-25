#ifndef HARDWARE_INPUT_CONTROLLER_H
#define HARDWARE_INPUT_CONTROLLER_H

#include <Arduino.h>
#include <esp_event.h>    
#include <Bluepad32.h>    
#include "DebugUtils.h"   
#include "InputManager.h" // For EDGE_Button, EDGE_Event

// Forward declarations
struct GameContext; 
class BluetoothManager; // Forward-declare BluetoothManager

#ifndef BP32_MAX_GAMEPADS 
#define BP32_MAX_GAMEPADS 4
#endif

// Define a function pointer type for the update activity callback
using UpdateActivityCallback = void (*)();

class HardwareInputController {
public:
    HardwareInputController();

    void init(
        GameContext* context, 
        BluetoothManager* btManager,
        ControllerPtr* controllersArray, 
        UpdateActivityCallback activityCallback 
    );

    static void handleButtonEvent_static(void* handler_args, esp_event_base_t base, int32_t id, void* event_data);
    static void onConnectedController_static(ControllerPtr ctl);
    static void onDisconnectedController_static(ControllerPtr ctl);

private:
    // Declare the static member pointers inside the class
    static GameContext* _s_gameContext_ptr; 
    static BluetoothManager* _s_bluetoothManager_ptr; 
    static ControllerPtr* _s_controllers_array_ptr; 
    static UpdateActivityCallback _s_update_activity_callback; 

};

#endif // HARDWARE_INPUT_CONTROLLER_H