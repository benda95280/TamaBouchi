#ifndef PLAY_MENU_SCENE_H
#define PLAY_MENU_SCENE_H

#include "Scene.h"
#include "GEM_u8g2.h"
#include "espasyncbutton.hpp" 
#include <memory> 
#include "../../System/GameContext.h" 

// Forward Declarations
class U8G2;

class PlayMenuScene : public Scene {
public:
    PlayMenuScene();
    ~PlayMenuScene() override;

    // This is a scene-specific init method, not an override of the base class.
    void init(GameContext& context); 
    void onEnter() override;
    void onExit() override;
    void update(unsigned long deltaTime) override;
    void draw(Renderer& renderer) override; 

    bool usesKeyQueue() const override { return true; }
    void processKeyPress(uint8_t keyCode) override;

private:
    std::unique_ptr<GEM_u8g2> menu;
    U8G2* u8g2_ptr = nullptr; // Still used by GEM

    GEMPage menuPagePlay;
    GEMItem menuItemFlappyTuck;
    GEMItem menuItemBack;

    static PlayMenuScene* currentInstance; 

    // Scene-specific context pointer
    GameContext* _gameContext = nullptr;

    void onFlappyTuckSelect();
    void onBackSelect();

    static void onFlappyTuckSelectWrapper();
    static void onBackSelectWrapper();
};

#endif // PLAY_MENU_SCENE_H