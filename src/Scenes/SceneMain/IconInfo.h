// --- START OF FILE src/SceneMain/IconInfo.h ---
#ifndef ICON_INFO_H
#define ICON_INFO_H

#include <stdint.h>

// --- MOVED IconAction enum definition HERE ---
enum class IconAction {
    NONE,
    GOTO_STATS,
    GOTO_PARAMS,
    GOTO_ACTION_MENU,
    ACTION_ICON1, 
    ACTION_ICON2,
    ACTION_ICON3,
    ACTION_ICON4,
    ACTION_ICON5,
    ACTION_ICON6,
    ACTION_SLEEP 
};
// --- END MOVED ---

struct IconInfo {
    const unsigned char* bitmap;
    IconAction action; 
    int x; // X position on screen
    int y; // Y position RELATIVE TO THE TOP OF ITS MENU BAR

    // Default arguments now work as IconAction is fully defined above
    IconInfo(const unsigned char* bmp = nullptr, IconAction act = IconAction::NONE, int iconX = -1, int iconY = -1)
        : bitmap(bmp), action(act), x(iconX), y(iconY) {}

    IconInfo(); 

};

#endif // ICON_INFO_H
// --- END OF FILE src/SceneMain/IconInfo.h ---