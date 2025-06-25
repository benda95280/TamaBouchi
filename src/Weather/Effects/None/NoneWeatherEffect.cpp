#include "NoneWeatherEffect.h"
#include "../../../System/GameContext.h" // Ensure GameContext is known
#include "../../../DebugUtils.h"
#include "SerialForwarder.h"


NoneWeatherEffect::NoneWeatherEffect(GameContext& context) // Takes GameContext
    : WeatherEffectBase(context) { // Pass context to base
    if (_context.serialForwarder) _context.serialForwarder->println("NoneWeatherEffect created");
}

void NoneWeatherEffect::init(unsigned long currentTime) {
    if (_context.serialForwarder) _context.serialForwarder->println("NoneWeatherEffect init");
}

void NoneWeatherEffect::update(unsigned long currentTime) { }
void NoneWeatherEffect::drawBackground() { }
void NoneWeatherEffect::drawForeground() { }

WeatherType NoneWeatherEffect::getType() const {
    return WeatherType::NONE;
}