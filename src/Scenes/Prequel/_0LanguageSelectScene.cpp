#include "_0LanguageSelectScene.h"
#include "SceneManager.h"
#include "Renderer.h"
#include "GameStats.h" 
#include "SerialForwarder.h"
#include <GEM_u8g2.h>
#include <GEMSelect.h>
#include "../../DebugUtils.h"
#include "../../System/GameContext.h" 

// --- Remove extern GameStats*, SerialForwarder* ---
extern QueueHandle_t keyQueue; 

_0LanguageSelectScene* _0LanguageSelectScene::currentInstance = nullptr;

SelectOptionByte prequelLanguageOptions[] = {
    {"English", (byte)Language::ENGLISH},
    {"Francais", (byte)Language::FRENCH}
};
const byte prequelLanguageOptionsCount = sizeof(prequelLanguageOptions) / sizeof(prequelLanguageOptions[0]);


_0LanguageSelectScene::_0LanguageSelectScene()
    : menuPageLanguage(""), 
      languageSelect(prequelLanguageOptionsCount, prequelLanguageOptions, GEM_LOOP),
      menuItemLanguageSelect("", reinterpret_cast<byte&>(languageVariable), languageSelect, onLanguageConfirmWrapper), 
      menuItemConfirm("", onLanguageConfirmWrapper) 
{
    managesOwnDrawing = true; 
    currentInstance = this;
    languageVariable = Language::ENGLISH; 
    debugPrint("SCENES", "_0LanguageSelectScene constructor");
}

_0LanguageSelectScene::~_0LanguageSelectScene() {
    debugPrint("SCENES", "_0LanguageSelectScene destroyed");
    if (currentInstance == this) {
        currentInstance = nullptr;
    }
}

void _0LanguageSelectScene::init(GameContext& context) { 
    Scene::init(); // Calls the generic base Scene::init()
    _gameContext = &context; // Store the context passed from the factory
    debugPrint("SCENES", "_0LanguageSelectScene::init");

    if (!_gameContext || !_gameContext->display) {
        debugPrint("SCENES", "ERROR: _0LanguageSelectScene::init - Critical context member (display) is null!");
        return;
    }
    u8g2_ptr = _gameContext->display; // Get U8G2 from context

    if (!menu) {
        menu.reset(new GEM_u8g2(*u8g2_ptr, GEM_POINTER_ROW, GEM_ITEMS_COUNT_AUTO));
        menu->setFontSmall();
        menuPageLanguage.addMenuItem(menuItemLanguageSelect);
        menuPageLanguage.addMenuItem(menuItemConfirm);
        menu->setMenuPageCurrent(menuPageLanguage);
        menu->hideVersion(); menu->setSplashDelay(0); menu->init();
        debugPrint("SCENES", "_0LanguageSelectScene: GEM menu initialized.");
    } else {
        menu->reInit();
        debugPrint("SCENES", "_0LanguageSelectScene: GEM menu re-initialized.");
    }
}

void _0LanguageSelectScene::onEnter() {
    debugPrint("SCENES", "_0LanguageSelectScene::onEnter");
    currentInstance = this; 

    if (menu) {
        menuPageLanguage.setTitle(loc(StringKey::PREQUEL_LANG_SELECT_TITLE));
        menuItemLanguageSelect.setTitle(loc(StringKey::PARAMS_LANG_SELECT)); 
        menuItemConfirm.setTitle(loc(StringKey::YES)); 

        if (_gameContext && _gameContext->gameStats && _gameContext->gameStats->selectedLanguage != LANGUAGE_UNINITIALIZED) { // Use context
            languageVariable = _gameContext->gameStats->selectedLanguage;
        } else {
            languageVariable = Language::ENGLISH; 
        }
        menu->drawMenu(); 
    } else {
        debugPrint("SCENES", "_0LanguageSelectScene::onEnter - Error: menu is null!");
    }
}

void _0LanguageSelectScene::onExit() {
    debugPrint("SCENES", "_0LanguageSelectScene::onExit");
}

void _0LanguageSelectScene::update(unsigned long deltaTime) { }

void _0LanguageSelectScene::draw(Renderer& renderer) { 
    if (menu) {
        menu->drawMenu();
    }
}

void _0LanguageSelectScene::processKeyPress(uint8_t keyCode) { 
    // This scene doesn't have a dialog box, so no handleDialogKeyPress is needed.
    if (menu) { menu->registerKeyPress(keyCode); } 
    else { debugPrint("SCENES", "_0LanguageSelectScene::processKeyPress - Error: menu is null!"); }
}

void _0LanguageSelectScene::onButtonOkLongPress(const EventMsg* msg) { }

void _0LanguageSelectScene::onLanguageConfirmWrapper() {
    if (currentInstance) {
        currentInstance->onLanguageConfirm();
    }
}

void _0LanguageSelectScene::onLanguageConfirm() {
    debugPrintf("SCENES", "_0LanguageSelectScene: Language confirmed: %d\n", (int)languageVariable);

    if (_gameContext && _gameContext->gameStats) { // Use context
        _gameContext->gameStats->setLanguage(languageVariable); 
        _gameContext->gameStats->setCompletedPrequelStage(PrequelStage::LANGUAGE_SELECTED);
        _gameContext->gameStats->save();
        debugPrint("SCENES", "_0LanguageSelectScene: Language and prequel stage saved.");
    } else {
        debugPrint("SCENES", "ERROR: GameStats (via context) is null in onLanguageConfirm!");
        extern Language currentLanguage; 
        currentLanguage = languageVariable;
    }

    if (_gameContext && _gameContext->sceneManager) { // Use context
        _gameContext->sceneManager->requestSetCurrentScene("PREQUEL_STAGE_1");
    } else {
        debugPrint("SCENES", "Error: SceneManager (via context) is null. Cannot switch scene.");
    }
}