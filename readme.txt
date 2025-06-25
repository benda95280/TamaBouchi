# TamaBouchi ESP32 Project (EDGE Engine)

## Overview

This project is a Tamagotchi-like game based on the game engine framework "EDGE" (ESP32 Display Game Engine), built for the ESP32 platform. It features a graphical display using U8g2, various game mechanics, interactive scenes, Bluetooth controller support, WiFi capabilities with OTA updates, and a web-based serial monitor, and web-based OLED Screen Viewer.

## Features

*   **Rewritten Game Engine (EDGE):**
    *   Scene Management
    *   Input Management (physical buttons, Bluetooth controllers)
    *   Rendering Abstraction (using U8g2)
    *   Entity-Component (basic structure)
*   **Game Mechanics & Scenes:**
    *   Character stats (age, health, happiness, hunger, etc.)
    *   Prequel story/tutorial sequence with multiple stages
    *   Main game loop with idle animations and character interaction
    *   Action Menu (Clean, Medicine, Feed, Play, Sleep)
    *   Stats & Parameters screens
    *   Sleeping state
    *   Flappy Bird Minigame
    *   Dialog Box system with formatting support
    *   Particle System
    *   Screen Effects (shake, fade)
    *   Path Generation (using FastNoiseLite)
    *   Weather System (Sunny, Cloudy, Rainy, Storm, Rainbow, Snow, etc.)
    *   Animated background birds
*   **Hardware & System:**
    *   ESP32 based
    *   SSD1306/SH1106 OLED display support (via U8g2)
    *   Bluetooth Classic controller support (via Bluepad32)
    *   WiFi connectivity
    *   Over-The-Air (OTA) updates (via PrettyOTA)
    *   WebSerial interface for commands and debugging (via MycilaWebSerial)
    *   Deep Sleep functionality for power saving
    *   Persistent storage for game state (via Preferences)
    *   Localization support (English, French)
*   **Development & Debugging:**
    *   Modular scene-based architecture
    *   Extensive debug logging system with feature-specific toggles
    *   Serial command handler for testing and development

## Project Structure

*   `src/`: Main application code (`Main.cpp`), game scenes, character logic, system controllers, helper utilities.
*   `include/`: Some shared header files like `FastNoiseLite.h` and `GEM_u8g2Rewrite.h`.
*   `images/`: Sources of somes images

## Hardware (Screen & Button could be virtual and manager with OLED Screen Viewer)

*   ESP32 Development Board
*   SSD1306 or SH1106 OLED Display
*   Physical buttons for input

## Software Dependencies & Third-Party Libraries

This project utilizes several third-party libraries. Their original licenses must be respected. Key dependencies include:

*   **ESP32-Game-Engine [EDGE]**: by Nicolas Bourré, licensed under **MIT**.
*   **MycilaWebSerial**: by Mathieu Carbou, licensed under **GPL-3.0**.
*   **U8g2 Library**: by Oliver Kraus, licensed under the **BSD 2-Clause License**.
*   **GEM - Arduino General Encapsulated Menu**: by Armin Joachimsmeyer, licensed under **MIT License**.
*   **FastNoiseLite**: by Auburn / Jordan Peck, licensed under the **MIT License**.
*   **Bluepad32**: by Ricardo Quesada, licensed under the **Apache License 2.0**.
*   **ESPAsyncWebServer & AsyncTCP**: licensed under **LGPL-3.0**.
*   **PrettyOTA**: by Tomoyuki Furusawa, licensed under the **MIT License**.
*   **ESPAsyncButton**: by Ardestan, licensed under the **MIT License**.
*   **ESP-IDF Components** (including Preferences, FreeRTOS): Primarily **Apache License 2.0** and **MIT License**.


## Installation & Setup

1.  **PlatformIO IDE**: This project is structured for PlatformIO. It's recommended to use VS Code with the PlatformIO extension.
2.  **Install ESP32 Plaform**: Ensure you have the Espressif 32 platform installed in PlatformIO.
3.  **Libraries**: PlatformIO should automatically manage most library dependencies listed in `platformio.ini` (not provided, but assumed). You may need to manually add some libraries to your `lib` folder or ensure they are correctly specified in `platformio.ini`.
    *   Ensure `FastNoiseLite.h` and `GEM_u8g2Rewrite.h` are correctly placed in your `include` path or a local library folder.
    *   Ensure the `MycilaWebSerial-main` library is correctly placed in your `lib` folder.
4.  **Hardware Connections**: Connect your ESP32, display, and buttons according to the pin definitions in `Main.ino` and `DisplayConfig.h`.
5.  **Build & Upload**: Use PlatformIO to build and upload the firmware to your ESP32.

## Usage

*   Upon first boot or after a reset of language settings, you will be prompted to select a language.
*   The device will then proceed through a prequel/tutorial sequence.
*   Interact with the device using the physical buttons (Up, Down, OK) or a connected Bluetooth gamepad.
*   Connect to the device's WiFi (if configured for AP mode or connected to your network) to access OTA updates, the WebSerial interface or OLED Screen Viewer. The WebSerial interface can be used for debugging and sending commands (Type 'help').

## Licensing

This project as a whole is licensed under the **GPL-3.0**.

You are free to use, study, share, and modify this software under the terms of the GPL-3.0. Any derivative works or distributions of this software (or works based on it) must also be licensed under the GPL-3.0.

Please see the `LICENSE` file in this repository for the full text of the GPL-3.0.

The licenses for third-party components used in this project (listed under "Software Dependencies") must also be respected. Their respective license files should be included with any distribution.

## Contributing

Contributions are welcome! Please feel free to submit issues or pull requests.