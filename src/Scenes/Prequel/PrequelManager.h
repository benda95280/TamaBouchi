#ifndef PREQUEL_MANAGER_H
#define PREQUEL_MANAGER_H

#include "GameStats.h"        
#include "../../DebugUtils.h"   
#include <String.h>           
#include "../../System/GameContext.h" // <<< NEW INCLUDE

class PrequelManager {
public:
    PrequelManager(GameContext& context); // Takes GameContext

    String getNextSceneNameAfterBootOrPrequel(); 

private:
    GameContext& _context; // Store reference to GameContext
    // GameStats* _gameStats; // Access via _context.gameStats
};

#endif // PREQUEL_MANAGER_H