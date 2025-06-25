// --- START OF FILE src/character/CharacterManager.cpp ---
#include "CharacterManager.h"
#include "SerialForwarder.h"
#include "../Helper/GraphicAssetTypes.h" // <<< MODIFIED: Include new path for GraphicAssetData
#include "../DebugUtils.h"
#include "level0/CharacterGraphics_L0.h"
#include "level0/CharacterConfig_L0.h"

CharacterManager *CharacterManager::_instance = nullptr;
CharacterManager::CharacterManager()
{
    debugPrint("CHARACTER_MANAGER", "CharacterManager constructing...");
    loadAllLevelData();
    debugPrint("CHARACTER_MANAGER", "CharacterManager construction complete.");
}
CharacterManager *CharacterManager::getInstance()
{
    if (_instance == nullptr)
    {
        _instance = new CharacterManager();
    }
    return _instance;
}
void CharacterManager::init(GameStats *gameStats)
{
    _gameStats_ptr = gameStats;
    if (_gameStats_ptr)
    {
        updateLevel(_gameStats_ptr->age);
        debugPrintf("CHARACTER_MANAGER", "CharacterManager initialized. Active level data: %u\n", getCurrentManagedLevel());
    }
    else
    {
        debugPrint("CHARACTER_MANAGER", "CharacterManager Error: GameStats pointer is null during init!");
        updateLevel(0);
        if (_currentLevelData)
        {
            debugPrint("CHARACTER_MANAGER", "CharacterManager Warning: Defaulting to Level 0 data due to missing GameStats.");
        }
        else
        {
            debugPrint("CHARACTER_MANAGER", "CharacterManager FATAL Error: Level 0 data could not be loaded!");
        }
    }
}
void CharacterManager::loadAllLevelData()
{
    // This function will call loadLevelData for each implemented level
    loadLevelData(0);
    // When Level 1 is added, uncomment:
    // loadLevelData(1);
    // etc.
}
void CharacterManager::loadLevelData(uint32_t level)
{
    if (_levelDataMap.count(level))
    {
        debugPrintf("CHARACTER_MANAGER", "Level %u data already loaded.\n", level);
        return;
    }
    debugPrintf("CHARACTER_MANAGER", "Loading character data for Level %u...\n", level);
    CharacterLevelData data;
    data.level = level;

    switch (level)
    {
    case 0:
        data.graphicAssets = CharacterGraphicsL0::getLevel0AssetData();
        data.availableSicknesses = CharacterConfigL0::availableSicknesses;
        data.canBeHungry = CharacterConfigL0::canBeHungry;
        data.canPoop = CharacterConfigL0::canPoop;
        break;
    // --- Example for future Level 1 ---
    // case 1:
    //     // Ensure CharacterGraphicsL1 and CharacterConfigL1 files and namespaces exist
    //     // and are included at the top of this file.
    //     data.graphicAssets = CharacterGraphicsL1::getLevel1AssetData();
    //     data.availableSicknesses = CharacterConfigL1::availableSicknessesL1;
    //     data.canBeHungry = CharacterConfigL1::canBeHungryL1;
    //     data.canPoop = CharacterConfigL1::canPoopL1;
    //     break;
    // --- End Example for Level 1 ---
    default:
        debugPrintf("CHARACTER_MANAGER", "Error: No data loader defined for level %u. Cannot load.\n", level);
        return; // Do not add incomplete data to the map
    }

    _levelDataMap[level] = data;
    debugPrintf("CHARACTER_MANAGER", "Successfully loaded data for Level %u.\n", level);
}
void CharacterManager::updateLevel(uint32_t currentAge)
{
    uint32_t targetLevel = 0;
    const CharacterLevelData *foundData = nullptr;

    // Find the highest level definition that is less than or equal to currentAge
    uint32_t highestMatchingLevel = 0;
    bool aMatchWasFound = false;

    for (const auto &pair : _levelDataMap)
    {
        if (pair.first <= currentAge)
        { // If the defined level is attainable at current age
            if (pair.first >= highestMatchingLevel)
            { // And it's the highest such level found so far
                highestMatchingLevel = pair.first;
                foundData = &pair.second;
                aMatchWasFound = true;
            }
        }
    }

    // If no level matched (e.g., currentAge is less than the lowest defined level, or map is empty)
    // default to trying to load/use level 0.
    if (!aMatchWasFound)
    {
        if (_levelDataMap.count(0))
        {
            foundData = &_levelDataMap.at(0);
            highestMatchingLevel = 0;
            debugPrintf("CHARACTER_MANAGER", "CharacterManager: No direct level match for age %u. Defaulting to Level 0.\n", currentAge);
        }
        else
        {
            // This case should be rare if Level 0 is always loaded.
            debugPrintf("CHARACTER_MANAGER", "CharacterManager Error: No level data found for age %u or lower, AND Level 0 data is missing!\n", currentAge);
            _currentLevelData = nullptr; // Ensure no stale data is used
            return;
        }
    }

    if (foundData != nullptr)
    {
        if (_currentLevelData == nullptr || _currentLevelData->level != highestMatchingLevel)
        {
            _currentLevelData = foundData;
            debugPrintf("CHARACTER_MANAGER", "CharacterManager: Set active character level data to %u (for age %u).\n", highestMatchingLevel, currentAge);
        }
    }
    else
    { // This block should ideally not be reached if Level 0 is always available
        if (_currentLevelData == nullptr)
        { // Only print error if we truly have no current level set
            debugPrintf("CHARACTER_MANAGER", "CharacterManager Error: No suitable level data found for age %u. _currentLevelData remains null.\n", currentAge);
        }
    }
}
bool CharacterManager::getGraphicAssetData(GraphicType type, GraphicAssetData &outData) const
{
    if (!_currentLevelData)
    {
        debugPrint("CHARACTER_MANAGER", "CharacterManager::getGraphicAssetData Error: No current level data loaded.");
        return false;
    }

    auto it = _currentLevelData->graphicAssets.find(type);
    if (it != _currentLevelData->graphicAssets.end())
    {
        outData = it->second;
        return outData.isValid();
    }
    else
    {
        debugPrintf("CHARACTER_MANAGER", "CharacterManager::getGraphicAssetData Error: GraphicType %d not found in current level data.\n", (int)type);
        return false;
    }
}
// --- Implementation for new/moved methods ---
bool CharacterManager::isSicknessAvailable(Sickness sicknessToCheck) const
{
    if (!_currentLevelData)
    {
        debugPrint("CHARACTER_MANAGER", "CharacterManager::isSicknessAvailable Error: No current level data.");
        return false; // Or default to true/false based on desired behavior
    }
    for (const auto &available : _currentLevelData->availableSicknesses)
    {
        if (sicknessToCheck == available)
        {
            return true;
        }
    }
    return false;
}
bool CharacterManager::currentLevelCanBeHungry() const
{
    if (!_currentLevelData)
    {
        debugPrint("CHARACTER_MANAGER", "CharacterManager::currentLevelCanBeHungry Error: No current level data.");
        return true; // Default to true if no data
    }
    return _currentLevelData->canBeHungry;
}
bool CharacterManager::currentLevelCanPoop() const
{
    if (!_currentLevelData)
    {
        debugPrint("CHARACTER_MANAGER", "CharacterManager::currentLevelCanPoop Error: No current level data.");
        return true; // Default to true if no data
    }
    return _currentLevelData->canPoop;
}
// --- End Implementation ---
uint32_t CharacterManager::getCurrentManagedLevel() const
{
    return (_currentLevelData) ? _currentLevelData->level : 0; // Default to 0 if no data
}
const std::vector<Sickness> &CharacterManager::getAvailableSicknesses() const
{
    static const std::vector<Sickness> emptyVector;
    if (!_currentLevelData)
    {
        debugPrint("CHARACTER_MANAGER", "CharacterManager::getAvailableSicknesses Warning: No current level data!");
        return emptyVector;
    }
    return _currentLevelData->availableSicknesses;
}
// --- END OF FILE src/character/CharacterManager.cpp ---
