#include "WeatherEffectBase.h"
#include "../../System/GameContext.h" // <<< NEW INCLUDE
#include "../../DebugUtils.h"

// WeatherEffectBase constructor now takes GameContext
WeatherEffectBase::WeatherEffectBase(GameContext& context)
    : _context(context), // Store GameContext reference
      _intensityState(RainIntensityState::NONE), 
      _particleDensity(0),
      _currentWindFactor(0.0f)
{
    // Access renderer, gameStats, logger via _context if needed immediately
    // e.g., if (_context.serialForwarder) _context.serialForwarder->println("Effect Base created.");
    // For this base class, it's often fine to leave them unaccessed in constructor.
    debugPrint("WEATHER", "WeatherEffectBase constructed with GameContext.");
}