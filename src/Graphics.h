// --- START OF FILE src/Graphics.h ---
#ifndef GRAPHICS_H
#define GRAPHICS_H

// Use PROGMEM for AVR if needed, but for ESP32 it's often optional.
// Using `const unsigned char` is generally fine for ESP32.
#ifdef ARDUINO_ARCH_AVR
#include <avr/pgmspace.h>
#elif defined(ESP32)
#include <pgmspace.h> // ESP32 defines PROGMEM in here
#else
#define PROGMEM // Define as empty for other architectures if needed
#endif

// --- UI Icons (8x8) ---
// Kept here as they are used by the main UI (IconMenuManager)
const unsigned char icon_placeholder_1[] PROGMEM = {
    0xff, 0x81, 0xbd, 0xdb, 0xdb, 0xbd, 0x81, 0xff, // Simple X
};
const unsigned char icon_placeholder_2[] PROGMEM = {
    0xff, 0xff, 0xdb, 0xdb, 0xdb, 0xdb, 0xff, 0xff, // Simple O
};
const unsigned char icon_placeholder_3[] PROGMEM = {
    0x18, 0x3c, 0x7e, 0xff, 0xff, 0x7e, 0x3c, 0x18, // Simple Diamond
};
const unsigned char icon_placeholder_4[] PROGMEM = { // Dummy 1
    0x00, 0x18, 0x24, 0x42, 0x42, 0x24, 0x18, 0x00, // Circle outline
};
const unsigned char icon_placeholder_5[] PROGMEM = { // Dummy 2
    0x00, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x00, // X outline
};
const unsigned char icon_placeholder_6[] PROGMEM = { // Dummy 3
    0x00, 0x3e, 0x41, 0x41, 0x41, 0x41, 0x3e, 0x00, // Square outline
};
const unsigned char icon_stats[] PROGMEM = { // Example Stats Icon (Graph)
    0x08, 0x1C, 0x1C, 0x3E, 0x3E, 0x7F, 0x7F, 0x00,
};
const unsigned char icon_parameters[] PROGMEM = { // Example Parameters Icon (Gears)
    0x00, 0x2A, 0x5F, 0xBD, 0xBD, 0x5F, 0x2A, 0x00,
};
const unsigned char icon_actions[] PROGMEM =     { 0x18, 0x24, 0x42, 0x5a, 0x42, 0x24, 0x18, 0x00 }; // Example Actions (Star/Sparkle)

// --- Character Graphics moved to character/level0/CharacterGraphics_L0.h ---

// --- Action Icons moved to SceneAction/ActionGraphics.h ---


#endif // GRAPHICS_H
// --- END OF FILE src/Graphics.h ---
