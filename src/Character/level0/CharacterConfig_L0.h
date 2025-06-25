// --- START OF FILE src/character/level0/CharacterConfig_L0.h ---
#ifndef CHARACTER_CONFIG_L0_H
#define CHARACTER_CONFIG_L0_H

#include "GameStats.h" 
#include <vector>
#include <pgmspace.h>

// --- Level 0 Configuration ---

namespace CharacterConfigL0 {

    // Define which sickness types are possible/available at Level 0
    const std::vector<Sickness> availableSicknesses = {
        Sickness::COLD,
        // Sickness::DIARRHEA, // Egg cannot have diarrhea
        Sickness::HEADACHE,
        Sickness::HOT
    };

    // Define needs applicable at Level 0
    const bool canBeHungry = false; // Egg doesn't get hungry in the conventional sense
    const bool canPoop = false;     // Egg doesn't poop

    // isSicknessAvailable helper function will be moved to CharacterManager
    // inline bool isSicknessAvailable(Sickness sicknessToCheck) { /* ... */ } // REMOVED

} // namespace CharacterConfigL0

#endif // CHARACTER_CONFIG_L0_H
// --- END OF FILE src/character/level0/CharacterConfig_L0.h ---