#include "CollectibleObject.h"
#include <cmath> // For round

CollectibleObject::CollectibleObject(float x, float y, CollectibleType type, const GraphicAssetData& asset)
    : _x(x), _y(y), _type(type), _asset(asset), _currentFrame(0), _lastFrameUpdateTime(0) {}

void CollectibleObject::update(int speed, unsigned long deltaTime) {
    _x -= speed; // Collectibles move with the screen

    // Animation update (if spritesheet)
    if (_asset.isSpritesheet()) {
        unsigned long currentTime = millis();
        if (currentTime - _lastFrameUpdateTime >= _asset.frameDurationMs) {
            _lastFrameUpdateTime = currentTime;
            _currentFrame = (_currentFrame + 1) % _asset.frameCount;
        }
    }
}

void CollectibleObject::draw(Renderer& renderer) {
    if (!_asset.isValid()) return;

    U8G2* u8g2 = renderer.getU8G2();
    if (!u8g2) return;

    int drawX = static_cast<int>(round(_x));
    int drawY = static_cast<int>(round(_y));

    const unsigned char* bitmapToDraw = _asset.bitmap;
    if (_asset.isSpritesheet() && _asset.frameCount > 0) {
        bitmapToDraw += (_currentFrame * _asset.bytesPerFrame);
    }

    if (bitmapToDraw) {
        uint8_t originalColor = u8g2->getDrawColor(); // Store original color
        u8g2->setDrawColor(1); // Ensure drawing in white (or non-background color)
        
        // The coordinates passed to drawXBMP should be the final screen coordinates.
        // Renderer's getXOffset/getYOffset are for the main game screen offset on the physical display.
        // drawX/drawY are logical game coordinates.
        u8g2->drawXBMP(renderer.getXOffset() + drawX, 
                       renderer.getYOffset() + drawY, 
                       _asset.width, 
                       _asset.height, 
                       bitmapToDraw);
        
        u8g2->setDrawColor(originalColor); // Restore original color
    }
}

bool CollectibleObject::isOffScreen() const {
    return (_x + _asset.width) < 0;
}

bool CollectibleObject::collidesWith(float birdX, float birdY, float birdRadius) const {
    // Simple AABB collision for collectibles
    float collectibleRight = _x + _asset.width;
    float collectibleBottom = _y + _asset.height;

    // Bird's bounding box
    float birdLeft = birdX - birdRadius;
    float birdRight = birdX + birdRadius;
    float birdTop = birdY - birdRadius;
    float birdBottom = birdY + birdRadius;

    bool xOverlap = birdRight > _x && birdLeft < collectibleRight;
    bool yOverlap = birdBottom > _y && birdTop < collectibleBottom;

    return xOverlap && yOverlap;
}