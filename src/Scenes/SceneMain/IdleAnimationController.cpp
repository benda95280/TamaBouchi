#include "IdleAnimationController.h"
#include <Arduino.h> // For millis(), random()
#include <cmath>     // For PI, cos, sin etc. if needed directly (Animator uses it)
IdleAnimationController::IdleAnimationController(Renderer &renderer, CharacterManager *charMgr, PathGenerator *pathGen)
    : _renderer(renderer), _characterManager(charMgr), _pathGenerator(pathGen),
      _snoozeState(SnoozeStateInternal::NONE), _snoozeFrameEndTime(0),
      _snoozeBitmap1(nullptr), _snoozeBitmap2(nullptr),
      _activeAnimType(CurrentIdleAnimType::NONE)
{
    debugPrint("SCENES", "IdleAnimationController created.");
}
IdleAnimationController::~IdleAnimationController()
{
    debugPrint("SCENES", "IdleAnimationController destroyed.");
}
void IdleAnimationController::reset()
{
    _currentAnimator.reset();
    _snoozeState = SnoozeStateInternal::NONE;
    _snoozeBitmap1 = nullptr;
    _snoozeBitmap2 = nullptr;
    _activeAnimType = CurrentIdleAnimType::NONE;
    debugPrint("SCENES", "IdleAnimationController reset.");
}
bool IdleAnimationController::isAnimating() const
{
    return (_currentAnimator && _currentAnimator->isFinished() == false) || _snoozeState != SnoozeStateInternal::NONE;
}
void IdleAnimationController::startRandomIdleAnimation(unsigned long currentTime, int targetX, int targetY)
{
    if (isAnimating())
    {
        debugPrint("SCENES", "IdleAnimController: Called startRandomIdleAnimation while already animating. Ignoring.");
        return;
    }
    reset(); // Ensure clean state

    int choice = random(0, 3); // 0 for Snooze, 1 for Lean, 2 for Path
    debugPrintf("SCENES", "IdleAnimController: Random choice for idle anim: %d", choice);

    bool success = false;
    switch (choice)
    {
    case 0:
        success = triggerSnoozeAnimation(currentTime, targetX, targetY);
        break;
    case 1:
        success = triggerLeanAnimation(currentTime, targetX, targetY);
        break;
    case 2:
        success = triggerPathAnimation(currentTime);
        break;
    }
    if (!success)
    {
        // Fallback or retry logic if needed, e.g., try a different animation
        debugPrint("SCENES", "IdleAnimController: Failed to start chosen random animation. Trying another...");
        // For simplicity, just try one other type if the first fails.
        // A more robust system might cycle through them or have weighted fallbacks.
        int fallbackChoice = (choice + 1) % 3;
        switch (fallbackChoice)
        {
        case 0:
            success = triggerSnoozeAnimation(currentTime, targetX, targetY);
            break;
        case 1:
            success = triggerLeanAnimation(currentTime, targetX, targetY);
            break;
        case 2:
            success = triggerPathAnimation(currentTime);
            break;
        }
        if (!success)
        {
            debugPrint("SCENES", "IdleAnimController: Fallback animation also failed to start.");
        }
    }
}
bool IdleAnimationController::triggerSnoozeAnimation(unsigned long currentTime, int targetX, int targetY)
{
    if (isAnimating())
        return false;
    if (!_characterManager)
    {
        debugPrint("SCENES", "IdleAnimCtrl Error: CharacterManager is null (Snooze).");
        return false;
    }

    _snoozeState = SnoozeStateInternal::SNOOZE_1;
    _snoozeFrameEndTime = currentTime + SNOOZE_FRAME_DURATION_MS;

    GraphicAssetData asset1, asset2;
    bool snooze1Ok = _characterManager->getGraphicAssetData(GraphicType::SNOOZE_1, asset1);
    bool snooze2Ok = _characterManager->getGraphicAssetData(GraphicType::SNOOZE_2, asset2);

    if (!snooze1Ok || !snooze2Ok || !asset1.isValid() || !asset2.isValid())
    {
        debugPrint("SCENES", "IdleAnimCtrl Error: Snooze assets not available.");
        _snoozeState = SnoozeStateInternal::NONE;
        return false;
    }
    if (random(0, 2) == 0)
    {
        _snoozeBitmap1 = asset1.bitmap;
        _snoozeBitmap2 = asset2.bitmap;
    }
    else
    {
        _snoozeBitmap1 = asset2.bitmap;
        _snoozeBitmap2 = asset1.bitmap;
    }
    _activeAnimType = CurrentIdleAnimType::SNOOZE;
    debugPrintf("SCENES", "IdleAnimCtrl: Starting SNOOZE sequence until %lu (frame 1)", _snoozeFrameEndTime);
    return true;
}
bool IdleAnimationController::triggerLeanAnimation(unsigned long currentTime, int targetX, int targetY)
{
    if (isAnimating())
        return false;
    if (!_characterManager)
    {
        debugPrint("SCENES", "IdleAnimCtrl Error: CharacterManager is null (Lean).");
        return false;
    }

    int leanDirection = (random(0, 2) == 0) ? -1 : 1;
    int leanOffsetX = leanDirection * random(2, MAX_LEAN_OFFSET_X + 1);
    float leanAngle = static_cast<float>(leanDirection * (random(50, static_cast<int>(MAX_LEAN_ANGLE * 10) + 1)) / 10.0f);

    GraphicAssetData asset;
    bool leanBmpOk = (random(0, 2) == 0)
                         ? _characterManager->getGraphicAssetData(GraphicType::SNOOZE_1, asset)
                         : _characterManager->getGraphicAssetData(GraphicType::SNOOZE_2, asset);

    if (!leanBmpOk || !asset.isValid())
    {
        debugPrint("SCENES", "IdleAnimCtrl Error: Cannot start LEAN anim - missing assets.");
        return false;
    }
    debugPrintf("SCENES", "IdleAnimCtrl: Starting LEAN animation (Offset: %d, Angle: %.1f)", leanOffsetX, leanAngle);
    _currentAnimator.reset(new Animator(_renderer, asset.bitmap, asset.width, asset.height,
                                        targetX, targetY, targetX + leanOffsetX, targetY, LEAN_DURATION_MS,
                                        0.0f, leanAngle, PIVOT_CENTER, static_cast<float>(asset.height),
                                        true, LEAN_REVERSE_DELAY_MS));
    _activeAnimType = CurrentIdleAnimType::LEAN;
    return true;
}
bool IdleAnimationController::triggerPathAnimation(unsigned long currentTime)
{
    if (isAnimating())
        return false;
    if (!_characterManager || !_pathGenerator)
    {
        debugPrint("SCENES", "IdleAnimCtrl Error: CharacterManager or PathGenerator is null (Path).");
        return false;
    }

    std::vector<PathPoint> generatedPath;
    unsigned long estimatedDuration;
    PathGenConfig config; // Use default config or customize if needed
    config.minPoints = 3;
    config.maxPoints = 5;
    config.minStepDistance = 10;
    config.maxStepDistance = 20;
    config.baseDurationMs = 3000;
    config.minDurationMs = 2000;
    config.maxDurationMs = 5000;

    if (!_pathGenerator->generateFlyingPath(generatedPath, estimatedDuration, _renderer.getWidth(), _renderer.getHeight(), config))
    {
        debugPrint("SCENES", "IdleAnimCtrl: Failed to generate a valid dynamic path.");
        return false;
    }
    debugPrintf("SCENES", "IdleAnimCtrl: Triggering dynamic PATH animation (%d points, est duration %lu ms).", generatedPath.size(), estimatedDuration);

    int spriteChoice = random(0, 3);
    GraphicAssetData spriteAsset;
    bool spriteOk = false;
    GraphicType spriteTypeToLoad;
    switch (spriteChoice)
    {
    case 0:
        spriteTypeToLoad = GraphicType::FLYING_BEE;
        debugPrint("SCENES", "  Using Bee sprite.");
        break;
    case 1:
        spriteTypeToLoad = GraphicType::FLYING_BUTTERFLY;
        debugPrint("SCENES", "  Using Butterfly sprite.");
        break;
    case 2:
        spriteTypeToLoad = GraphicType::FLYING_VEHICLE;
        debugPrint("SCENES", "  Using Space Vehicle sprite.");
        break;
    default:
        return false; // Should not happen
    }
    spriteOk = _characterManager->getGraphicAssetData(spriteTypeToLoad, spriteAsset);

    if (!spriteOk || !spriteAsset.isSpritesheet())
    {
        debugPrint("SCENES", "IdleAnimCtrl Error: Failed to get valid sprite asset data for path.");
        return false;
    }
    _currentAnimator.reset(new Animator(_renderer, spriteAsset.bitmap, spriteAsset.width, spriteAsset.height,
                                        spriteAsset.frameCount, spriteAsset.bytesPerFrame, spriteAsset.frameDurationMs,
                                        estimatedDuration, true, 1));
    _currentAnimator->setGeneratedPath(generatedPath);
    _activeAnimType = CurrentIdleAnimType::PATH;
    return true;
}
bool IdleAnimationController::update(unsigned long currentTime)
{
    if (_currentAnimator)
    {
        if (!_currentAnimator->update())
        { // Animator finished
            _currentAnimator.reset();
            _activeAnimType = CurrentIdleAnimType::NONE;
            return false; // Animation done
        }
        return true; // Animator still running
    }
    else if (_snoozeState != SnoozeStateInternal::NONE)
    {
        if (currentTime >= _snoozeFrameEndTime)
        {
            if (_snoozeState == SnoozeStateInternal::SNOOZE_1)
            {
                _snoozeState = SnoozeStateInternal::SNOOZE_2;
                _snoozeFrameEndTime = currentTime + SNOOZE_FRAME_DURATION_MS;
            }
            else
            { // Was SNOOZE_2, now finished
                _snoozeState = SnoozeStateInternal::NONE;
                _snoozeBitmap1 = nullptr;
                _snoozeBitmap2 = nullptr;
                _activeAnimType = CurrentIdleAnimType::NONE;
                return false; // Snooze done
            }
        }
        return true; // Snooze still active
    }
    _activeAnimType = CurrentIdleAnimType::NONE; // Should not happen if isAnimating() was true
    return false;                                // Not animating
}
void IdleAnimationController::draw()
{
    if (_currentAnimator)
    {
        _currentAnimator->draw();
    }
    else if (_snoozeState != SnoozeStateInternal::NONE)
    {
        const unsigned char *bitmapToDraw = nullptr;
        GraphicAssetData asset;
        bool assetOk = false;

        if (_snoozeState == SnoozeStateInternal::SNOOZE_1 && _snoozeBitmap1)
        {
            bitmapToDraw = _snoozeBitmap1;
            assetOk = _characterManager && _characterManager->getGraphicAssetData(GraphicType::SNOOZE_1, asset);
        }
        else if (_snoozeState == SnoozeStateInternal::SNOOZE_2 && _snoozeBitmap2)
        {
            bitmapToDraw = _snoozeBitmap2;
            assetOk = _characterManager && _characterManager->getGraphicAssetData(GraphicType::SNOOZE_2, asset);
        }

        if (bitmapToDraw && assetOk && asset.isValid())
        {
            // Assume targetX/Y are character's main position, get from CharacterManager or pass to draw
            // For simplicity, assume character is centered for now if MainScene doesn't provide position
            int charX = (_renderer.getWidth() - asset.width) / 2;
            int charY = (_renderer.getHeight() - asset.height) / 2;
            U8G2 *u8g2 = _renderer.getU8G2();
            if (u8g2)
                u8g2->drawXBMP(_renderer.getXOffset() + charX, _renderer.getYOffset() + charY, asset.width, asset.height, bitmapToDraw);
        }
    }
}