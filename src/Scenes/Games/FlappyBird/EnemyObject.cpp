#include "EnemyObject.h"
#include <cmath> // For round

EnemyObject::EnemyObject(float x, float y, EnemyType type, const GraphicAssetData& asset)
    : _x(x), _y(y), _vx(0), _vy(0), _type(type), _asset(asset), _currentFrame(0), _lastFrameUpdateTime(0) {}

void EnemyObject::draw(Renderer& renderer) {
    if (!_asset.isValid()) return;

    int drawX = static_cast<int>(round(_x));
    int drawY = static_cast<int>(round(_y));

    const unsigned char* bitmapToDraw = _asset.bitmap;
    if (_asset.isSpritesheet() && _asset.frameCount > 0) {
        bitmapToDraw += (_currentFrame * _asset.bytesPerFrame);
    }

    if (bitmapToDraw) {
        renderer.getU8G2()->drawXBMP(renderer.getXOffset() + drawX, renderer.getYOffset() + drawY, _asset.width, _asset.height, bitmapToDraw);
    }
}

bool EnemyObject::isOffScreen() const {
    // Enemies can go off screen in any direction based on their movement
    return (_x + _asset.width < 0 || _x > _renderer.getWidth() || _y + _asset.height < 0 || _y > _renderer.getHeight());
}

bool EnemyObject::collidesWith(float birdX, float birdY, float birdRadius) const {
    float enemyRight = _x + _asset.width;
    float enemyBottom = _y + _asset.height;

    float birdLeft = birdX - birdRadius;
    float birdRight = birdX + birdRadius;
    float birdTop = birdY - birdRadius;
    float birdBottom = birdY + birdRadius;

    bool xOverlap = birdRight > _x && birdLeft < enemyRight;
    bool yOverlap = birdBottom > _y && birdTop < enemyBottom;

    return xOverlap && yOverlap;
}
