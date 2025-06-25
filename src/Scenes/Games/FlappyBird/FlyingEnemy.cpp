#include "FlyingEnemy.h"

FlyingEnemy::FlyingEnemy(float x, float y, float speedX)
    : EnemyObject(x, y, EnemyType::FLYING_BIRD, FlappyBirdGraphics::getEnemyBirdAsset()) {
    _vx = speedX; // Flying enemy has a horizontal speed
    _vy = 0;      // No vertical speed unless implemented
}

void FlyingEnemy::update(int screenSpeed, unsigned long deltaTime) {
    // screenSpeed is the general speed of pipes, etc.
    // The enemy's own _vx determines its movement relative to that, or independently.
    // For this simple enemy, let's assume its _vx is its absolute speed.
    _x += _vx; 
    // _x -= screenSpeed; // If enemy should move with screen + its own speed

    // Animation
    if (_asset.isSpritesheet()) {
        unsigned long currentTime = millis();
        if (currentTime - _lastFrameUpdateTime >= _asset.frameDurationMs) {
            _lastFrameUpdateTime = currentTime;
            _currentFrame = (_currentFrame + 1) % _asset.frameCount;
        }
    }
}
