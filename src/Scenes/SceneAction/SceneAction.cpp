#include "SceneAction.h"
#include "SceneManager.h" 
#include "InputManager.h"
#include "Renderer.h"
#include <SerialForwarder.h>
#include <U8g2lib.h>
#include "ActionGraphics.h"
#include "GameStats.h"
#include "Localization.h"
#include <algorithm>
#include <cstring>
#include "Animator.h"
#include "../../DialogBox/DialogBox.h"
#include "../../DebugUtils.h"
#include "GEM_u8g2.h"
#include "../../System/GameContext.h"


SceneAction::SceneAction() : _currentContext(ActionMenuContext::MAIN),
                             _selectedCol(0),
                             _selectedRow(0),
                             _hoveredItemNameKey(StringKey::ACTION_CLEANING),
                             _clickAnimation(nullptr),
                             _pendingAction(GridAction::NONE),
                             _animatingIconIndex(-1)
{
}

SceneAction::~SceneAction()
{
    debugPrint("SCENES", "SceneAction destructor called.");
}

void SceneAction::init(GameContext &context)
{
    Scene::init();
    _gameContext = &context;
    debugPrint("SCENES", "SceneAction::init");

    if (!_gameContext || !_gameContext->renderer)
    {
        debugPrint("SCENES", "ERROR: SceneAction::init - Critical context members (renderer) are null!");
        return;
    }
    _dialogBox.reset(new DialogBox(*_gameContext->renderer));

    // Register for abstract engine buttons and events
    _gameContext->inputManager->registerButtonListener(EDGE_Button::LEFT, EDGE_Event::CLICK, this, [this]()
                          { 
        if (handleDialogKeyPress(GEM_KEY_UP)) { return; } 
        this->onNavLeft(); });
    _gameContext->inputManager->registerButtonListener(EDGE_Button::RIGHT, EDGE_Event::CLICK, this, [this]()
                          { 
        if (handleDialogKeyPress(GEM_KEY_DOWN)) { return; }
        this->onNavRight(); });
    _gameContext->inputManager->registerButtonListener(EDGE_Button::OK, EDGE_Event::CLICK, this, [this]()
                          { 
        if (handleDialogKeyPress(GEM_KEY_OK)) { return; }
        this->onSelect(); });
    _gameContext->inputManager->registerButtonListener(EDGE_Button::OK, EDGE_Event::LONG_PRESS, this, [this]()
                          { 
        if (handleDialogKeyPress(GEM_KEY_CANCEL)) { return; }
        this->onLongPressExit(); });
}

void SceneAction::onEnter()
{
    debugPrint("SCENES", "SceneAction::onEnter");
    _currentContext = ActionMenuContext::MAIN;
    setupGridForContext(_currentContext);
    _selectedCol = 0;
    _selectedRow = 0;
    updateHoveredItemName();
    _clickAnimation.reset();
    _pendingAction = GridAction::NONE;
    _animatingIconIndex = -1;
    if (_dialogBox)
        _dialogBox->close();
}

void SceneAction::onExit()
{
    debugPrint("SCENES", "SceneAction::onExit");
    _currentContext = ActionMenuContext::MAIN;
    _clickAnimation.reset();
    _pendingAction = GridAction::NONE;
    _animatingIconIndex = -1;
    if (_dialogBox)
        _dialogBox->close();
    if (_gameContext && _gameContext->inputManager) {
        _gameContext->inputManager->unregisterAllListenersForScene(this);
    }
}

void SceneAction::update(unsigned long deltaTime)
{
    unsigned long currentTime = millis();

    if (_dialogBox && _dialogBox->isActive())
    {
        _dialogBox->update(currentTime);
    }

    if (_clickAnimation)
    {
        bool stillAnimating = _clickAnimation->update();
        if (!stillAnimating)
        {
            debugPrintf("SCENES", "Click animation finished for action %d", (int)_pendingAction);
            GridAction completedAction = _pendingAction;
            _pendingAction = GridAction::NONE;
            _clickAnimation.reset();
            _animatingIconIndex = -1;

            if (completedAction == GridAction::GOTO_CLEANING_MENU)
            {
                _currentContext = ActionMenuContext::CLEANING;
                setupGridForContext(_currentContext);
                _selectedCol = 0;
                _selectedRow = 0;
                updateHoveredItemName();
            }
            else if (completedAction == GridAction::GOTO_MEDICINE_MENU)
            {
                _currentContext = ActionMenuContext::MEDICINE;
                setupGridForContext(_currentContext);
                _selectedCol = 0;
                _selectedRow = 0;
                updateHoveredItemName();
            }
            else if (completedAction == GridAction::BACK_TO_MAIN)
            {
                _currentContext = ActionMenuContext::MAIN;
                setupGridForContext(_currentContext);
                _selectedCol = 0;
                _selectedRow = 0;
                updateHoveredItemName();
            }
            else if (completedAction != GridAction::NONE)
            {
                performGridAction(completedAction);
            }

            updateHoveredItemName();
        }
    }
}

void SceneAction::draw(Renderer &renderer)
{ 
    drawGrid(renderer);
    if (_clickAnimation)
    {
        _clickAnimation->draw();
    }
    else
    {
        drawSelector(renderer);
    }
    drawHoverText(renderer);
    if (_dialogBox && _dialogBox->isActive())
    {
        _dialogBox->draw();
    }
}

void SceneAction::setupGridForContext(ActionMenuContext context)
{
    for (int i = 0; i < GRID_TOTAL_ICONS; ++i)
    {
        _gridItems[i] = ActionGridItem(nullptr, GridAction::NONE);
    }
    switch (context)
    {
    case ActionMenuContext::MAIN:
        _gridItems[0] = {bmp_Action_Brosse, GridAction::GOTO_CLEANING_MENU};
        _gridItems[1] = {bmp_Action_Medicine, GridAction::GOTO_MEDICINE_MENU};
        _gridItems[2] = {bmp_Action_Feed, GridAction::FEEDING};
        _gridItems[3] = {bmp_Action_Play, GridAction::GOTO_PLAY_MENU};
        _gridItems[4] = {bmp_Action_Sleep, GridAction::SLEEP};
        _gridItems[5] = {epd_bitmap_Unknown, GridAction::PLACEHOLDER_5};
        _gridItems[6] = {epd_bitmap_Unknown, GridAction::PLACEHOLDER_6};
        _gridItems[7] = {epd_bitmap_Unknown, GridAction::PLACEHOLDER_7};
        break;
    case ActionMenuContext::CLEANING:
        _gridItems[0] = {bmp_Cleaning_Douche, GridAction::CLEANING_DOUCHE};
        _gridItems[1] = {bmp_Cleaning_Toilet, GridAction::CLEANING_TOILET};
        _gridItems[2] = {bmp_Cleaning_Brosse, GridAction::CLEANING_BROSSE};
        _gridItems[3] = {bmp_Cleaning_Papier, GridAction::CLEANING_PAPIER};
        _gridItems[4] = {bmp_Cleaning_Dentifrice, GridAction::CLEANING_DENTIFRICE};
        _gridItems[5] = {bmp_Cleaning_Serviette, GridAction::CLEANING_SERVIETTE};
        _gridItems[7] = {bmp_Return, GridAction::BACK_TO_MAIN};
        break;
    case ActionMenuContext::MEDICINE:
        _gridItems[0] = {bmp_Medicine_Fiole, GridAction::MEDICINE_FIOLE};
        _gridItems[1] = {bmp_Medicine_Seringue, GridAction::MEDICINE_SERINGUE};
        _gridItems[2] = {bmp_Medicine_Pansement, GridAction::MEDICINE_PANSEMENT};
        _gridItems[3] = {bmp_Medicine_Bandage, GridAction::MEDICINE_BANDAGE};
        _gridItems[4] = {bmp_Medicine_Gelule, GridAction::MEDICINE_GELULE};
        _gridItems[5] = {bmp_Medicine_Thermometre, GridAction::MEDICINE_THERMOMETRE};
        _gridItems[7] = {bmp_Return, GridAction::BACK_TO_MAIN};
        break;
    }
    updateHoveredItemName();
}

void SceneAction::onNavLeft()
{
    if (_clickAnimation)
        return;
    if (_dialogBox && _dialogBox->isActive())
        return;
    _selectedCol--;
    if (_selectedCol < 0)
    {
        _selectedCol = GRID_COLS - 1;
        _selectedRow--;
        if (_selectedRow < 0)
        {
            _selectedRow = GRID_ROWS - 1;
        }
    }
    updateHoveredItemName();
}

void SceneAction::onNavRight()
{
    if (_clickAnimation)
        return;
    if (_dialogBox && _dialogBox->isActive())
        return;
    _selectedCol++;
    if (_selectedCol >= GRID_COLS)
    {
        _selectedCol = 0;
        _selectedRow++;
        if (_selectedRow >= GRID_ROWS)
        {
            _selectedRow = 0;
        }
    }
    updateHoveredItemName();
}

void SceneAction::onNavUp()
{
    if (_clickAnimation)
        return;
    if (_dialogBox && _dialogBox->isActive())
        return;
    _selectedRow--;
    if (_selectedRow < 0)
    {
        _selectedRow = GRID_ROWS - 1;
    }
    updateHoveredItemName();
}

void SceneAction::onNavDown()
{
    if (_clickAnimation)
        return;
    if (_dialogBox && _dialogBox->isActive())
        return;
    _selectedRow++;
    if (_selectedRow >= GRID_ROWS)
    {
        _selectedRow = 0;
    }
    updateHoveredItemName();
}

void SceneAction::onSelect()
{
    if (_clickAnimation)
        return;
    if (_dialogBox && _dialogBox->isActive())
        return;

    int selectedIndex = _selectedRow * GRID_COLS + _selectedCol;
    if (selectedIndex < 0 || selectedIndex >= GRID_TOTAL_ICONS)
        return;
    ActionGridItem &selectedItem = _gridItems[selectedIndex];
    if (selectedItem.action == GridAction::NONE || selectedItem.bitmap == nullptr)
        return;
    debugPrintf("SCENES", "Action Selected: Row=%d, Col=%d -> Action=%d", _selectedRow, _selectedCol, (int)selectedItem.action);
    _pendingAction = selectedItem.action;
    _animatingIconIndex = selectedIndex;
    int iconX = _selectedCol * GRID_ICON_WIDTH;
    int iconY = _selectedRow * GRID_ICON_HEIGHT;
    const unsigned long ANIM_DURATION_MS = 80;
    const float START_SCALE = 1.0f;
    const float END_SCALE = 1.2f;

    if (_gameContext && _gameContext->renderer)
    { 
        _clickAnimation.reset(new Animator(*_gameContext->renderer, selectedItem.bitmap, GRID_ICON_WIDTH, GRID_ICON_HEIGHT, iconX, iconY, START_SCALE, END_SCALE, ANIM_DURATION_MS, 1, true, 0));
        debugPrintf("SCENES", "Starting SCALE click animation for action %d at index %d", (int)_pendingAction, _animatingIconIndex);
    }
    else
    {
        debugPrint("SCENES", "Error: GameContext or Renderer null, cannot create click animation. Performing action directly.");
        if (_pendingAction == GridAction::GOTO_CLEANING_MENU)
        {
            _currentContext = ActionMenuContext::CLEANING;
            setupGridForContext(_currentContext);
            _selectedCol = 0;
            _selectedRow = 0;
            updateHoveredItemName();
        }
        else if (_pendingAction == GridAction::GOTO_MEDICINE_MENU)
        {
            _currentContext = ActionMenuContext::MEDICINE;
            setupGridForContext(_currentContext);
            _selectedCol = 0;
            _selectedRow = 0;
            updateHoveredItemName();
        }
        else if (_pendingAction == GridAction::BACK_TO_MAIN)
        {
            _currentContext = ActionMenuContext::MAIN;
            setupGridForContext(_currentContext);
            _selectedCol = 0;
            _selectedRow = 0;
            updateHoveredItemName();
        }
        else if (_pendingAction != GridAction::NONE)
        {
            performGridAction(_pendingAction);
        }
        _pendingAction = GridAction::NONE;
        _animatingIconIndex = -1;
    }
}

void SceneAction::onLongPressExit()
{
    if (_clickAnimation)
    {
        debugPrint("SCENES", "SceneAction: Long press ignored during click animation.");
        return;
    }
    if (_dialogBox && _dialogBox->isActive())
        return;

    if (_gameContext && _gameContext->sceneManager)
    { 
        if (_currentContext != ActionMenuContext::MAIN)
        {
            debugPrint("SCENES", "SceneAction: Long press exit from submenu -> MAIN ACTION MENU.");
            _currentContext = ActionMenuContext::MAIN;
            setupGridForContext(_currentContext);
            _selectedCol = 0;
            _selectedRow = 0;
            updateHoveredItemName();
        }
        else
        {
            debugPrint("SCENES", "SceneAction: Long press exit from main -> MAIN SCENE.");
            _gameContext->sceneManager->requestSetCurrentScene("MAIN");
        }
    }
}

void SceneAction::performGridAction(GridAction action)
{
    bool stateChanged = false;
    if (!_gameContext || !_gameContext->gameStats || !_dialogBox || !_gameContext->sceneManager)
    {
        debugPrint("SCENES", "Error: Critical context members unavailable for action.");
        return;
    }
    GameStats *gameStats = _gameContext->gameStats;

    switch (action)
    {
    case GridAction::GOTO_PLAY_MENU:
        debugPrint("SCENES", "ACTION: Go to Play Menu selected.");
        _gameContext->sceneManager->requestPushScene("PLAY_MENU");
        break;
    case GridAction::FEEDING:
        debugPrint("SCENES", "ACTION: Feeding (Not implemented)");
        _dialogBox->showTemporary("Feeding...");
        break;
    case GridAction::PLAYING:
        debugPrint("SCENES", "ACTION: Playing (Now likely superseded by Play Menu)");
        _dialogBox->showTemporary("Playing!");
        break;
    case GridAction::SLEEP:
        if (gameStats->isSleeping)
        {
            debugPrint("SCENES", "ACTION: Cannot sleep, already sleeping.");
            _dialogBox->showTemporary(loc(StringKey::DIALOG_ALREADY_SLEEPING));
        }
        else if (gameStats->fatigue >= SLEEP_FATIGUE_THRESHOLD)
        {
            debugPrint("SCENES", "ACTION: Sleep triggered.");
            gameStats->setIsSleeping(true);
            stateChanged = true;
            _gameContext->sceneManager->requestSetCurrentScene("MAIN");
        }
        else
        {
            debugPrint("SCENES", "ACTION: Cannot sleep: Not tired enough.");
            _dialogBox->showTemporary(loc(StringKey::DIALOG_NOT_TIRED_ENOUGH));
        }
        break;
    case GridAction::CLEANING_DOUCHE:
        doCleaningDouche();
        stateChanged = true;
        _dialogBox->showTemporary("Cleaned!");
        break;
    case GridAction::CLEANING_TOILET:
        stateChanged = doCleaningToilet();
        if (stateChanged)
            _dialogBox->showTemporary("Poop cleaned!");
        break;
    case GridAction::CLEANING_BROSSE:
        doCleaningBrosse();
        stateChanged = true;
        _dialogBox->showTemporary("Brushed!");
        break;
    case GridAction::CLEANING_PAPIER:
        doCleaningPapier();
        stateChanged = true;
        _dialogBox->showTemporary("Wiped!");
        break;
    case GridAction::CLEANING_DENTIFRICE:
        doCleaningDentifrice();
        stateChanged = true;
        _dialogBox->showTemporary("Teeth brushed!");
        break;
    case GridAction::CLEANING_SERVIETTE:
        doCleaningServiette();
        stateChanged = true;
        _dialogBox->showTemporary("Dried!");
        break;
    case GridAction::MEDICINE_FIOLE:
        doMedicineFiole();
        stateChanged = true;
        break;
    case GridAction::MEDICINE_SERINGUE:
        doMedicineSeringue();
        stateChanged = true;
        break;
    case GridAction::MEDICINE_PANSEMENT:
        doMedicinePansement();
        stateChanged = true;
        _dialogBox->showTemporary("Ouch!");
        break;
    case GridAction::MEDICINE_BANDAGE:
        doMedicineBandage();
        stateChanged = true;
        _dialogBox->showTemporary("All wrapped up!");
        break;
    case GridAction::MEDICINE_GELULE:
        doMedicineGelule();
        stateChanged = true;
        break;
    case GridAction::MEDICINE_THERMOMETRE:
        doMedicineThermometre();
        stateChanged = true;
        _dialogBox->showTemporary("Checking...");
        break;
    case GridAction::PLACEHOLDER_5:
        debugPrint("SCENES", "ACTION: Placeholder 5 selected");
        _dialogBox->showTemporary("Action 5");
        break;
    case GridAction::PLACEHOLDER_6:
        debugPrint("SCENES", "ACTION: Placeholder 6 selected");
        _dialogBox->showTemporary("Action 6");
        break;
    case GridAction::PLACEHOLDER_7:
        debugPrint("SCENES", "ACTION: Placeholder 7 selected");
        _dialogBox->showTemporary("Action 7");
        break;
    case GridAction::PLACEHOLDER_8:
        debugPrint("SCENES", "ACTION: Placeholder 8 selected");
        _dialogBox->showTemporary("Action 8");
        break;
    case GridAction::NONE:
    case GridAction::GOTO_CLEANING_MENU:
    case GridAction::GOTO_MEDICINE_MENU:
    case GridAction::BACK_TO_MAIN:
    default:
        if (action != GridAction::NONE)
        {
            debugPrintf("SCENES", "ACTION: Invalid action %d passed to performGridAction.", (int)action);
        }
        break;
    }

    if (stateChanged)
    {
        gameStats->save();
    }
}

void SceneAction::doCleaningDouche()
{
    debugPrint("SCENES", "Cleaning: Douche selected (Shower)");
    if (_gameContext && _gameContext->gameStats)
    {
        _gameContext->gameStats->setDirty(std::max(0, (int)_gameContext->gameStats->dirty - 30));
        _gameContext->gameStats->setHappiness(std::min((uint8_t)100, (uint8_t)(_gameContext->gameStats->happiness + 5)));
        debugPrint("SCENES", "Shower given!");
    }
}
bool SceneAction::doCleaningToilet()
{
    debugPrint("SCENES", "Cleaning: Toilet selected");
    if (_gameContext && _gameContext->gameStats && _gameContext->gameStats->poopCount > 0)
    {
        _gameContext->gameStats->setPoopCount(0);
        _gameContext->gameStats->setDirty(std::max(0, (int)_gameContext->gameStats->dirty - 50));
        debugPrint("SCENES", "Poop cleaned via Toilet!");
        return true;
    }
    else
    {
        debugPrint("SCENES", "No poop to clean or GameStats (via context) unavailable.");
        if (_dialogBox)
            _dialogBox->showTemporary("No poop!");
        return false;
    }
}
void SceneAction::doCleaningBrosse() { debugPrint("SCENES", "Cleaning: Brosse (Brush) - Action TBD"); }
void SceneAction::doCleaningPapier() { debugPrint("SCENES", "Cleaning: Papier (Paper) - Action TBD"); }
void SceneAction::doCleaningDentifrice() { debugPrint("SCENES", "Cleaning: Dentifrice (Toothpaste) - Action TBD"); }
void SceneAction::doCleaningServiette() { debugPrint("SCENES", "Cleaning: Serviette (Towel) - Action TBD"); }

void SceneAction::doMedicineFiole()
{
    debugPrint("SCENES", "Medicine: Fiole selected");
    if (_gameContext && _gameContext->gameStats)
    {
        GameStats *gs = _gameContext->gameStats;
        if (gs->sickness == Sickness::VOMIT || gs->sickness == Sickness::DIARRHEA)
        {
            gs->setSickness(Sickness::NONE);
            gs->setHealth(std::min((uint8_t)100, (uint8_t)(gs->health + 15)));
            debugPrint("SCENES", "Vial administered, sickness cured.");
            if (_dialogBox)
                _dialogBox->showTemporary("Cured!");
        }
        else
        {
            debugPrint("SCENES", "Vial had no effect.");
            if (_dialogBox)
                _dialogBox->showTemporary("No effect!");
        }
    }
}
void SceneAction::doMedicineSeringue()
{
    debugPrint("SCENES", "Medicine: Seringue selected");
    if (_gameContext && _gameContext->gameStats)
    {
        GameStats *gs = _gameContext->gameStats;
        if (gs->sickness == Sickness::COLD || gs->sickness == Sickness::HOT)
        {
            gs->setSickness(Sickness::NONE);
            gs->setHealth(std::min((uint8_t)100, (uint8_t)(gs->health + 20)));
            debugPrint("SCENES", "Syringe administered, sickness cured.");
            if (_dialogBox)
                _dialogBox->showTemporary("Cured!");
        }
        else
        {
            debugPrint("SCENES", "Syringe had no effect.");
            if (_dialogBox)
                _dialogBox->showTemporary("No effect!");
        }
    }
}
void SceneAction::doMedicinePansement()
{
    debugPrint("SCENES", "Medicine: Pansement (Band-Aid) - Action TBD");
    if (_dialogBox)
        _dialogBox->showTemporary("Ouch!");
}
void SceneAction::doMedicineBandage()
{
    debugPrint("SCENES", "Medicine: Bandage - Action TBD");
    if (_dialogBox)
        _dialogBox->showTemporary("All wrapped up!");
}
void SceneAction::doMedicineGelule()
{
    debugPrint("SCENES", "Medicine: Gelule selected");
    if (_gameContext && _gameContext->gameStats)
    {
        GameStats *gs = _gameContext->gameStats;
        if (gs->sickness == Sickness::HEADACHE)
        {
            gs->setSickness(Sickness::NONE);
            gs->setHealth(std::min((uint8_t)100, (uint8_t)(gs->health + 5)));
            gs->setHappiness(std::min((uint8_t)100, (uint8_t)(gs->happiness + 10)));
            debugPrint("SCENES", "Pill administered, headache cured.");
            if (_dialogBox)
                _dialogBox->showTemporary("Headache gone!");
        }
        else
        {
            debugPrint("SCENES", "Pill had no effect.");
            if (_dialogBox)
                _dialogBox->showTemporary("No effect!");
        }
    }
}
void SceneAction::doMedicineThermometre()
{
    debugPrint("SCENES", "Medicine: Thermometre - Action TBD");
    if (_dialogBox)
        _dialogBox->showTemporary("Checking...");
}

StringKey SceneAction::getActionNameKey(GridAction action)
{
    switch (action)
    {
    case GridAction::GOTO_CLEANING_MENU:
        return StringKey::ACTION_CLEANING;
    case GridAction::GOTO_MEDICINE_MENU:
        return StringKey::ACTION_MEDICINE;
    case GridAction::GOTO_PLAY_MENU:
        return StringKey::ACTION_PLAYING;
    case GridAction::FEEDING:
        return StringKey::ACTION_FEEDING;
    case GridAction::PLAYING:
        return StringKey::ACTION_PLAYING;
    case GridAction::SLEEP:
        return StringKey::ACTION_SLEEP;
    case GridAction::CLEANING_DOUCHE:
        return StringKey::CLEANING_DOUCHE;
    case GridAction::CLEANING_TOILET:
        return StringKey::CLEANING_TOILET;
    case GridAction::CLEANING_BROSSE:
        return StringKey::CLEANING_BROSSE;
    case GridAction::CLEANING_PAPIER:
        return StringKey::CLEANING_PAPIER;
    case GridAction::CLEANING_DENTIFRICE:
        return StringKey::CLEANING_DENTIFRICE;
    case GridAction::CLEANING_SERVIETTE:
        return StringKey::CLEANING_SERVIETTE;
    case GridAction::MEDICINE_FIOLE:
        return StringKey::MEDICINE_FIOLE;
    case GridAction::MEDICINE_SERINGUE:
        return StringKey::MEDICINE_SERINGUE;
    case GridAction::MEDICINE_PANSEMENT:
        return StringKey::MEDICINE_PANSEMENT;
    case GridAction::MEDICINE_BANDAGE:
        return StringKey::MEDICINE_BANDAGE;
    case GridAction::MEDICINE_GELULE:
        return StringKey::MEDICINE_GELULE;
    case GridAction::MEDICINE_THERMOMETRE:
        return StringKey::MEDICINE_THERMOMETRE;
    case GridAction::BACK_TO_MAIN:
        return StringKey::HINT_EXIT;
    case GridAction::PLACEHOLDER_5:
        return StringKey::ACTION_PLACEHOLDER5;
    case GridAction::PLACEHOLDER_6:
        return StringKey::ACTION_PLACEHOLDER6;
    case GridAction::PLACEHOLDER_7:
        return StringKey::ACTION_PLACEHOLDER7;
    case GridAction::PLACEHOLDER_8:
        return StringKey::ACTION_TITLE;
    case GridAction::NONE:
    default:
        return StringKey::ACTION_TITLE;
    }
}

void SceneAction::updateHoveredItemName()
{
    int selectedIndex = _selectedRow * GRID_COLS + _selectedCol;
    if (selectedIndex >= 0 && selectedIndex < GRID_TOTAL_ICONS)
    {
        if (_gridItems[selectedIndex].action != GridAction::NONE)
        {
            _hoveredItemNameKey = getActionNameKey(_gridItems[selectedIndex].action);
        }
        else
        {
            _hoveredItemNameKey = StringKey::ACTION_TITLE;
        }
    }
    else
    {
        _hoveredItemNameKey = StringKey::ACTION_TITLE;
    }
}

void SceneAction::drawGrid(Renderer &renderer)
{
    if (!_gameContext || !_gameContext->display)
        return;
    U8G2 *u8g2 = _gameContext->display;
    for (int r = 0; r < GRID_ROWS; ++r)
    {
        for (int c = 0; c < GRID_COLS; ++c)
        {
            int index = r * GRID_COLS + c;
            if (_clickAnimation && index == _animatingIconIndex)
            {
                continue;
            }
            int drawX = c * GRID_ICON_WIDTH;
            int drawY = r * GRID_ICON_HEIGHT;
            const unsigned char *bitmapToDraw = _gridItems[index].bitmap;
            if (bitmapToDraw == nullptr)
            {
                bitmapToDraw = epd_bitmap_Unknown;
            }
            if (bitmapToDraw != nullptr)
            {
                u8g2->drawXBMP(renderer.getXOffset() + drawX, renderer.getYOffset() + drawY, GRID_ICON_WIDTH, GRID_ICON_HEIGHT, bitmapToDraw);
            }
        }
    }
}
void SceneAction::drawSelector(Renderer &renderer)
{
    if (_clickAnimation)
        return;
    int selectorX = _selectedCol * GRID_ICON_WIDTH;
    int selectorY = _selectedRow * GRID_ICON_HEIGHT;
    int selectorSize = GRID_ICON_WIDTH;
    int thickness = 1;
    for (int i = 0; i < thickness; ++i)
    {
        renderer.drawRectangle(selectorX + i, selectorY + i, selectorSize - 2 * i, selectorSize - 2 * i);
    }
}
void SceneAction::drawHoverText(Renderer &renderer)
{
    if (!_gameContext || !_gameContext->display)
        return;
    U8G2 *u8g2 = _gameContext->display;
    const char *nameStr = loc(_hoveredItemNameKey);
    int selectedIndex = _selectedRow * GRID_COLS + _selectedCol;
    bool isEmptySlot = (selectedIndex < 0 || selectedIndex >= GRID_TOTAL_ICONS || _gridItems[selectedIndex].action == GridAction::NONE);
    if (isEmptySlot || strcmp(nameStr, "???") == 0 || strlen(nameStr) == 0 || _hoveredItemNameKey == StringKey::ACTION_TITLE)
    {
        return;
    }
    u8g2->setFont(u8g2_font_5x7_tf);
    u8g2->setFontPosTop();
    u8g2_uint_t textWidth = u8g2->getStrWidth(nameStr);
    u8g2_uint_t fontHeight = u8g2->getMaxCharHeight();
    int boxPadding = 4;
    int boxWidth = textWidth + 2 * boxPadding;
    int boxHeight = fontHeight + 2 * boxPadding;
    int boxX = renderer.getXOffset() + (renderer.getWidth() - boxWidth) / 2;
    int boxY = renderer.getYOffset() + (renderer.getHeight() - boxHeight) / 2;
    boxX = std::max(renderer.getXOffset(), boxX);
    boxY = std::max(renderer.getYOffset(), boxY);
    boxX = std::min(renderer.getXOffset() + renderer.getWidth() - boxWidth, boxX);
    boxY = std::min(renderer.getYOffset() + renderer.getHeight() - boxHeight, boxY);
    u8g2->setDrawColor(0);
    u8g2->drawBox(boxX, boxY, boxWidth, boxHeight);
    u8g2->setDrawColor(1);
    int textX = boxX + boxPadding;
    int textY = boxY + boxPadding;
    u8g2->drawStr(textX, textY, nameStr);
}

bool SceneAction::handleDialogKeyPress(uint8_t keyCode) {
    if (_dialogBox && _dialogBox->isActive()) {
        if (keyCode == GEM_KEY_OK) {
            _dialogBox->close();
            return true;
        }
    }
    return false;
}