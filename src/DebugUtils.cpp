#include "DebugUtils.h"
#include <SerialForwarder.h>

// External reference to the serial forwarder
extern SerialForwarder* forwardedSerial_ptr;

// External references to debug flags
extern bool DEBUG_MASTER_ENABLE;
extern bool DEBUG_ANIMATOR;
extern bool DEBUG_GAME_STATS;
extern bool DEBUG_DIALOG_BOX;
extern bool DEBUG_PATH_GENERATOR;
extern bool DEBUG_CHARACTER_MANAGER;
extern bool DEBUG_SCENES;
extern bool DEBUG_WIFI_MANAGER;
extern bool DEBUG_BLUETOOTH;
extern bool DEBUG_SLEEP_CONTROLLER;
extern bool DEBUG_HARDWARE_INPUT;
extern bool DEBUG_SYSTEM;
extern bool DEBUG_EFFECTS_MANAGER;
extern bool DEBUG_WEATHER;
extern bool DEBUG_PARTICLE_SYSTEM;
extern bool DEBUG_TASK;
extern bool DEBUG_DEEP_SLEEP;
extern bool DEBUG_U8G2_WEBSTREAM;



// Check if debugging is enabled for a specific feature
bool isDebugEnabled(const char* feature) {
    if (!DEBUG_MASTER_ENABLE) return false;
    
    if (strcmp(feature, "ANIMATOR") == 0) return DEBUG_ANIMATOR;
    if (strcmp(feature, "GAME_STATS") == 0) return DEBUG_GAME_STATS;
    if (strcmp(feature, "DIALOG_BOX") == 0) return DEBUG_DIALOG_BOX;
    if (strcmp(feature, "PATH_GENERATOR") == 0) return DEBUG_PATH_GENERATOR;
    if (strcmp(feature, "CHARACTER_MANAGER") == 0) return DEBUG_CHARACTER_MANAGER;
    if (strcmp(feature, "SCENES") == 0) return DEBUG_SCENES;
    if (strcmp(feature, "WIFI_MANAGER") == 0) return DEBUG_WIFI_MANAGER;
    if (strcmp(feature, "BLUETOOTH") == 0) return DEBUG_BLUETOOTH;
    if (strcmp(feature, "SLEEP_CONTROLLER") == 0) return DEBUG_SLEEP_CONTROLLER;
    if (strcmp(feature, "HARDWARE_INPUT") == 0) return DEBUG_HARDWARE_INPUT;
    if (strcmp(feature, "SYSTEM") == 0) return DEBUG_SYSTEM;
    if (strcmp(feature, "EFFECTS_MANAGER") == 0) return DEBUG_EFFECTS_MANAGER;
    if (strcmp(feature, "WEATHER") == 0) return DEBUG_WEATHER;
    if (strcmp(feature, "PARTICLE_SYSTEM") == 0) return DEBUG_PARTICLE_SYSTEM;
    if (strcmp(feature, "MASTER") == 0) return DEBUG_MASTER_ENABLE;
    if (strcmp(feature, "TASK") == 0) return DEBUG_TASK;
    if (strcmp(feature, "DEEP_SLEEP") == 0) return DEBUG_DEEP_SLEEP;
    if (strcmp(feature, "U8G2_WEBSTREAM") == 0) return DEBUG_U8G2_WEBSTREAM;

    return false; // Unknown feature
}

// Debug print function - only prints if debugging is enabled for the feature
void debugPrint(const char* feature, const char* message) {
    if (isDebugEnabled(feature) && forwardedSerial_ptr != nullptr) {
        forwardedSerial_ptr->printf("[DEBUG:%s] %s\n", feature, message);
    }
}

// Debug printf function - only prints if debugging is enabled for the feature
void debugPrintf(const char* feature, const char* format, ...) {
    if (isDebugEnabled(feature) && forwardedSerial_ptr != nullptr) {
        char buffer[256]; // Buffer for the formatted message
        
        // Format the message with variable arguments
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        
        // Print the formatted message with the feature prefix
        forwardedSerial_ptr->printf("[DEBUG:%s] %s", feature, buffer);
    }
}

// Function to set debug flag for a specific feature
bool setDebugFlag(const char* feature, bool value) {
    if (strcmp(feature, "MASTER") == 0 || strcmp(feature, "ALL") == 0) {
        DEBUG_MASTER_ENABLE = value;
        return true;
    }
    else if (strcmp(feature, "ANIMATOR") == 0) { DEBUG_ANIMATOR = value; return true; }
    else if (strcmp(feature, "GAME_STATS") == 0) { DEBUG_GAME_STATS = value; return true; }
    else if (strcmp(feature, "DIALOG_BOX") == 0) { DEBUG_DIALOG_BOX = value; return true; }
    else if (strcmp(feature, "PATH_GENERATOR") == 0) { DEBUG_PATH_GENERATOR = value; return true; }
    else if (strcmp(feature, "CHARACTER_MANAGER") == 0) { DEBUG_CHARACTER_MANAGER = value; return true; }
    else if (strcmp(feature, "SCENES") == 0) { DEBUG_SCENES = value; return true; }
    else if (strcmp(feature, "WIFI_MANAGER") == 0) { DEBUG_WIFI_MANAGER = value; return true; }
    else if (strcmp(feature, "BLUETOOTH") == 0) { DEBUG_BLUETOOTH = value; return true; }
    else if (strcmp(feature, "SLEEP_CONTROLLER") == 0) { DEBUG_SLEEP_CONTROLLER = value; return true; }
    else if (strcmp(feature, "HARDWARE_INPUT") == 0) { DEBUG_HARDWARE_INPUT = value; return true; }
    else if (strcmp(feature, "SYSTEM") == 0) { DEBUG_SYSTEM = value; return true; }
    else if (strcmp(feature, "EFFECTS_MANAGER") == 0) { DEBUG_EFFECTS_MANAGER = value; return true; }
    else if (strcmp(feature, "WEATHER") == 0) { DEBUG_WEATHER = value; return true; }
    else if (strcmp(feature, "PARTICLE_SYSTEM") == 0) { DEBUG_PARTICLE_SYSTEM = value; return true; }
    else if (strcmp(feature, "TASK") == 0) { DEBUG_TASK = value; return true; }
    else if (strcmp(feature, "DEEP_SLEEP") == 0) { DEBUG_DEEP_SLEEP = value; return true; }
    else if (strcmp(feature, "U8G2_WEBSTREAM") == 0) { DEBUG_U8G2_WEBSTREAM = value; return true; }

    

    
    return false; // Unknown feature
}

// Function to get the current debug configuration as a string
String getDebugConfig() {
    String config = "Debug Configuration:\n";
    config += "MASTER: " + String(DEBUG_MASTER_ENABLE ? "ON" : "OFF") + "\n";
    config += "ANIMATOR: " + String(DEBUG_ANIMATOR ? "ON" : "OFF") + "\n";
    config += "GAME_STATS: " + String(DEBUG_GAME_STATS ? "ON" : "OFF") + "\n";
    config += "DIALOG_BOX: " + String(DEBUG_DIALOG_BOX ? "ON" : "OFF") + "\n";
    config += "PATH_GENERATOR: " + String(DEBUG_PATH_GENERATOR ? "ON" : "OFF") + "\n";
    config += "CHARACTER_MANAGER: " + String(DEBUG_CHARACTER_MANAGER ? "ON" : "OFF") + "\n";
    config += "SCENES: " + String(DEBUG_SCENES ? "ON" : "OFF") + "\n";
    config += "WIFI_MANAGER: " + String(DEBUG_WIFI_MANAGER ? "ON" : "OFF") + "\n";
    config += "BLUETOOTH: " + String(DEBUG_BLUETOOTH ? "ON" : "OFF") + "\n";
    config += "SLEEP_CONTROLLER: " + String(DEBUG_SLEEP_CONTROLLER ? "ON" : "OFF") + "\n";
    config += "HARDWARE_INPUT: " + String(DEBUG_HARDWARE_INPUT ? "ON" : "OFF") + "\n";
    config += "SYSTEM: " + String(DEBUG_SYSTEM ? "ON" : "OFF") + "\n";
    config += "EFFECTS_MANAGER: " + String(DEBUG_EFFECTS_MANAGER ? "ON" : "OFF") + "\n";
    config += "WEATHER: " + String(DEBUG_WEATHER ? "ON" : "OFF") + "\n";
    config += "PARTICLE_SYSTEM: " + String(DEBUG_PARTICLE_SYSTEM ? "ON" : "OFF") + "\n";
    config += "TASK: " + String(DEBUG_TASK ? "ON" : "OFF") + "\n";
    config += "DEEP_SLEEP: " + String(DEBUG_DEEP_SLEEP ? "ON" : "OFF") + "\n";
    config += "U8G2_WEBSTREAM: " + String(DEBUG_U8G2_WEBSTREAM ? "ON" : "OFF") + "\n";

    return config;
}