#ifndef GLOBAL_MAPPINGS_H
#define GLOBAL_MAPPINGS_H

#include "driver/gpio.h" // Include gpio_num_t definition

// Declare virtual button GPIO numbers as extern constants
extern const gpio_num_t VIRTUAL_BTN_OK;
extern const gpio_num_t VIRTUAL_BTN_LEFT;
extern const gpio_num_t VIRTUAL_BTN_RIGHT;

// Declare the physical button pins
extern const gpio_num_t PHYSICAL_UP_BUTTON_PIN;
extern const gpio_num_t PHYSICAL_DOWN_BUTTON_PIN;

// Declare the WAKEUP_BUTTON_PIN as extern const
extern const gpio_num_t WAKEUP_BUTTON_PIN;

void updateLastActivityTime(); // Declaration

#endif // GLOBAL_MAPPINGS_H
