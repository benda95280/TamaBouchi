#include "HeavySnowWeatherEffect.h"
#include "../../../System/GameContext.h" // Ensure GameContext is known
#include "../../../DebugUtils.h"
#include "SerialForwarder.h"


HeavySnowWeatherEffect::HeavySnowWeatherEffect(GameContext& context) // Takes GameContext
    : SnowyWeatherEffect(context, true) { // Pass context and true for isHeavy
    if (_context.serialForwarder) _context.serialForwarder->println("HeavySnowWeatherEffect created (derived from Snowy)");
}

WeatherType HeavySnowWeatherEffect::getType() const {
    return WeatherType::HEAVY_SNOW;
}