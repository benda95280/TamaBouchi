// --- START OF FILE Animator.h ---
#ifndef ANIMATOR_H
#define ANIMATOR_H

#include <Arduino.h>
#include <functional>
#include "Renderer.h"
#include <pgmspace.h>
#include <cmath>
#include <vector>
#include "Helper/PathGenerator.h" // <-- Include PathGenerator to get PathPoint definition
#include "Helper/GraphicAssetTypes.h" // <<< MODIFIED: Include new path for GraphicAssetData


// Pivot Point Definitions...
#define PIVOT_CENTER -1.0f
#define PIVOT_TOP_LEFT 0.0f


class Animator {
public:
    enum class AnimationType {
        MOVEMENT,
        FRAME_SPRITESHEET,
        ROTATION_MOVEMENT,
        PATH_MOVEMENT, // Uses dynamically generated path now
        SCALE
    };

    // Existing Constructors... (Keep MOVEMENT, FRAME_SPRITESHEET, ROTATION_MOVEMENT, SCALE as is)
    Animator(Renderer& renderer, const unsigned char* bitmap, int width, int height,
             int startX, int startY, int endX, int endY, unsigned long durationMillis);
    Animator(Renderer& renderer, const unsigned char* spriteSheet,
             int frameWidth, int frameHeight, int frameCount, size_t bytesPerFrame,
             int x, int y, unsigned long frameDurationMillis, int loops = 1);
    Animator(Renderer& renderer, const unsigned char* bitmap, int width, int height,
             int startX, int startY, int endX, int endY, unsigned long durationMillis,
             float startAngle, float endAngle,
             float pivotX = PIVOT_CENTER, float pivotY = PIVOT_CENTER,
             bool reverse = false, unsigned long reverseDelay = 0);
    Animator(Renderer& renderer, const unsigned char* bitmap, int width, int height,
             int x, int y, float startScale, float endScale,
             unsigned long durationMillis, int loops = 1,
             bool reverse = false, unsigned long reverseDelay = 0);

    // --- Simplified Path Movement Constructor ---
    // Takes sprite info and total duration. Path is set later.
    Animator(Renderer& renderer,
             const unsigned char* spriteBitmapOrSheet,
             int spriteWidth, int spriteHeight,
             int frameCount, size_t bytesPerFrame,
             unsigned long frameDurationMillis,
             unsigned long totalPathDurationMillis, // Duration for the path itself
             bool smoothPath = false,
             int loops = 1); // Loops for the PATH


    ~Animator() = default;

    // --- New method to set the path ---
    void setGeneratedPath(const std::vector<PathPoint>& pathPoints);

    bool update();
    void draw();
    bool isFinished() const;
    AnimationType getType() const { return _type; }

private:
    Renderer& _renderer;
    AnimationType _type;

    // Common properties...
    const unsigned char* _bitmap = nullptr;
    int _width;
    int _height;

    // Frame animation properties...
    int _frameCount = 0;
    int _currentFrame = 0;
    int _loops = 0;
    int _currentLoop = 0;
    unsigned long _frameDurationMillis = 0;
    size_t _bytesPerFrame = 0;
    int _fixedX, _fixedY;

    // Movement properties...
    int _startX, _startY;
    int _endX, _endY;
    float _currentX, _currentY;
    unsigned long _durationMillis; // Used for all except FRAME_SPRITESHEET

    // Rotation properties...
    float _startAngle = 0.0f;
    float _endAngle = 0.0f;
    float _currentAngle = 0.0f;
    float _pivotX = 0.0f;
    float _pivotY = 0.0f;

    // Scaling properties...
    float _startScale = 1.0f;
    float _endScale = 1.0f;
    float _currentScale = 1.0f;

    // --- Path properties ---
    std::vector<PathPoint> _pathPointsVector; // <-- Store the path internally
    int _currentPathSegment = 0;
    unsigned long _segmentDurationMillis = 0;
    bool _smoothPath = false;
    // --- End Path properties ---

    // Reversal properties...
    bool _reverseAnimation = false;
    unsigned long _reverseDelayMillis = 0;
    enum class AnimState { FORWARD, PAUSED_BEFORE_REVERSE, REVERSING, FINISHED };
    AnimState _state = AnimState::FORWARD;
    unsigned long _pauseStartTimeMillis = 0;

    // Timing properties...
    unsigned long _startTimeMillis;
    unsigned long _lastFrameUpdateTimeMillis;

    // State...
    bool _isAnimating = false;

    // Private Helpers...
    void drawRotatedBitmap();
    void drawScaledBitmap();
    float catmullRom(float t, float p0, float p1, float p2, float p3);
    void calculateSegmentDuration(); // Helper to calc segment time after path is set
};

#endif // ANIMATOR_H
// --- END OF FILE Animator.h ---