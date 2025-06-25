#include "Pipe.h"
#include "Renderer.h" // Make sure Renderer is included if its methods are used directly here
#include <cmath>      // For std::abs, round
#include "FlappyTuckGraphics.h" // For hazard graphics

// Constructor - Initialize inherited 'x' and pipe-specific members
Pipe::Pipe(int initialX, int screenHeight, int initialGapY, int initialWidth, int initialGapHeight)
    : gapY(initialGapY),
      width(initialWidth),
      gapHeight(initialGapHeight),
      _initialGapHeight(initialGapHeight), 
      scored(false),
      screenHeight(screenHeight),
      _accumulatedVerticalMove(0.0f), // Initialize accumulator
      _accumulatedGapChange(0.0f)    // Initialize accumulator
{
    this->x = initialX;
}

// Move the pipe left - Uses inherited 'x'
void Pipe::update(int speed, unsigned long deltaTime) {
    x -= speed;

    // Vertical movement
    if (_isMovingVertically) {
        _accumulatedVerticalMove += _verticalSpeed * _verticalMoveDirection;
        int pixelsToMove = static_cast<int>(round(_accumulatedVerticalMove));

        if (pixelsToMove != 0) {
            gapY += pixelsToMove;
            _accumulatedVerticalMove -= pixelsToMove; // Consume the whole part

            if (_verticalMoveDirection > 0 && gapY + gapHeight >= _maxY) { 
                gapY = _maxY - gapHeight;
                _verticalMoveDirection = -1;
                _accumulatedVerticalMove = 0; // Reset accumulator on direction change
            } else if (_verticalMoveDirection < 0 && gapY <= _minY) { 
                gapY = _minY;
                _verticalMoveDirection = 1;
                _accumulatedVerticalMove = 0; // Reset accumulator on direction change
            }
            gapY = std::max(0, std::min(screenHeight - gapHeight, gapY)); 
        }
    }

    // Gap changing
    if (_isGapChanging) {
        _accumulatedGapChange += _gapChangeSpeed;
        int gapHeightPixelChange = static_cast<int>(round(_accumulatedGapChange));

        if (gapHeightPixelChange != 0) {
            int oldGapHeight = gapHeight;
            gapHeight += gapHeightPixelChange;
             _accumulatedGapChange -= gapHeightPixelChange; // Consume the whole part

            int gapDiff = gapHeight - oldGapHeight;
            gapY -= gapDiff / 2; // Adjust to keep center somewhat stable

            // Check if target is reached or overshot
            bool targetReached = false;
            if (_gapChangeSpeed > 0) { // Expanding
                if (gapHeight >= _targetGapHeight) targetReached = true;
            } else { // Shrinking
                if (gapHeight <= _targetGapHeight) targetReached = true;
            }

            if (targetReached) {
                gapHeight = _targetGapHeight;
                int temp = _targetGapHeight;
                _targetGapHeight = _initialGapHeight; 
                _initialGapHeight = temp; 
                _gapChangeSpeed *= -1; 
                _accumulatedGapChange = 0; // Reset accumulator
            }
        }
        gapHeight = std::max(8, gapHeight); 
        gapY = std::max(0, std::min(screenHeight - gapHeight, gapY));
    }
}

// Draw the pipe
void Pipe::draw(Renderer& renderer) {
    if (_isBroken) return; 

    U8G2* u8g2 = renderer.getU8G2();
    if (!u8g2) return;

    renderer.drawFilledRectangle(x, 0, width, gapY);
    renderer.drawFilledRectangle(x, gapY + gapHeight, width, screenHeight - (gapY + gapHeight));

    if (_isFragile) {
        uint8_t originalColor = u8g2->getDrawColor();
        u8g2->setDrawColor(2); 
        if (gapY > 2) renderer.drawRectangle(x + 1, 1, width - 2, gapY - 2); 
        if (screenHeight - (gapY + gapHeight) > 2) renderer.drawRectangle(x + 1, gapY + gapHeight + 1, width - 2, screenHeight - (gapY + gapHeight) - 2); 
        u8g2->setDrawColor(originalColor);
    }

    GraphicAssetData hazardAsset = FlappyTuckGraphics::getPipeHazardAsset();
    if (_hasTopHazard && hazardAsset.isValid()) {
        renderer.getU8G2()->drawXBMP(renderer.getXOffset() + x + (width - hazardAsset.width)/2, renderer.getYOffset() + gapY - hazardAsset.height, hazardAsset.width, hazardAsset.height, hazardAsset.bitmap);
    }
    if (_hasBottomHazard && hazardAsset.isValid()) {
        renderer.getU8G2()->drawXBMP(renderer.getXOffset() + x + (width - hazardAsset.width)/2, renderer.getYOffset() + gapY + gapHeight, hazardAsset.width, hazardAsset.height, hazardAsset.bitmap);
    }
}

// Reset pipe properties
void Pipe::reset(int startX, int screenHeight, int newGapY, int newWidth, int newGapHeight) {
    x = startX;
    this->screenHeight = screenHeight;
    gapY = newGapY;
    width = newWidth; 
    gapHeight = newGapHeight;
    _initialGapHeight = newGapHeight;
    scored = false;
    _isBroken = false;
    _isMovingVertically = false;
    _verticalSpeed = 0.0f; 
    _accumulatedVerticalMove = 0.0f; // Reset accumulator
    _isGapChanging = false;
    _gapChangeSpeed = 0.0f; 
    _accumulatedGapChange = 0.0f; // Reset accumulator
    _isFragile = false;
    _hasTopHazard = false;
    _hasBottomHazard = false;
}

// Check if off-screen - Uses inherited 'x'
bool Pipe::isOffScreen() const {
    return (x + width) < 0;
}

// Check collision - Uses inherited 'x'
bool Pipe::collidesWith(float birdX, float birdY, float birdRadius) const {
    if (_isBroken) return false;

    bool collisionX = (birdX + birdRadius > x) && (birdX - birdRadius < x + width);
    if (!collisionX) {
        return false;
    }

    bool collisionY_pipe = (birdY - birdRadius < gapY) || (birdY + birdRadius > gapY + gapHeight);

    if (collisionY_pipe) {
        if (_isFragile) {
            return true; 
        }
        return true; 
    }

    if (_hasTopHazard) {
        float hazardTopY = gapY - _hazardHeight; 
        float hazardBottomY = gapY;             
        if (birdY + birdRadius > hazardTopY && birdY - birdRadius < hazardBottomY) {
            return true; 
        }
    }
    if (_hasBottomHazard) {
        float hazardTopY = gapY + gapHeight; 
        float hazardBottomY = gapY + gapHeight + _hazardHeight; 
        if (birdY - birdRadius < hazardBottomY && birdY + birdRadius > hazardTopY) {
            return true; 
        }
    }

    return false; 
}

// Getters - Use inherited 'x'
int Pipe::getX() const {
    return x;
}

int Pipe::getWidth() const {
    return width;
}

// --- Scoring Implementations ---
bool Pipe::needsScoreCheck() const {
    return !_isBroken; 
}

void Pipe::markScored() {
    scored = true;
}

bool Pipe::hasBeenScored() const {
    return scored;
}

// --- Public setters for new mechanics ---
void Pipe::setMoving(bool moving, float speed, int minY, int maxY) {
    _isMovingVertically = moving;
    _verticalSpeed = speed; 
    _minY = minY;
    _maxY = maxY;
    _verticalMoveDirection = (random(0,2) == 0) ? 1 : -1; 
    _accumulatedVerticalMove = 0.0f;
}

void Pipe::setGapChanging(bool changing, float speed, int targetGap) {
    _isGapChanging = changing;
    _gapChangeSpeed = speed; 
    _targetGapHeight = targetGap;
    _initialGapHeight = gapHeight; 
    _accumulatedGapChange = 0.0f;
}

void Pipe::setFragile(bool fragile) {
    _isFragile = fragile;
    _isBroken = false; 
}

void Pipe::setHazards(bool top, bool bottom, int hazardH) {
    _hasTopHazard = top;
    _hasBottomHazard = bottom;
    _hazardHeight = hazardH;
}