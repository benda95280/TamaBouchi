// --- START OF FILE src/character/level0/CharacterGraphics_L0.cpp ---

#include "CharacterGraphics_L0.h"
#include "../CharacterManager.h" // Need GraphicType enum
#include "../../Helper/GraphicAssetTypes.h" // <<< MODIFIED: Include new path for GraphicAssetData

// Use a namespace to encapsulate level-specific graphic loading logic
namespace CharacterGraphicsL0 {

    // Function to create and return the map of graphic assets for this level
    std::map<GraphicType, GraphicAssetData> getLevel0AssetData() {
        std::map<GraphicType, GraphicAssetData> assets;

        // --- Populate Graphic Assets using constructors ---
        assets[GraphicType::STATIC_IDLE] = GraphicAssetData(bmp_Oeuf_static, CHARACTER_WIDTH, CHARACTER_HEIGHT);
        assets[GraphicType::DOWNING_SHEET] = GraphicAssetData(epd_bitmap_Oeuf_Downing, DOWNING_FRAME_WIDTH, DOWNING_FRAME_HEIGHT, DOWNING_FRAME_COUNT, DOWNING_BYTES_PER_FRAME, DOWNING_FRAME_DURATION_MS);
        assets[GraphicType::SNOOZE_1] = GraphicAssetData(bmp_Oeuf_snooze1, CHARACTER_WIDTH, CHARACTER_HEIGHT);
        assets[GraphicType::SNOOZE_2] = GraphicAssetData(bmp_Oeuf_snooze2, CHARACTER_WIDTH, CHARACTER_HEIGHT);

        // Sickness Overlays (using static bitmap constructor)
        assets[GraphicType::SICKNESS_COLD] = GraphicAssetData(epd_bitmap_Cold, SICKNESS_OVERLAY_WIDTH, SICKNESS_OVERLAY_HEIGHT);
        assets[GraphicType::SICKNESS_DIARRHEA] = GraphicAssetData(epd_bitmap_Diarrhea, SICKNESS_OVERLAY_WIDTH, SICKNESS_OVERLAY_HEIGHT);
        assets[GraphicType::SICKNESS_HEADACHE] = GraphicAssetData(epd_bitmap_HeadHache, SICKNESS_OVERLAY_WIDTH, SICKNESS_OVERLAY_HEIGHT);
        assets[GraphicType::SICKNESS_HOT] = GraphicAssetData(epd_bitmap_Hot, SICKNESS_OVERLAY_WIDTH, SICKNESS_OVERLAY_HEIGHT);

        // Flying Sprites (using spritesheet constructor)
        assets[GraphicType::FLYING_BEE] = GraphicAssetData(epd_bitmap_Bee_flying, FLYING_FRAME_WIDTH, FLYING_FRAME_HEIGHT, FLYING_FRAME_COUNT, FLYING_BYTES_PER_FRAME, FLYING_FRAME_DURATION_MS);
        assets[GraphicType::FLYING_BUTTERFLY] = GraphicAssetData(epd_bitmap_butterfly_flying, FLYING_FRAME_WIDTH, FLYING_FRAME_HEIGHT, FLYING_FRAME_COUNT, FLYING_BYTES_PER_FRAME, FLYING_FRAME_DURATION_MS);
        assets[GraphicType::FLYING_VEHICLE] = GraphicAssetData(epd_bitmap_spacial_flying, FLYING_FRAME_WIDTH, FLYING_FRAME_HEIGHT, FLYING_FRAME_COUNT, FLYING_BYTES_PER_FRAME, FLYING_FRAME_DURATION_MS);

        return assets;
    }

} // namespace CharacterGraphicsL0

// --- END OF FILE src/character/level0/CharacterGraphics_L0.cpp ---