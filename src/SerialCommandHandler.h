#ifndef SERIAL_COMMAND_HANDLER_H
#define SERIAL_COMMAND_HANDLER_H

#include <Arduino.h>
#include <Preferences.h> 
#include "SerialForwarder.h" 
#include "Weather/WeatherTypes.h" 
#include "GameStats.h" 
#include "System/DeepSleepController.h" 
#include "System/GameContext.h" // <<< NEW INCLUDE

// Forward declarations
class WiFiManager;
class BluetoothManager;
class EDGE;
class WeatherManager; 

class SerialCommandHandler {
public:
    // Constructor now takes GameContext
    SerialCommandHandler(GameContext& context); // Simplified constructor

    void init(); 
    void handleSerialInput(); 
    void processSerialCommand(String command); 

private:
    GameContext& _context; // Store reference to GameContext

    // Remove individual manager pointers if accessed via context
    // SerialForwarder& _serial; // Now _context.serialForwarder
    // Preferences& _prefs;      // Now _context.preferences
    // GameStats& _stats;        // Now _context.gameStats
    // WiFiManager& _wifiMgr;    // Now _context.wifiManager
    // BluetoothManager& _btMgr; // Now _context.bluetoothManager
    // EDGE* _engine_ptr;        // Now _context.engine (or its components like sceneManager)
    // DeepSleepController* _deepSleepController_ptr; // Now _context.deepSleepController

    String _commandBuffer = "";
    bool _commandComplete = false;

    void printHelp();
    bool stringToWeatherType(const String& s, WeatherType& outType); 
    bool stringToSickness(const String& s, Sickness& outType);
    void handleDialogShow(const String& args);
    void handleDialogTemp(const String& args);
    void handleForceSleep(const String& args); 
    bool stringToPrequelStage(const String& s, PrequelStage& outStage); 
    void handleSpawnBirds(const String& args);

    void listScenes();
    void setScene(const String& sceneNameOrIdStr); 
    void printSystemInfo(); 
};

#endif // SERIAL_COMMAND_HANDLER_H