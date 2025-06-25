#include "PlayMenuScene.h"
#include "SceneManager.h" 
#include "Renderer.h"
#include "Localization.h"
#include "SerialForwarder.h"
#include "../../DebugUtils.h"
#include "GEM_u8g2.h" 
#include "../../System/GameContext.h" 

extern QueueHandle_t keyQueue;

PlayMenuScene* PlayMenuScene::currentInstance = nullptr;

void PlayMenuScene::onFlappyBirdSelectWrapper() { if (currentInstance) currentInstance->onFlappyBirdSelect(); }
void PlayMenuScene::onBackSelectWrapper() { if (currentInstance) currentInstance->onBackSelect(); }

PlayMenuScene::PlayMenuScene() :
    menuPagePlay(""), 
    menuItemFlappyBird("", onFlappyBirdSelectWrapper),
    menuItemBack("", onBackSelectWrapper)
{
    managesOwnDrawing = true;
    currentInstance = this;
    debugPrint("SCENES", "PlayMenuScene constructor");
}

PlayMenuScene::~PlayMenuScene() {
    debugPrint("SCENES", "PlayMenuScene destructor");
    if (currentInstance == this) {
        currentInstance = nullptr;
    }
}

void PlayMenuScene::init(GameContext& context) { 
    Scene::init(); 
    _gameContext = &context;
    debugPrint("SCENES", "PlayMenuScene::init");
    currentInstance = this;

    if (!_gameContext || !_gameContext->display) {
        debugPrint("SCENES", "ERROR: PlayMenuScene::init - Critical context member (display) is null!");
        return;
    }
    u8g2_ptr = _gameContext->display; // Get U8G2 from context

    if (!menu) {
        menu.reset(new GEM_u8g2(*u8g2_ptr, GEM_POINTER_ROW, GEM_ITEMS_COUNT_AUTO));
        menu->setFontSmall(); 

        menuPagePlay.addMenuItem(menuItemFlappyBird);
        menuPagePlay.addMenuItem(menuItemBack);

        menu->setMenuPageCurrent(menuPagePlay);
        menu->hideVersion();
        menu->setSplashDelay(0);
        menu->init();
        debugPrint("SCENES", "PlayMenuScene: GEM menu initialized.");
    } else {
        menu->reInit();
        debugPrint("SCENES", "PlayMenuScene: GEM menu re-initialized.");
    }
}

void PlayMenuScene::onEnter() {
    debugPrint("SCENES", "PlayMenuScene::onEnter");
    currentInstance = this;

    if (menu) {
        menuPagePlay.setTitle(loc(StringKey::ACTION_PLAYING)); 
        menuItemFlappyBird.setTitle("Flappy Tama"); 
        menuItemBack.setTitle(loc(StringKey::HINT_EXIT)); 
        menu->drawMenu(); 
    }
}

void PlayMenuScene::onExit() {
    debugPrint("SCENES", "PlayMenuScene::onExit");
}

void PlayMenuScene::update(unsigned long deltaTime) { }

void PlayMenuScene::draw(Renderer& renderer) { 
    if (menu) {
        menu->drawMenu();
    }
}

void PlayMenuScene::processKeyPress(uint8_t keyCode) { 
    // This scene does not have a custom dialog box, so handleDialogKeyPress is not needed.
    if (menu) { menu->registerKeyPress(keyCode); }
}

void PlayMenuScene::onFlappyBirdSelect() {
    debugPrint("SCENES", "PlayMenuScene: Flappy Bird selected.");
    if (_gameContext && _gameContext->sceneManager) { 
        _gameContext->sceneManager->requestPushScene("FLAPPY_GAME");
    }
}

void PlayMenuScene::onBackSelect() {
    debugPrint("SCENES", "PlayMenuScene: Back selected.");
    if (_gameContext && _gameContext->sceneManager) { 
        _gameContext->sceneManager->requestSetCurrentScene("ACTIONS"); 
    }
}