#ifndef GAME_SCENE_H
#define GAME_SCENE_H

#include "Scene.h" 
#include "../../DialogBox/DialogBox.h"
#include <memory> 
#include "../../System/GameContext.h" 

// Forward Declarations
class Renderer;

class GameScene : public Scene {
public:
    GameScene();
    ~GameScene() override;

    // This init is specific to GameScene and its children, it's not overriding the base Scene::init()
    virtual void init(GameContext& context); 
    void onEnter() override;
    void onExit() override;

    void update(unsigned long deltaTime) override;
    void draw(Renderer& renderer) override;

    bool isFatigueDialogActive() const;
    // The base GameScene now owns the dialog and handles key presses for it
    DialogBox* getDialogBox() override { return _fatigueWarningDialog.get(); } 
    bool handleDialogKeyPress(uint8_t keyCode);

protected:
    // GameScene and its derivatives now store their own context pointer.
    GameContext* _gameContext = nullptr;

    void signalGameExit(bool completedSuccessfully = true, uint32_t pointsEarned = 0);
    virtual void onGamePausedByFatigue();
    virtual void onGameResumedFromFatiguePause();
    bool isGamePausedByFatigue() const { return _isPausedByFatigue; }
    unsigned long getGameDurationMs() const;

private:
    unsigned long _gameStartTimeMs;
    unsigned long _lastFatigueUpdateTimeMs;
    
    static const unsigned long FATIGUE_INCREASE_INTERVAL_MS = 30000; 
    static const uint8_t FATIGUE_PER_INTERVAL = 3;                  
    static const uint8_t FATIGUE_WARNING_LOW_THRESHOLD = 75;       
    static const uint8_t FATIGUE_WARNING_HIGH_THRESHOLD = 90;      
    static const uint8_t FATIGUE_MAX_EXIT_THRESHOLD = 100;        

    bool _isPausedByFatigue;
    bool _lowFatigueWarningShown;
    bool _highFatigueWarningShown;

    std::unique_ptr<DialogBox> _fatigueWarningDialog;

    void handleFatigueManagement(unsigned long currentTime);
    void showFatigueWarningDialog(uint8_t currentFatigue);
};

#endif // GAME_SCENE_H