#include "HardwareInputController.h"
#include "InputManager.h"
#include "BluetoothManager.h"
#include "SerialForwarder.h"
#include "espasyncbutton.hpp" // Keep this here, as this is the adapter for this library
#include "System/GameContext.h" 
#include "GlobalMappings.h"
#include <map>


GameContext* HardwareInputController::_s_gameContext_ptr = nullptr;
BluetoothManager* HardwareInputController::_s_bluetoothManager_ptr = nullptr;
ControllerPtr* HardwareInputController::_s_controllers_array_ptr = nullptr;
UpdateActivityCallback HardwareInputController::_s_update_activity_callback = nullptr; 

HardwareInputController::HardwareInputController() {
}

void HardwareInputController::init(
    GameContext* context,
    BluetoothManager* btManager,
    ControllerPtr* controllersArray, 
    UpdateActivityCallback activityCallback 
    ) {
    _s_gameContext_ptr = context;
    _s_bluetoothManager_ptr = btManager;
    _s_controllers_array_ptr = controllersArray;
    _s_update_activity_callback = activityCallback; 

    if (_s_bluetoothManager_ptr) {
        _s_bluetoothManager_ptr->init(context, controllersArray);
    }

    debugPrint("HARDWARE_INPUT","HardwareInputController Initialized with activity callback.");
}


void HardwareInputController::handleButtonEvent_static(void* handler_args, esp_event_base_t base, int32_t id, void* event_data) {
    static const std::map<int, EDGE_Button> buttonMapping = {
        {PHYSICAL_UP_BUTTON_PIN, EDGE_Button::UP},
        {PHYSICAL_DOWN_BUTTON_PIN, EDGE_Button::DOWN},
        {VIRTUAL_BTN_OK, EDGE_Button::OK},
        {VIRTUAL_BTN_LEFT, EDGE_Button::LEFT},
        {VIRTUAL_BTN_RIGHT, EDGE_Button::RIGHT}
    };
    EventMsg* temp_msg = reinterpret_cast<EventMsg*>(event_data);
    debugPrintf("HARDWARE_INPUT","HIC::handleButtonEvent_static: Received Event! Base='%s', ID(EventType)=%d, GPIO(Pin)=%d\n",
                               base, (int)id, (int)temp_msg->gpio);

    if (_s_gameContext_ptr && _s_gameContext_ptr->inputManager && base == EBTN_EVENTS) {
        InputManager& inputMgr = *(_s_gameContext_ptr->inputManager);
        EventMsg* msg = reinterpret_cast<EventMsg*>(event_data);
        ESPButton::event_t specificEventType = ESPButton::int2event_t(id);
        
        auto it = buttonMapping.find(msg->gpio);
        if (it == buttonMapping.end()) {
            debugPrintf("HARDWARE_INPUT", "HIC Warning: Unmapped GPIO pin %d", msg->gpio);
            return;
        }
        EDGE_Button engineButton = it->second;
        
        EDGE_Event engineEvent;
        bool eventMapped = true;
        switch(specificEventType) {
            case ESPButton::event_t::press: engineEvent = EDGE_Event::PRESS; break;
            case ESPButton::event_t::release: engineEvent = EDGE_Event::RELEASE; break;
            case ESPButton::event_t::click: engineEvent = EDGE_Event::CLICK; break;
            case ESPButton::event_t::longPress: engineEvent = EDGE_Event::LONG_PRESS; break;
            default: eventMapped = false; break;
        }
        
        if (eventMapped) {
            inputMgr.processButtonEvent(engineButton, engineEvent);
        } else {
            debugPrintf("HARDWARE_INPUT", "HIC Warning: Unhandled event type: %d", (int)specificEventType);
        }
        
        if (_s_update_activity_callback) {
            _s_update_activity_callback();
        }

    } else if (!_s_gameContext_ptr || !_s_gameContext_ptr->inputManager) {
        debugPrint("HARDWARE_INPUT","HIC Warning: handleButtonEvent_static called before GameContext or InputManager ptr set.");
    } else if (base != EBTN_EVENTS) {
        debugPrintf("HARDWARE_INPUT","HIC Warning: handleButtonEvent_static received event from unexpected base: %s\n", base);
    }
}

void HardwareInputController::onConnectedController_static(ControllerPtr ctl) {
    if (!ctl) return;
    int idx = ctl->index();
    debugPrintf("HARDWARE_INPUT","HIC: Controller Connected (Idx %d): %s\n", idx, ctl->getModelName().c_str());

    if (!_s_controllers_array_ptr) {
        debugPrint("HARDWARE_INPUT","HIC Error: Controllers array pointer is null!");
        return;
    }

    bool foundSlot = false;
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (_s_controllers_array_ptr[i] == nullptr) {
            _s_controllers_array_ptr[i] = ctl;
            foundSlot = true;
            if (_s_gameContext_ptr && _s_gameContext_ptr->bluetoothManager) {
                 _s_gameContext_ptr->bluetoothManager->onConnect(ctl);
            } else if (_s_bluetoothManager_ptr) { 
                 _s_bluetoothManager_ptr->onConnect(ctl);
            }
            else debugPrint("HARDWARE_INPUT","HIC Warning: BluetoothManager unavailable in onConnectedController_static");
            break;
        }
    }
    if (!foundSlot) {
        debugPrint("HARDWARE_INPUT","HIC Error: No empty slot for controller!");
    }
    
    if (_s_update_activity_callback) {
        _s_update_activity_callback();
    }
}

void HardwareInputController::onDisconnectedController_static(ControllerPtr ctl) {
    if (!ctl) return;
    int idx = ctl->index();
    debugPrintf("HARDWARE_INPUT","HIC: Controller Disconnected (Idx %d)\n", idx);

    if (!_s_controllers_array_ptr) {
        debugPrint("HARDWARE_INPUT","HIC Error: Controllers array pointer is null!");
        return;
    }

    bool foundCtl = false;
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (_s_controllers_array_ptr[i] == ctl) {
            _s_controllers_array_ptr[i] = nullptr;
            foundCtl = true;
            if (_s_gameContext_ptr && _s_gameContext_ptr->bluetoothManager) {
                _s_gameContext_ptr->bluetoothManager->onDisconnect(ctl);
            } else if (_s_bluetoothManager_ptr) { 
                 _s_bluetoothManager_ptr->onDisconnect(ctl);
            }
            else debugPrint("HARDWARE_INPUT","HIC Warning: BluetoothManager unavailable in onDisconnectedController_static");
            break;
        }
    }
    if (!foundCtl) {
        debugPrint("HARDWARE_INPUT","HIC Warning: Disconnected controller not found.");
    }
}