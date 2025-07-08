#ifndef LANGUAGE_SELECT_SCENE_H
#define LANGUAGE_SELECT_SCENE_H

#include "Scene.h"
#include "GEM_u8g2.h"
#include "GEMSelect.h"
#include "espasyncbutton.hpp"
#include <memory>
#include "Localization.h" 
#include "../../System/GameContext.h" 

// Forward Declarations
class U8G2;

class _0LanguageSelectScene : public Scene {
public:
    _0LanguageSelectScene();
    ~_0LanguageSelectScene() override;

    // This is a scene-specific init method, not an override of the base class.
    void init(GameContext& context); 
    void onEnter() override;
    void onExit() override;
    void update(unsigned long deltaTime) override;
    void draw(Renderer& renderer) override; // Renderer& stays for drawing

    bool usesKeyQueue() const override { return true; }
    void processKeyPress(uint8_t keyCode) override;

private:
    std::unique_ptr<GEM_u8g2> menu;
    U8G2* u8g2_ptr = nullptr; // Still used by GEM

    GEMPage menuPageLanguage;
    GEMSelect languageSelect;
    Language languageVariable; 
    GEMItem menuItemLanguageSelect;
    GEMItem menuItemConfirm; 

    static _0LanguageSelectScene* currentInstance; 

    // This scene needs access to GameContext to save language settings
    GameContext* _gameContext = nullptr; 

    void onButtonOkLongPress(const EventMsg* msg); 

    void onLanguageConfirm();
    static void onLanguageConfirmWrapper();
};

#endif // LANGUAGE_SELECT_SCENE_H