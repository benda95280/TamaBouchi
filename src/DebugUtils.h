#ifndef DEBUG_UTILS_H
#define DEBUG_UTILS_H

#include <Arduino.h>

// Check if debugging is enabled for a specific feature
bool isDebugEnabled(const char* feature);

// Debug print function - only prints if debugging is enabled for the feature
void debugPrint(const char* feature, const char* message);

// Debug printf function - only prints if debugging is enabled for the feature
void debugPrintf(const char* feature, const char* format, ...);

// Function to set debug flag for a specific feature
bool setDebugFlag(const char* feature, bool value);

// Function to get the current debug configuration as a string
String getDebugConfig();

#endif // DEBUG_UTILS_H