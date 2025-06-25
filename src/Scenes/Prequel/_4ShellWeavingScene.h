#ifndef SHELL_WEAVING_SCENE_H
#define SHELL_WEAVING_SCENE_H

#include "Scene.h"
#include "../../DialogBox/DialogBox.h"
#include <memory>
#include <vector>
#include "../../Helper/EffectsManager.h"
#include "../../System/GameContext.h" 

// Forward Declarations
class Renderer;

enum class ShellFragmentType
{
    NONE,
    TYPE_A,
    TYPE_B,
    TYPE_C,
    _TYPE_COUNT
};

struct ShellFragment
{
    float x, y;
    float vy;
    ShellFragmentType type;
    bool active = false;
    char displayChar = ' ';
};

class _4ShellWeavingScene : public Scene
{
public:
    _4ShellWeavingScene();
    ~_4ShellWeavingScene() override;

    SceneType getSceneType() const override { return SceneType::PREQUEL_STAGE_4; }

    // This is a scene-specific init method, not an override of the base class.
    void init(GameContext &context); 
    void onEnter() override;
    void onExit() override;
    void update(unsigned long deltaTime) override;
    void draw(Renderer &renderer) override; 

    DialogBox *getDialogBox() override { return _dialogBox.get(); }

private:
    std::unique_ptr<DialogBox> _dialogBox;
    std::unique_ptr<EffectsManager> _effectsManager;
    bool _instructionsShown = false;

    static const int MAX_FRAGMENTS = 8;
    ShellFragment _fragments[MAX_FRAGMENTS];
    unsigned long _lastFragmentSpawnTime = 0;
    unsigned long _nextFragmentSpawnInterval = 2000;

    int _collectorX = 0;
    const int COLLECTOR_Y_OFFSET = 5;
    const int COLLECTOR_WIDTH = 10;
    const int COLLECTOR_HEIGHT = 5;
    const int COLLECTOR_SPEED = 3;
    const int COLLECTION_RADIUS = 8;

    static const int PATTERN_LENGTH = 3;
    ShellFragmentType _targetPattern[PATTERN_LENGTH];
    ShellFragmentType _currentSlots[PATTERN_LENGTH];
    int _currentSlotIndex = 0;

    int _segmentsWoven = 0;
    static const int SEGMENTS_TO_WIN = 3;

    unsigned long _successfulWeaveTime = 0;
    const unsigned long WEAVE_SUCCESS_DISPLAY_MS = 1000;
    unsigned long _stageStartTime = 0;
    const unsigned long STAGE_DURATION_MS = 15000;

    // Scene-specific context pointer
    GameContext* _gameContext = nullptr;

    void onButtonLeftPress();
    void onButtonRightPress();
    void onButtonOkClick();
    bool handleDialogKeyPress(uint8_t keyCode);

    void completeStage();
    void spawnFragment();
    void updateFragments(unsigned long currentTime);
    void checkCollection();
    void setupNewPattern();
    void resetSlots();
    char getCharForFragmentType(ShellFragmentType type);
    void drawFragments(Renderer &renderer);
    void drawCollector(Renderer &renderer);
    void drawWeavingUI(Renderer &renderer);
    void drawShellProgress(Renderer &renderer);
};

#endif // SHELL_WEAVING_SCENE_H