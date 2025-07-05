#ifndef WEATHER_TYPES_H
#define WEATHER_TYPES_H

#include <stdint.h> // For uint8_t

enum class WeatherType : uint8_t {
    NONE = 0, // Default, no active weather effects
    SUNNY,
    CLOUDY, 
    RAINY,
    HEAVY_RAIN,
    SNOWY,        
    HEAVY_SNOW,   
    STORM,
    RAINBOW,
    WINDY,
    FOG,
    AURORA,
    UNKNOWN
};

enum class StrikeType : uint8_t {
    SMALL_1,      // Uses epd_bitmap_strike1
    SMALL_2,      // Uses epd_bitmap_strike2
    FULLSCREEN    // Uses epd_bitmap_strike3
};

enum class RainIntensityState : uint8_t {
    NONE,
    STARTING,
    PEAK,
    ENDING
};

#endif // WEATHER_TYPES_H