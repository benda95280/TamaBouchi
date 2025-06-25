#ifndef GAME_CONTEXT_H
#define GAME_CONTEXT_H

#include "WakeUpInfo.h" // <<< INCLUDE NEW HEADER

// Forward declarations for all managed services
class GameStats;
class WiFiManager;
class BluetoothManager;
class SerialCommandHandler;
class SerialForwarder;
class Renderer;
class InputManager;
class SceneManager;
class WeatherManager;
class CharacterManager;
class DeepSleepController;
class PeriodicTaskManager;
class HardwareInputController;
class PathGenerator;
class Preferences; 
class U8G2;
class PrequelManager;      

struct GameContext {
    GameStats* gameStats = nullptr;
    WiFiManager* wifiManager = nullptr;
    BluetoothManager* bluetoothManager = nullptr;
    SerialCommandHandler* commandHandler = nullptr; 
    SerialForwarder* serialForwarder = nullptr;
    Renderer* renderer = nullptr;                   
    InputManager* inputManager = nullptr;             
    SceneManager* sceneManager = nullptr;             
    WeatherManager* weatherManager = nullptr;
    CharacterManager* characterManager = nullptr;
    DeepSleepController* deepSleepController = nullptr;
    PeriodicTaskManager* periodicTaskManager = nullptr;
    HardwareInputController* hardwareInputController = nullptr; 
    PathGenerator* pathGenerator = nullptr;
    Preferences* preferences = nullptr; 
    U8G2* display = nullptr;          
    const uint8_t* defaultFont = nullptr; 
    PrequelManager* prequelManager = nullptr;
    WakeUpInfo lastWakeUpInfo;

    GameContext() = default;
};

#endif // GAME_CONTEXT_H