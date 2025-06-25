// --- START OF FILE src/SceneMain/IconMenuManager.cpp ---
#include "IconMenuManager.h" // Include own header (which defines IconAction via IconInfo.h)
#include "IconInfo.h"        // Include IconInfo struct definition
#include "../../DebugUtils.h"

IconInfo::IconInfo() : bitmap(nullptr), action(IconAction::NONE), x(-1), y(-1) {}

#include "Renderer.h"
#include "GameStats.h"
#include "../../Graphics.h" 
#include <U8g2lib.h>
#include <Arduino.h>
#include <vector> 
#include "SerialForwarder.h"


extern SerialForwarder* forwardedSerial_ptr; 

IconMenuManager::IconMenuManager(Renderer& renderer, const GameStats& stats) :
    _renderer(renderer),
    _gameStats(stats),
    _selectedRow(ROW_NONE),
    _selectedIconIndex(-1),
    _selectionTimeoutCounter(0)
{}

void IconMenuManager::setupLayout() {
    int screenWidth = _renderer.getWidth();

    debugPrintf("SCENES", "IconMenuManager: Setting up layout for age %u...", _gameStats.age);
    _topIcons.clear();
    _bottomIcons.clear();

    const int iconRelativeY = ICON_SPACING; 

    IconAction statsAction = IconAction::GOTO_STATS;
    IconAction paramsAction = IconAction::GOTO_PARAMS;
    IconAction actionsAction = IconAction::GOTO_ACTION_MENU;
    IconAction sleepAction = IconAction::ACTION_SLEEP; 
    IconAction action1 = IconAction::ACTION_ICON1;
    IconAction action2 = IconAction::ACTION_ICON2;
    IconAction action3 = IconAction::ACTION_ICON3;
    IconAction action4 = IconAction::ACTION_ICON4;
    IconAction action5 = IconAction::ACTION_ICON5;
    IconAction action6 = IconAction::ACTION_ICON6;


    if (_gameStats.age == 0) {
        _topIcons.push_back(IconInfo(icon_stats, statsAction, ICON_SPACING, iconRelativeY));
        _topIcons.push_back(IconInfo(icon_parameters, paramsAction, screenWidth - ICON_WIDTH - ICON_SPACING, iconRelativeY));
        _bottomIcons.push_back(IconInfo(icon_actions, actionsAction, ICON_SPACING, iconRelativeY));
        int sleepIconX = ICON_SPACING + ICON_WIDTH + ICON_SPACING;
        _bottomIcons.push_back(IconInfo(icon_placeholder_4, sleepAction, sleepIconX, iconRelativeY));

    } else {
        std::vector<IconInfo> tempTop;
        std::vector<IconInfo> tempBottom;

        if (_gameStats.age == 1) {
            tempTop.push_back(IconInfo(icon_placeholder_1, action1));
            tempTop.push_back(IconInfo(icon_placeholder_2, action2));
            tempTop.push_back(IconInfo(icon_placeholder_3, action3));
            tempBottom.push_back(IconInfo(icon_stats, statsAction));
            tempBottom.push_back(IconInfo(icon_parameters, paramsAction));
            tempBottom.push_back(IconInfo(icon_placeholder_4, sleepAction)); 
        } else { // Age 2+
            tempTop.push_back(IconInfo(icon_placeholder_1, action1));
            tempTop.push_back(IconInfo(icon_placeholder_2, action2));
            tempTop.push_back(IconInfo(icon_placeholder_3, action3));
            tempBottom.push_back(IconInfo(icon_stats, statsAction));
            tempBottom.push_back(IconInfo(icon_parameters, paramsAction));
            tempBottom.push_back(IconInfo(icon_placeholder_4, sleepAction)); 
            tempBottom.push_back(IconInfo(icon_placeholder_5, action5));
            tempBottom.push_back(IconInfo(icon_placeholder_6, action6));
        }

        if (!tempTop.empty()) {
            int totalTopWidth = tempTop.size() * ICON_WIDTH + (tempTop.size() - 1) * ICON_SPACING;
            int startTopX = (screenWidth - totalTopWidth) / 2;
            int currentTopX = startTopX;
            for (const auto& icon : tempTop) {
                _topIcons.push_back(IconInfo(icon.bitmap, icon.action, currentTopX, iconRelativeY));
                currentTopX += ICON_WIDTH + ICON_SPACING;
            }
        }

        if (!tempBottom.empty()) {
             int totalBottomWidth = tempBottom.size() * ICON_WIDTH + (tempBottom.size() - 1) * ICON_SPACING;
             int startBottomX = (screenWidth - totalBottomWidth) / 2;
             int currentBottomX = startBottomX;
             for (const auto& icon : tempBottom) {
                 _bottomIcons.push_back(IconInfo(icon.bitmap, icon.action, currentBottomX, iconRelativeY));
                 currentBottomX += ICON_WIDTH + ICON_SPACING;
             }
        }
    }
    selectDefaultIcon();
}

void IconMenuManager::resetSelection() { 
    selectDefaultIcon();
    resetSelectionTimeout();
}

void IconMenuManager::update() { 
    if (_selectedRow != ROW_NONE) {
        _selectionTimeoutCounter++;
        if (_selectionTimeoutCounter >= SELECTION_TIMEOUT_TICKS) {
            _selectedRow = ROW_NONE;
            _selectedIconIndex = -1;
            _selectionTimeoutCounter = 0;
            debugPrint("SCENES", "IconMenuManager: Selection timed out.");
        }
    }
}

void IconMenuManager::drawRow(MenuRowType rowType, int menuBaseScreenY) {
    const std::vector<IconInfo>* iconsToDraw = nullptr;
    if (rowType == ROW_TOP) {
        iconsToDraw = &_topIcons;
    } else if (rowType == ROW_BOTTOM) {
        iconsToDraw = &_bottomIcons;
    } else {
        return; 
    }

    if (!iconsToDraw) return;

    for (const auto& icon : *iconsToDraw) {
        drawIcon(icon.x, menuBaseScreenY + icon.y, icon.bitmap);
    }

    if (_selectedRow == rowType && _selectedIconIndex >=0 && _selectedIconIndex < (int)iconsToDraw->size()) {
        drawSelectionBoxForIcon((*iconsToDraw)[_selectedIconIndex], menuBaseScreenY);
    }
}


IconAction IconMenuManager::handleInput(MenuInputKey key) { 
    IconAction triggeredAction = IconAction::NONE;
    if (_topIcons.empty() && _bottomIcons.empty()) { _selectedRow = ROW_NONE; _selectedIconIndex = -1; return IconAction::NONE; }
    
    bool selectionJustActivated = (_selectedRow == ROW_NONE);
    if (selectionJustActivated && (key == MenuInputKey::LEFT || key == MenuInputKey::RIGHT || key == MenuInputKey::SELECT)) {
        selectDefaultIcon(); 
         debugPrintf("SCENES", "IconMenuManager: Input activated menu. Row: %d, Idx: %d", _selectedRow, _selectedIconIndex);
    } else if (_selectedRow != ROW_NONE) { 
        switch (key) {
            case MenuInputKey::LEFT:
                if (_selectedRow == ROW_TOP) { 
                    if (_selectedIconIndex > 0) { _selectedIconIndex--; } 
                    else { 
                        if (!_bottomIcons.empty()) { _selectedRow = ROW_BOTTOM; _selectedIconIndex = _bottomIcons.size() - 1; } 
                        else { _selectedIconIndex = _topIcons.empty() ? -1 : _topIcons.size() - 1; } 
                    }
                } else { 
                    if (_selectedIconIndex > 0) { _selectedIconIndex--; } 
                    else { 
                        if (!_topIcons.empty()) { _selectedRow = ROW_TOP; _selectedIconIndex = _topIcons.size() - 1; } 
                        else { _selectedIconIndex = _bottomIcons.empty() ? -1 : _bottomIcons.size() - 1; } 
                    }
                }
                break;
            case MenuInputKey::RIGHT:
                 if (_selectedRow == ROW_TOP) { 
                     const std::vector<IconInfo>& currentIcons = _topIcons; 
                     if (!currentIcons.empty() && _selectedIconIndex < (int)currentIcons.size() - 1) { _selectedIconIndex++; } 
                     else { 
                         if (!_bottomIcons.empty()) { _selectedRow = ROW_BOTTOM; _selectedIconIndex = 0; } 
                         else { _selectedIconIndex = 0; } 
                     }
                 } else { 
                     const std::vector<IconInfo>& currentIcons = _bottomIcons; 
                     if (!currentIcons.empty() && _selectedIconIndex < (int)currentIcons.size() - 1) { _selectedIconIndex++; } 
                     else { 
                         if (!_topIcons.empty()) { _selectedRow = ROW_TOP; _selectedIconIndex = 0; } 
                         else { _selectedIconIndex = 0; } 
                     }
                 }
                break;
            case MenuInputKey::SELECT: {
                    const std::vector<IconInfo>* currentIconsPtr = nullptr;
                    if (_selectedRow == ROW_TOP && !_topIcons.empty() && _selectedIconIndex >= 0 && _selectedIconIndex < (int)_topIcons.size()) { 
                        currentIconsPtr = &_topIcons; 
                    } else if (_selectedRow == ROW_BOTTOM && !_bottomIcons.empty() && _selectedIconIndex >= 0 && _selectedIconIndex < (int)_bottomIcons.size()) { 
                        currentIconsPtr = &_bottomIcons; 
                    }
                    if (currentIconsPtr) { 
                        const auto& icon = (*currentIconsPtr)[_selectedIconIndex]; 
                        triggeredAction = icon.action; 
                        debugPrintf("SCENES", "IconMenuManager: Icon selected, returning action %d", (int)triggeredAction); 
                    } else { 
                        debugPrintf("SCENES", "IconMenuManager: Select pressed but no valid icon selected (Row: %d, Idx: %d).", _selectedRow, _selectedIconIndex); 
                        triggeredAction = IconAction::NONE; 
                    }
                } break;
        }
    }
    if (_selectedRow != ROW_NONE) { 
      resetSelectionTimeout();
    }
    return triggeredAction;
}

void IconMenuManager::drawIcon(int screenX, int screenY, const unsigned char* bitmap) { 
    if (screenX >= -ICON_WIDTH && screenX < _renderer.getWidth() && 
        screenY >= -ICON_HEIGHT && screenY < _renderer.getHeight() && 
        bitmap != nullptr) { 
        U8G2* u8g2 = _renderer.getU8G2(); 
        if (u8g2) { 
            u8g2->drawXBMP(_renderer.getXOffset() + screenX, _renderer.getYOffset() + screenY, ICON_WIDTH, ICON_HEIGHT, bitmap); 
        } 
    }
}

void IconMenuManager::drawSelectionBoxForIcon(const IconInfo& icon, int menuBaseScreenY) { 
    int iconScreenX = icon.x;
    int iconScreenY = menuBaseScreenY + icon.y; 

    if (iconScreenX != -1 && iconScreenY != -1) { 
        int selectorX = iconScreenX - (SELECTOR_SIZE - ICON_WIDTH) / 2; 
        int selectorY = iconScreenY - (SELECTOR_SIZE - ICON_HEIGHT) / 2; 
        _renderer.drawRectangle(selectorX, selectorY, SELECTOR_SIZE, SELECTOR_SIZE); 
    }
}

void IconMenuManager::resetSelectionTimeout() { 
    _selectionTimeoutCounter = 0;
}

void IconMenuManager::selectDefaultIcon() { 
    if (!_topIcons.empty()) { _selectedRow = ROW_TOP; _selectedIconIndex = 0; }
    else if (!_bottomIcons.empty()) { _selectedRow = ROW_BOTTOM; _selectedIconIndex = 0; }
    else { _selectedRow = ROW_NONE; _selectedIconIndex = -1; }
}
// --- END OF FILE src/SceneMain/IconMenuManager.cpp ---