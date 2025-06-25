#ifndef SCENE_ACTION_H
#define SCENE_ACTION_H

#include "Scene.h" 
#include <vector>
#include <memory>           
#include "ActionGraphics.h" 
#include "espasyncbutton.hpp" 
#include "Localization.h" 
#include "../../DialogBox/DialogBox.h" 
#include "../../System/GameContext.h" 

// Forward declarations
class Renderer;
class Animator;

enum class ActionMenuContext {
    MAIN,
    CLEANING,
    MEDICINE
};

enum class GridAction {
    NONE,
    GOTO_CLEANING_MENU,
    GOTO_MEDICINE_MENU,
    GOTO_PLAY_MENU,      
    FEEDING,
    PLAYING,             
    SLEEP,
    BACK_TO_MAIN, 
    CLEANING_DOUCHE,
    CLEANING_TOILET,
    CLEANING_BROSSE,
    CLEANING_PAPIER,
    CLEANING_DENTIFRICE,
    CLEANING_SERVIETTE,
    MEDICINE_FIOLE,
    MEDICINE_SERINGUE,
    MEDICINE_PANSEMENT,
    MEDICINE_BANDAGE,
    MEDICINE_GELULE,
    MEDICINE_THERMOMETRE,
    PLACEHOLDER_5, 
    PLACEHOLDER_6,
    PLACEHOLDER_7,
    PLACEHOLDER_8
};

struct ActionGridItem {
    const unsigned char* bitmap = nullptr; 
    GridAction action = GridAction::NONE; 

    ActionGridItem(const unsigned char* bmp = nullptr, GridAction act = GridAction::NONE)
        : bitmap(bmp), action(act) {}
};


class SceneAction : public Scene {
public:
    SceneAction();
    ~SceneAction() override; 

    // This is a scene-specific init, not an override of the base class.
    void init(GameContext& context); 
    void onEnter() override;
    void onExit() override;
    void update(unsigned long deltaTime) override;
    void draw(Renderer& renderer) override; 

    SceneType getSceneType() const override { return SceneType::ACTION; } 
    DialogBox* getDialogBox() override { return _dialogBox.get(); } 


private:
    static const int GRID_COLS = 4;
    static const int GRID_ROWS = 2;
    static const int GRID_ICON_WIDTH = ACTION_ICON_WIDTH; 
    static const int GRID_ICON_HEIGHT = ACTION_ICON_HEIGHT; 
    static const int GRID_TOTAL_ICONS = GRID_COLS * GRID_ROWS;

    ActionMenuContext _currentContext = ActionMenuContext::MAIN; 
    ActionGridItem _gridItems[GRID_TOTAL_ICONS];                 
    int _selectedCol = 0;
    int _selectedRow = 0;
    StringKey _hoveredItemNameKey = StringKey::ACTION_TITLE; 

    std::unique_ptr<Animator> _clickAnimation; 
    GridAction _pendingAction;                 
    int _animatingIconIndex;                   

    std::unique_ptr<DialogBox> _dialogBox; 

    // Scene-specific context pointer
    GameContext* _gameContext = nullptr;

    void setupGridForContext(ActionMenuContext context); 
    void performGridAction(GridAction action);           
    void updateHoveredItemName();                        
    StringKey getActionNameKey(GridAction action);       

    void doCleaningDouche();
    bool doCleaningToilet(); 
    void doCleaningBrosse();
    void doCleaningPapier();
    void doCleaningDentifrice();
    void doCleaningServiette();
    void doMedicineFiole();
    void doMedicineSeringue();
    void doMedicinePansement();
    void doMedicineBandage();
    void doMedicineGelule();
    void doMedicineThermometre();

    void onNavLeft();
    void onNavRight();
    void onNavUp();
    void onNavDown();
    void onSelect();
    void onLongPressExit(); 
    bool handleDialogKeyPress(uint8_t keyCode);


    void drawGrid(Renderer& renderer);
    void drawSelector(Renderer& renderer);
    void drawHoverText(Renderer& renderer);
};

#endif // SCENE_ACTION_H