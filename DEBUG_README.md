# TamaBouchi Debug System

This document describes the debug system implemented in the TamaBouchi project.

## Overview

The debug system allows enabling or disabling debug messages for specific features of the application. This helps in troubleshooting issues and understanding the behavior of the application without cluttering the serial output with unnecessary information.

## Debug Flags

The following debug flags are available:

| Flag | Description |
|------|-------------|
| `DEBUG_MASTER_ENABLE` | Master switch to enable/disable all debug output |
| `DEBUG_ANIMATOR` | Animation debugging |
| `DEBUG_GAME_STATS` | Game statistics debugging |
| `DEBUG_DIALOG_BOX` | Dialog box debugging |
| `DEBUG_PATH_GENERATOR` | Path generation debugging |
| `DEBUG_CHARACTER_MANAGER` | Character management debugging |
| `DEBUG_SCENES` | Scene-specific debugging |
| `DEBUG_WIFI_MANAGER` | WiFi manager debugging |
| `DEBUG_BLUETOOTH` | Bluetooth debugging |
| `DEBUG_SLEEP_CONTROLLER` | Deep sleep controller debugging |
| `DEBUG_HARDWARE_INPUT` | Hardware input controller debugging |
| `DEBUG_SYSTEM` | System-level debugging |
| `DEBUG_EFFECTS_MANAGER` | Effects manager debugging |
| `DEBUG_WEATHER` | Weather manager debugging |
| `DEBUG_PARTICLE_SYSTEM` | Particle system debugging |

## Debug Functions

The following functions are available for debugging:

### `bool isDebugEnabled(const char* feature)`

Checks if debugging is enabled for a specific feature.

Example:
```cpp
if (isDebugEnabled("ANIMATOR")) {
    // Debug code here
}
```

### `void debugPrint(const char* feature, const char* message)`

Prints a debug message if debugging is enabled for the specified feature.

Example:
```cpp
debugPrint("SYSTEM", "Initializing WiFiManager...");
```

### `void debugPrintf(const char* feature, const char* format, ...)`

Prints a formatted debug message if debugging is enabled for the specified feature.

Example:
```cpp
debugPrintf("SCENES", "Scene change requested. NextSceneID: %d", nextSceneId);
```

### `bool setDebugFlag(const char* feature, bool value)`

Sets the debug flag for a specific feature.

Example:
```cpp
setDebugFlag("ANIMATOR", true); // Enable animator debugging
```

### `String getDebugConfig()`

Returns a string with the current debug configuration.

Example:
```cpp
String config = getDebugConfig();
Serial.println(config);
```

## Serial Commands

The following serial commands are available to control the debug system:

| Command | Description |
|---------|-------------|
| `debug_status` | Shows current debug configuration |
| `debug_enable <feature>` | Enables debugging for a specific feature |
| `debug_disable <feature>` | Disables debugging for a specific feature |
| `debug_enable_all` | Enables all debugging features |
| `debug_disable_all` | Disables all debugging features (master switch off) |

## Implementation Details

The debug system is implemented in the following files:

- `DebugUtils.h`: Header file with function declarations
- `DebugUtils.cpp`: Implementation of debug functions
- `Main.ino`: Definition of debug flags

## Usage Guidelines

1. Always use the debug functions instead of directly using `forwardedSerial_ptr` for debug messages.
2. Use the appropriate feature tag for each debug message.
3. Format debug messages consistently with a clear prefix indicating the component and action.
4. Keep debug messages concise and informative.
5. Consider the performance impact of complex debug messages in time-critical code.

## Example

```cpp
// Check if debugging is enabled for a specific feature
if (isDebugEnabled("ANIMATOR")) {
    // Perform debug-only operations
}

// Print a debug message
debugPrint("SYSTEM", "Initializing WiFiManager...");

// Print a formatted debug message
debugPrintf("SCENES", "Scene change requested. NextSceneID: %d, Replace: %s",
            nextSceneId, replace ? "true" : "false");
```