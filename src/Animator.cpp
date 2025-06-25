// --- START OF FILE Animator.cpp ---
#include "Animator.h"
#include <Arduino.h>
#include <algorithm>
#include <cmath>
// #include "SceneMain/Paths.h" // REMOVED
#include "SerialForwarder.h"
#include "DebugUtils.h"

extern SerialForwarder* forwardedSerial_ptr; // Use pointer type

// --- Constructors ---
// MOVEMENT, FRAME_SPRITESHEET, ROTATION_MOVEMENT, SCALE constructors remain largely the same
// Ensure they initialize the new _pathPointsVector member (e.g., .clear() or default init).

Animator::Animator(Renderer& renderer, const unsigned char* bitmap, int width, int height,
                   int startX, int startY, int endX, int endY, unsigned long durationMillis)
    : _renderer(renderer), _type(AnimationType::MOVEMENT), _bitmap(bitmap), _width(width), _height(height),
      _frameCount(0), _currentFrame(0), _loops(0), _currentLoop(0), _frameDurationMillis(0), _bytesPerFrame(0),
      _fixedX(0), _fixedY(0), _startX(startX), _startY(startY), _endX(endX), _endY(endY),
      _currentX(static_cast<float>(startX)), _currentY(static_cast<float>(startY)), _durationMillis(durationMillis),
      _startAngle(0), _endAngle(0), _currentAngle(0), _pivotX(0), _pivotY(0),
      _startScale(1.0f), _endScale(1.0f), _currentScale(1.0f),
      _currentPathSegment(0), _segmentDurationMillis(0), _smoothPath(false), // Init path vars
      _reverseAnimation(false), _reverseDelayMillis(0), _state(AnimState::FORWARD), _pauseStartTimeMillis(0),
      _startTimeMillis(millis()), _lastFrameUpdateTimeMillis(0), _isAnimating(true)
{
    // _pathPointsVector is default initialized (empty)
    if (_durationMillis == 0) {
        _currentX = static_cast<float>(_endX); _currentY = static_cast<float>(_endY);
        _isAnimating = false; _state = AnimState::FINISHED;
    }
    debugPrintf("ANIMATOR", "MOVEMENT Created. Duration: %lu, Animating: %s, State: %d", _durationMillis, _isAnimating?"true":"false", (int)_state);
}

Animator::Animator(Renderer& renderer, const unsigned char* spriteSheet,
                   int frameWidth, int frameHeight, int frameCount, size_t bytesPerFrame,
                   int x, int y, unsigned long frameDurationMillis, int loops)
    : _renderer(renderer), _type(AnimationType::FRAME_SPRITESHEET), _bitmap(spriteSheet), _width(frameWidth), _height(frameHeight),
      _frameCount(frameCount), _currentFrame(0), _loops(loops), _currentLoop(0), _frameDurationMillis(frameDurationMillis), _bytesPerFrame(bytesPerFrame),
      _fixedX(x), _fixedY(y), _startX(0), _startY(0), _endX(0), _endY(0), _currentX(0), _currentY(0), _durationMillis(0),
      _startAngle(0), _endAngle(0), _currentAngle(0), _pivotX(0), _pivotY(0),
      _startScale(1.0f), _endScale(1.0f), _currentScale(1.0f),
      _currentPathSegment(0), _segmentDurationMillis(0), _smoothPath(false), // Init path vars
      _reverseAnimation(false), _reverseDelayMillis(0), _state(AnimState::FORWARD), _pauseStartTimeMillis(0),
      _startTimeMillis(millis()), _lastFrameUpdateTimeMillis(_startTimeMillis), _isAnimating(true)
{
     // _pathPointsVector is default initialized (empty)
     if (_frameDurationMillis == 0 || _frameCount <= 0) {
         _isAnimating = false; _state = AnimState::FINISHED;
     }
     debugPrintf("ANIMATOR", "FRAME Created. Frames: %d, FrameDur: %lu, Loops: %d, Animating: %s, State: %d", 
              _frameCount, _frameDurationMillis, _loops, _isAnimating?"true":"false", (int)_state);
}

Animator::Animator(Renderer& renderer, const unsigned char* bitmap, int width, int height,
                   int startX, int startY, int endX, int endY, unsigned long durationMillis,
                   float startAngle, float endAngle, float pivotX, float pivotY, bool reverse, unsigned long reverseDelay)
    : _renderer(renderer), _type(AnimationType::ROTATION_MOVEMENT), _bitmap(bitmap), _width(width), _height(height),
      _frameCount(0), _currentFrame(0), _loops(0), _currentLoop(0), _frameDurationMillis(0), _bytesPerFrame(0),
      _fixedX(0), _fixedY(0), _startX(startX), _startY(startY), _endX(endX), _endY(endY),
      _currentX(static_cast<float>(startX)), _currentY(static_cast<float>(startY)), _durationMillis(durationMillis),
      _startAngle(startAngle), _endAngle(endAngle), _currentAngle(startAngle), _pivotX(pivotX), _pivotY(pivotY),
      _startScale(1.0f), _endScale(1.0f), _currentScale(1.0f),
      _currentPathSegment(0), _segmentDurationMillis(0), _smoothPath(false), // Init path vars
      _reverseAnimation(reverse), _reverseDelayMillis(reverseDelay), _state(AnimState::FORWARD), _pauseStartTimeMillis(0),
      _startTimeMillis(millis()), _lastFrameUpdateTimeMillis(0), _isAnimating(true)
{
    // _pathPointsVector is default initialized (empty)
    if (_pivotX == PIVOT_CENTER) _pivotX = static_cast<float>(_width) / 2.0f;
    if (_pivotY == PIVOT_CENTER) _pivotY = static_cast<float>(_height) / 2.0f;
    if (_durationMillis == 0) {
        _currentX = static_cast<float>(_endX); _currentY = static_cast<float>(_endY); _currentAngle = _endAngle;
        if (!_reverseAnimation) { _isAnimating = false; _state = AnimState::FINISHED; }
        else {
             if (_reverseDelayMillis > 0) { _state = AnimState::PAUSED_BEFORE_REVERSE; _pauseStartTimeMillis = millis(); }
             else { _state = AnimState::REVERSING; _startTimeMillis = millis(); std::swap(_startX, _endX); std::swap(_startY, _endY); std::swap(_startAngle, _endAngle); _currentX = (float)_startX; _currentY = (float)_startY; _currentAngle = _startAngle; }
        }
    }
    debugPrintf("ANIMATOR", "ROTATION Created. Duration: %lu, Reverse: %s, Animating: %s, State: %d", 
            _durationMillis, _reverseAnimation?"true":"false", _isAnimating?"true":"false", (int)_state);
}

Animator::Animator(Renderer& renderer, const unsigned char* bitmap, int width, int height,
                   int x, int y, float startScale, float endScale,
                   unsigned long durationMillis, int loops, bool reverse, unsigned long reverseDelay)
    : _renderer(renderer), _type(AnimationType::SCALE), _bitmap(bitmap), _width(width), _height(height),
      _frameCount(0), _currentFrame(0), _loops(loops), _currentLoop(0), _frameDurationMillis(0), _bytesPerFrame(0),
      _fixedX(x), _fixedY(y),
      _startX(0), _startY(0), _endX(0), _endY(0),
      _currentX(static_cast<float>(x)), _currentY(static_cast<float>(y)),
      _durationMillis(durationMillis),
      _startAngle(0), _endAngle(0), _currentAngle(0), _pivotX(0), _pivotY(0),
      _startScale(startScale), _endScale(endScale), _currentScale(startScale),
      _currentPathSegment(0), _segmentDurationMillis(0), _smoothPath(false), // Init path vars
      _reverseAnimation(reverse), _reverseDelayMillis(reverseDelay), _state(AnimState::FORWARD), _pauseStartTimeMillis(0),
      _startTimeMillis(millis()), _lastFrameUpdateTimeMillis(0), _isAnimating(true)
{
    // _pathPointsVector is default initialized (empty)
    if (_durationMillis == 0) {
        _currentScale = _endScale;
        if (!_reverseAnimation) { _isAnimating = false; _state = AnimState::FINISHED; }
        else {
            if (_reverseDelayMillis > 0) { _state = AnimState::PAUSED_BEFORE_REVERSE; _pauseStartTimeMillis = millis(); }
            else { _state = AnimState::REVERSING; _startTimeMillis = millis(); std::swap(_startScale, _endScale); _currentScale = _startScale; }
        }
    }
    debugPrintf("ANIMATOR", "SCALE Created. Duration: %lu, StartScale: %.2f, EndScale: %.2f, Reverse: %s, Animating: %s, State: %d",
        _durationMillis, _startScale, _endScale, _reverseAnimation?"true":"false", _isAnimating?"true":"false", (int)_state);
}


// --- Simplified Path Movement Constructor ---
Animator::Animator(Renderer& renderer,
                   const unsigned char* spriteBitmapOrSheet,
                   int spriteWidth, int spriteHeight,
                   int frameCount, size_t bytesPerFrame,
                   unsigned long frameDurationMillis,
                   unsigned long totalPathDurationMillis,
                   bool smoothPath,
                   int loops)
    : _renderer(renderer),
      _type(AnimationType::PATH_MOVEMENT),
      _bitmap(spriteBitmapOrSheet),
      _width(spriteWidth),
      _height(spriteHeight),
      _frameCount(frameCount <= 1 ? 0 : frameCount),
      _currentFrame(0),
      _loops(loops), // Path loops
      _currentLoop(0),
      _frameDurationMillis(frameDurationMillis),
      _bytesPerFrame(bytesPerFrame),
      _fixedX(0), _fixedY(0),
      _startX(0), _startY(0), _endX(0), _endY(0),
      _currentX(0), _currentY(0), // Will be set by setGeneratedPath
      _durationMillis(totalPathDurationMillis),
      _startAngle(0), _endAngle(0), _currentAngle(0), _pivotX(0), _pivotY(0),
      _startScale(1.0f), _endScale(1.0f), _currentScale(1.0f),
      // _pathData(nullptr), _pathPointCount(0), // Removed
      _currentPathSegment(0),
      _segmentDurationMillis(0), // Will be calculated by setGeneratedPath
      _smoothPath(smoothPath),
      _reverseAnimation(false), _reverseDelayMillis(0), _state(AnimState::FORWARD), _pauseStartTimeMillis(0),
      _startTimeMillis(millis()),
      _lastFrameUpdateTimeMillis(_startTimeMillis),
      _isAnimating(false) // Start as not animating until path is set
{
     // _pathPointsVector is default initialized (empty)
     debugPrint("ANIMATOR", "PATH Instance created. Waiting for path data...");
     // Path validity and duration checks moved to setGeneratedPath
}


// --- Method to set the generated path ---
void Animator::setGeneratedPath(const std::vector<PathPoint>& pathPoints) {
    if (pathPoints.size() < 2) {
        debugPrint("ANIMATOR", "PATH Error: Provided path has less than 2 points.");
        _isAnimating = false;
        _state = AnimState::FINISHED;
        return;
    }

    _pathPointsVector = pathPoints; // Copy the path points

    // Set starting position
    _currentX = static_cast<float>(_pathPointsVector[0].x);
    _currentY = static_cast<float>(_pathPointsVector[0].y);

    calculateSegmentDuration(); // Calculate segment duration based on the new path

    if (_segmentDurationMillis == 0 && _pathPointsVector.size() > 1) {
         debugPrint("ANIMATOR", "PATH Warning: Total duration too short for the number of segments in provided path.");
    }

    // Reset state and start animation
    _startTimeMillis = millis();
    _lastFrameUpdateTimeMillis = _startTimeMillis;
    _currentLoop = 0;
    _currentPathSegment = 0;
    _state = AnimState::FORWARD;
    _isAnimating = true;

    debugPrintf("ANIMATOR", "PATH set. Animated: %s, Smooth: %s, Points: %u, Segments: %u, TotalDur: %lu, SegDur: %lu, Loops: %d",
        (_frameCount>0)?"true":"false", _smoothPath?"true":"false", _pathPointsVector.size(), _pathPointsVector.size()-1, _durationMillis, _segmentDurationMillis, _loops);
}

// --- Helper to calculate segment duration ---
void Animator::calculateSegmentDuration() {
    if (_durationMillis == 0 || _pathPointsVector.empty()) {
        _segmentDurationMillis = 0;
        return;
    }
    size_t numSegments = _pathPointsVector.size() - 1;
    if (numSegments > 0) {
        _segmentDurationMillis = _durationMillis / numSegments;
    } else {
        _segmentDurationMillis = _durationMillis; // Only one segment (or just start point)
    }
}


// --- Catmull-Rom interpolation function ---
float Animator::catmullRom(float t, float p0, float p1, float p2, float p3) {
    float t2 = t * t;
    float t3 = t2 * t;
    return 0.5f * ( (2.0f * p1) +
                    (-p0 + p2) * t +
                    (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
                    (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3 );
}

// --- update() method ---
bool Animator::update() {
    if (!_isAnimating || _state == AnimState::FINISHED) { return false; } // Check if animation should run

    unsigned long currentTime = millis();

    // Handle Pause State
    if (_state == AnimState::PAUSED_BEFORE_REVERSE) {
        if (currentTime - _pauseStartTimeMillis >= _reverseDelayMillis) {
            _state = AnimState::REVERSING;
            _startTimeMillis = currentTime;
            if (_type == AnimationType::ROTATION_MOVEMENT) { std::swap(_startX, _endX); std::swap(_startY, _endY); std::swap(_startAngle, _endAngle); _currentX = (float)_startX; _currentY = (float)_startY; _currentAngle = _startAngle;}
            if (_type == AnimationType::SCALE) { std::swap(_startScale, _endScale); _currentScale = _startScale;}
            // Path reversal not implemented here
        } else {
            return true; // Still paused
        }
    }

    unsigned long elapsedTime = currentTime - _startTimeMillis;
    float progress = 0.0f;
    bool phaseComplete = false;
    bool pathLoopComplete = false;

    // Update Frame Animation (if applicable)
    if (_type == AnimationType::FRAME_SPRITESHEET || (_type == AnimationType::PATH_MOVEMENT && _frameCount > 0)) {
        if ( _frameDurationMillis > 0 && currentTime - _lastFrameUpdateTimeMillis >= _frameDurationMillis) {
            _lastFrameUpdateTimeMillis += _frameDurationMillis;
            _currentFrame++;
            if (_currentFrame >= _frameCount) {
                if (_type == AnimationType::FRAME_SPRITESHEET) {
                    _currentLoop++;
                    if (_loops > 0 && _currentLoop >= _loops) {
                         _isAnimating = false; _state = AnimState::FINISHED; _currentFrame = _frameCount - 1; return false;
                     } else { _currentFrame = 0; }
                 } else { // Path movement frame animation loops indefinitely
                     _currentFrame = 0;
                 }
            }
        }
        if (_type == AnimationType::FRAME_SPRITESHEET) return true; // Frame-only anim doesn't need progress calc
    }

    // Calculate progress for duration-based types
    if (_type != AnimationType::FRAME_SPRITESHEET) {
        if (_durationMillis == 0 && _type != AnimationType::PATH_MOVEMENT) { // Path handles 0 duration differently
             progress = 1.0f; phaseComplete = true;
         } else if (_durationMillis > 0) { // Only calc progress if duration > 0
            if (elapsedTime >= _durationMillis) { progress = 1.0f; phaseComplete = true; }
            else { progress = static_cast<float>(elapsedTime) / static_cast<float>(_durationMillis); }
         } else {
             // If duration is 0 and it's a path, it's handled within PATH logic
             phaseComplete = (_type != AnimationType::PATH_MOVEMENT); // Mark other 0-duration types complete
             progress = 1.0;
         }
    }


    // Update State based on Type
    if (_type == AnimationType::MOVEMENT || _type == AnimationType::ROTATION_MOVEMENT) {
        _currentX = static_cast<float>(_startX) + (static_cast<float>(_endX - _startX) * progress);
        _currentY = static_cast<float>(_startY) + (static_cast<float>(_endY - _startY) * progress);
        if (_type == AnimationType::ROTATION_MOVEMENT) {
            _currentAngle = _startAngle + (_endAngle - _startAngle) * progress;
        }
    } else if (_type == AnimationType::PATH_MOVEMENT) {
        if (_pathPointsVector.size() < 2) { _isAnimating = false; _state = AnimState::FINISHED; return false; }

        size_t numSegments = _pathPointsVector.size() - 1;
        unsigned long totalLoopDuration = numSegments * _segmentDurationMillis;
        if (totalLoopDuration == 0 && numSegments > 0) totalLoopDuration = 1;

        pathLoopComplete = (totalLoopDuration > 0 && elapsedTime >= totalLoopDuration);

        if (pathLoopComplete) {
             _currentLoop++;
             if (_loops > 0 && _currentLoop >= _loops) {
                 _isAnimating = false; _state = AnimState::FINISHED;
                 _currentX = (float)_pathPointsVector.back().x; _currentY = (float)_pathPointsVector.back().y;
                 return false;
             } else {
                 _startTimeMillis = currentTime; elapsedTime = 0; _currentPathSegment = 0;
                 _currentX = (float)_pathPointsVector[0].x; _currentY = (float)_pathPointsVector[0].y;
                 pathLoopComplete = false;
             }
        }

        if (!pathLoopComplete) {
            if (_segmentDurationMillis > 0) {
                 _currentPathSegment = std::min((size_t)(elapsedTime / _segmentDurationMillis), numSegments - 1);
            } else {
                 _currentPathSegment = numSegments - 1; // Jump to last segment if duration is 0
            }
            unsigned long timeIntoSegment = (_segmentDurationMillis > 0) ? (elapsedTime % _segmentDurationMillis) : _segmentDurationMillis;
            float segmentProgress = (_segmentDurationMillis > 0) ? ((float)timeIntoSegment / (float)_segmentDurationMillis) : 1.0f;

            if (_smoothPath && _pathPointsVector.size() >= 2) { // Need at least 2 points for smoothing refs
                PathPoint p0_data, p1_data, p2_data, p3_data;
                int p1_idx = _currentPathSegment;
                int p2_idx = p1_idx + 1;
                int p0_idx = (p1_idx == 0) ? p1_idx : p1_idx - 1; // Use p1 if at start
                int p3_idx = (p2_idx >= numSegments) ? p2_idx : p2_idx + 1; // Use p2 if at end
                // Clamp indices
                p0_idx = std::max(0, std::min((int)_pathPointsVector.size() - 1, p0_idx));
                p1_idx = std::max(0, std::min((int)_pathPointsVector.size() - 1, p1_idx));
                p2_idx = std::max(0, std::min((int)_pathPointsVector.size() - 1, p2_idx));
                p3_idx = std::max(0, std::min((int)_pathPointsVector.size() - 1, p3_idx));

                p0_data = _pathPointsVector[p0_idx];
                p1_data = _pathPointsVector[p1_idx];
                p2_data = _pathPointsVector[p2_idx];
                p3_data = _pathPointsVector[p3_idx];
                _currentX = catmullRom(segmentProgress, (float)p0_data.x, (float)p1_data.x, (float)p2_data.x, (float)p3_data.x);
                _currentY = catmullRom(segmentProgress, (float)p0_data.y, (float)p1_data.y, (float)p2_data.y, (float)p3_data.y);
            } else { // Linear Interpolation
                const PathPoint& p1 = _pathPointsVector[_currentPathSegment];
                const PathPoint& p2 = _pathPointsVector[_currentPathSegment + 1];
                _currentX = (float)p1.x + ((float)p2.x - (float)p1.x) * segmentProgress;
                _currentY = (float)p1.y + ((float)p2.y - (float)p1.y) * segmentProgress;
            }
        } else if (_durationMillis == 0) { // Handle 0 duration path completion
            _currentLoop++;
             if (_loops > 0 && _currentLoop >= _loops) {
                  _isAnimating = false; _state = AnimState::FINISHED;
                  _currentX = (float)_pathPointsVector.back().x; _currentY = (float)_pathPointsVector.back().y;
                  return false;
             } else { // Reset for next loop immediately
                  _startTimeMillis = currentTime; elapsedTime = 0; _currentPathSegment = 0;
                 _currentX = (float)_pathPointsVector[0].x; _currentY = (float)_pathPointsVector[0].y;
             }
        }
        return true; // Path animation continues based on loop logic

    } else if (_type == AnimationType::SCALE) {
        _currentScale = _startScale + (_endScale - _startScale) * progress;
    }


    // Handle phase completion for non-path, non-frame animations
    if (phaseComplete && _type != AnimationType::PATH_MOVEMENT) {
         if (_state == AnimState::FORWARD && _reverseAnimation) {
             if (_reverseDelayMillis > 0) { _state = AnimState::PAUSED_BEFORE_REVERSE; _pauseStartTimeMillis = currentTime; }
             else {
                  _state = AnimState::REVERSING; _startTimeMillis = currentTime;
                  if (_type == AnimationType::ROTATION_MOVEMENT) { std::swap(_startX, _endX); std::swap(_startY, _endY); std::swap(_startAngle, _endAngle); _currentX = (float)_startX; _currentY = (float)_startY; _currentAngle = _startAngle;}
                  if (_type == AnimationType::SCALE) { std::swap(_startScale, _endScale); _currentScale = _startScale;}
             }
             return true;
         } else {
             _state = AnimState::FINISHED; _isAnimating = false; return false;
         }
    }

    return true; // Still animating
}


// Draw method handles all types
void Animator::draw() {
    if (_state == AnimState::FINISHED || !_isAnimating) return; // Don't draw if finished or not animating

    U8G2* u8g2 = _renderer.getU8G2();
    if (!u8g2 || !_bitmap) return; // Need renderer and bitmap/spritesheet

    // --- Draw based on type ---
    if (_type == AnimationType::MOVEMENT) {
        u8g2->drawXBMP( _renderer.getXOffset() + static_cast<int>(round(_currentX)), _renderer.getYOffset() + static_cast<int>(round(_currentY)), _width, _height, _bitmap);
    } else if (_type == AnimationType::FRAME_SPRITESHEET) {
         if (_currentFrame < _frameCount) {
             const uint8_t* framePtr = _bitmap + (_currentFrame * _bytesPerFrame);
             u8g2->drawXBMP( _renderer.getXOffset() + _fixedX, _renderer.getYOffset() + _fixedY, _width, _height, framePtr);
         }
    } else if (_type == AnimationType::ROTATION_MOVEMENT) {
        drawRotatedBitmap();
    } else if (_type == AnimationType::PATH_MOVEMENT) {
         const uint8_t* frameToDraw = _bitmap; // Default to the start (for static path item)
         if (_frameCount > 0 && _currentFrame < _frameCount) { // If animated path
             frameToDraw = _bitmap + (_currentFrame * _bytesPerFrame);
         }
         u8g2->drawXBMP( _renderer.getXOffset() + static_cast<int>(round(_currentX)), _renderer.getYOffset() + static_cast<int>(round(_currentY)), _width, _height, frameToDraw);
    } else if (_type == AnimationType::SCALE) {
        drawScaledBitmap();
    }
}

// --- drawRotatedBitmap (remains the same) ---
void Animator::drawRotatedBitmap() {
    U8G2* u8g2 = _renderer.getU8G2();
    if (!_bitmap || !u8g2) return;

    float angleRad = radians(_currentAngle);
    float cosAngle = cos(angleRad);
    float sinAngle = sin(angleRad);

    int screenOffsetX = _renderer.getXOffset();
    int screenOffsetY = _renderer.getYOffset();

    for (int y = 0; y < _height; ++y) {
        for (int x = 0; x < _width; ++x) {
            const unsigned char* bytePtr = _bitmap + (y * ((_width + 7) / 8)) + (x / 8);
            unsigned char byteVal = pgm_read_byte(bytePtr);
            if ((byteVal >> (x % 8)) & 0x01) { // LSB-first bit check
                float translatedX = static_cast<float>(x) - _pivotX;
                float translatedY = static_cast<float>(y) - _pivotY;
                float rotatedX = translatedX * cosAngle - translatedY * sinAngle;
                float rotatedY = translatedX * sinAngle + translatedY * cosAngle;
                int finalX = screenOffsetX + static_cast<int>(round(rotatedX + _pivotX + _currentX));
                int finalY = screenOffsetY + static_cast<int>(round(rotatedY + _pivotY + _currentY));
                u8g2->drawPixel(finalX, finalY);
            }
        }
    }
}

// --- drawScaledBitmap (remains the same) ---
void Animator::drawScaledBitmap() {
    U8G2* u8g2 = _renderer.getU8G2();
    if (!_bitmap || !u8g2 || _currentScale <= 0) return;

    int scaledW = static_cast<int>(round(_width * _currentScale));
    int scaledH = static_cast<int>(round(_height * _currentScale));
    if (scaledW <= 0 || scaledH <= 0) return;

    int originalCenterX = _fixedX + _width / 2;
    int originalCenterY = _fixedY + _height / 2;
    int drawX = _renderer.getXOffset() + originalCenterX - scaledW / 2;
    int drawY = _renderer.getYOffset() + originalCenterY - scaledH / 2;

    for (int destY = drawY; destY < drawY + scaledH; ++destY) {
        for (int destX = drawX; destX < drawX + scaledW; ++destX) {
            if (destX < _renderer.getXOffset() || destX >= _renderer.getXOffset() + _renderer.getWidth() ||
                destY < _renderer.getYOffset() || destY >= _renderer.getYOffset() + _renderer.getHeight()) {
                continue;
            }
            int srcX = static_cast<int>(floor(static_cast<float>(destX - drawX) / _currentScale));
            int srcY = static_cast<int>(floor(static_cast<float>(destY - drawY) / _currentScale));
            if (srcX >= 0 && srcX < _width && srcY >= 0 && srcY < _height) {
                const unsigned char* bytePtr = _bitmap + (srcY * ((_width + 7) / 8)) + (srcX / 8);
                unsigned char byteVal = pgm_read_byte(bytePtr);
                 if ((byteVal >> (srcX % 8)) & 0x01) { // LSB FIRST
                    u8g2->drawPixel(destX, destY);
                }
            }
        }
    }
}


// --- isFinished relies solely on the state machine ---
bool Animator::isFinished() const {
    return _state == AnimState::FINISHED;
}
// --- END OF FILE Animator.cpp ---
