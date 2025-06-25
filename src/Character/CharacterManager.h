// --- START OF FILE src/character/CharacterManager.h ---
#ifndef CHARACTER_MANAGER_H
#define CHARACTER_MANAGER_H
#include <map>
#include <vector>
#include <pgmspace.h>
#include "GameStats.h"
#include "../Helper/GraphicAssetTypes.h" // <<< MODIFIED: Include new path
#include "../DebugUtils.h"
// Forward declaration
class SerialForwarder;
extern SerialForwarder *forwardedSerial_ptr;
enum class GraphicType
{
    STATIC_IDLE,
    DOWNING_SHEET,
    SNOOZE_1,
    SNOOZE_2,
    SICKNESS_COLD,
    SICKNESS_HOT,
    SICKNESS_DIARRHEA,
    SICKNESS_HEADACHE,
    FLYING_BEE,
    FLYING_BUTTERFLY,
    FLYING_VEHICLE
};
// GraphicAssetData struct MOVED to Helper/GraphicAssetTypes.h
struct CharacterLevelData
{
    uint32_t level = 0;
    std::map<GraphicType, GraphicAssetData> graphicAssets;
    std::vector<Sickness> availableSicknesses;
    // --- NEW: Config flags for needs ---
    bool canBeHungry = true; // Default to true for higher levels
    bool canPoop = true;     // Default to true for higher levels
    // --- End NEW ---
    CharacterLevelData() = default;
};
namespace CharacterGraphicsL0
{
    std::map<GraphicType, GraphicAssetData> getLevel0AssetData();
}
// Add namespaces for L1 etc. when they are created
// namespace CharacterGraphicsL1 { std::map<GraphicType, GraphicAssetData> getLevel1AssetData(); }
class CharacterManager
{
public:
    CharacterManager(const CharacterManager &) = delete;
    CharacterManager &operator=(const CharacterManager &) = delete;
    static CharacterManager *getInstance();
    void init(GameStats *gameStats);
    void updateLevel(uint32_t currentAge);
    bool getGraphicAssetData(GraphicType type, GraphicAssetData &outData) const;

    // --- MOVED and new methods ---
    bool isSicknessAvailable(Sickness sicknessToCheck) const; // Moved from CharacterConfigL0
    bool currentLevelCanBeHungry() const;
    bool currentLevelCanPoop() const;
    // --- End MOVED and new methods ---

    uint32_t getCurrentManagedLevel() const;
    const std::vector<Sickness> &getAvailableSicknesses() const;

private:
    CharacterManager();
    ~CharacterManager() = default;
    static CharacterManager *_instance;
    std::map<uint32_t, CharacterLevelData> _levelDataMap;
    const CharacterLevelData *_currentLevelData = nullptr;
    GameStats *_gameStats_ptr = nullptr;

    void loadAllLevelData();
    void loadLevelData(uint32_t level); // <<< MODIFIED: Generic loader
};
#endif // CHARACTER_MANAGER_H
// --- END OF FILE src/character/CharacterManager.h ---
