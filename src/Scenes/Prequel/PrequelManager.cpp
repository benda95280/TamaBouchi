#include "PrequelManager.h"
// #include "SerialForwarder.h" // Access via context if GameContext were passed
#include "../../System/GameContext.h" // <<< NEW INCLUDE
#include "../../DebugUtils.h"


PrequelManager::PrequelManager(GameContext& context) : // Takes GameContext
    _context(context) // Store context reference
{
    debugPrint("SCENE","PrequelManager Initialized with GameContext.");
}

String PrequelManager::getNextSceneNameAfterBootOrPrequel() { 
    if (!_context.gameStats) { // Access via context
        debugPrint("SCENE","PrequelManager Error: GameStats (via context) is null! Defaulting to MAIN scene.");
        return "MAIN"; 
    }
    GameStats* gameStats = _context.gameStats; // Convenience pointer

    if (gameStats->selectedLanguage == LANGUAGE_UNINITIALIZED) {
        debugPrint("SCENE","PrequelManager: Language not set. -> LANGUAGE_SELECT_PREQUEL");
        return "LANGUAGE_SELECT_PREQUEL";
    }

    if (gameStats->completedPrequelStage == PrequelStage::PREQUEL_FINISHED) {
        debugPrint("SCENE","PrequelManager: Prequel finished. -> MAIN");
        return "MAIN";
    }

    String nextPrequelSceneName = "MAIN"; 
    switch (gameStats->completedPrequelStage) {
        case PrequelStage::NONE: 
        case PrequelStage::LANGUAGE_SELECTED:
            nextPrequelSceneName = "PREQUEL_STAGE_1";
            debugPrint("SCENE","PrequelManager: Language selected or NONE. -> PREQUEL_STAGE_1");
            break;
        case PrequelStage::STAGE_1_AWAKENING_COMPLETE:
            nextPrequelSceneName = "PREQUEL_STAGE_2";
            debugPrint("SCENE","PrequelManager: Stage 1 complete. -> PREQUEL_STAGE_2");
            break;
        case PrequelStage::STAGE_2_CONGLOMERATE_COMPLETE:
            nextPrequelSceneName = "PREQUEL_STAGE_3";
            debugPrint("SCENE","PrequelManager: Stage 2 complete. -> PREQUEL_STAGE_3");
            break;
        case PrequelStage::STAGE_3_JOURNEY_COMPLETE:
            nextPrequelSceneName = "PREQUEL_STAGE_4";
            debugPrint("SCENE","PrequelManager: Stage 3 complete. -> PREQUEL_STAGE_4");
            break;
        case PrequelStage::STAGE_4_SHELLWEAVE_COMPLETE:
            debugPrint("SCENE","PrequelManager: Stage 4 complete. Marking prequel as finished. -> MAIN.");
            gameStats->setCompletedPrequelStage(PrequelStage::PREQUEL_FINISHED); 
            gameStats->save();
            nextPrequelSceneName = "MAIN";
            break;
        default:
            debugPrintf("SCENE","PrequelManager: Unknown prequel stage %d. Defaulting to MAIN.\n", (int)gameStats->completedPrequelStage);
            nextPrequelSceneName = "MAIN";
            break;
    }
    return nextPrequelSceneName;
}