#include "HeavyRainWeatherEffect.h"
#include "../../../System/GameContext.h" // Ensure GameContext is known for _context usage
#include "../../../DebugUtils.h"         // For debugPrint/debugPrintf
#include "SerialForwarder.h"


HeavyRainWeatherEffect::HeavyRainWeatherEffect(GameContext& context) // Takes GameContext
    : RainyWeatherEffect(context, true) { // Pass context and true for isHeavy to the RainyWeatherEffect constructor
    if (_context.serialForwarder) { // Access logger via context
        _context.serialForwarder->println("HeavyRainWeatherEffect created (derived from Rainy)");
    }
}

WeatherType HeavyRainWeatherEffect::getType() const {
    return WeatherType::HEAVY_RAIN;
}

// Note: init(), update(), drawBackground(), drawForeground() are inherited from RainyWeatherEffect.
// RainyWeatherEffect's methods should already correctly use the _isHeavyRain flag
// (which is set to true by this constructor) to adjust particle density, speed, appearance etc.
// If specific overrides are needed for HeavyRain that differ significantly from how RainyWeatherEffect
// handles its "heavy" mode, then those methods would be implemented here.
// For example, if HeavyRain needed a different particle character or a different sound cue (if sound was implemented).
// Based on the current structure, RainyWeatherEffect likely handles the "heaviness" via the boolean passed.