// --- START OF FILE src/SceneMain/IconMenuManager.h ---
#ifndef ICON_MENU_MANAGER_H
#define ICON_MENU_MANAGER_H

#include <vector>
#include <memory> 
#include "../../Graphics.h" 
#include "IconInfo.h" // Include IconInfo.h (which now contains IconAction enum)
#include "espasyncbutton.hpp" 

// Forward declarations
class Renderer;
struct GameStats; 

// --- IconAction enum definition MOVED to IconInfo.h ---

// Enum for input keys relevant to the menu manager
enum class MenuInputKey {
    LEFT,
    RIGHT,
    SELECT
};


class IconMenuManager {
public:
    // --- Constants exposed for MainScene layout ---
    static const int ICON_WIDTH = 8;
    static const int ICON_HEIGHT = 8;
    static const int ICON_SPACING = 4; 
    static const int SELECTOR_SIZE = ICON_WIDTH + 4; 
    static const int SELECTION_TIMEOUT_TICKS = 300; 

    // --- Row definitions ---
    enum MenuRowType { 
        ROW_NONE = -1,
        ROW_TOP = 0,
        ROW_BOTTOM = 1
    };


    IconMenuManager(Renderer& renderer, const GameStats& stats);

    void setupLayout(); 
    void resetSelection(); 
    void update(); 
    IconAction handleInput(MenuInputKey key); 

    void drawRow(MenuRowType rowType, int menuBaseScreenY);

    MenuRowType getSelectedRow() const { return _selectedRow; }
    int getSelectedIconIndex() const { return _selectedIconIndex; }


private:
    Renderer& _renderer;
    const GameStats& _gameStats; 

    std::vector<IconInfo> _topIcons;
    std::vector<IconInfo> _bottomIcons;
    MenuRowType _selectedRow;
    int _selectedIconIndex;
    unsigned long _selectionTimeoutCounter;

    void drawIcon(int screenX, int screenY, const unsigned char* bitmap); 
    void drawSelectionBoxForIcon(const IconInfo& icon, int menuBaseScreenY);
    void resetSelectionTimeout();
    void selectDefaultIcon();

};

#endif // ICON_MENU_MANAGER_H
// --- END OF FILE src/SceneMain/IconMenuManager.h ---