#ifndef FLYING_ENEMY_H
#define FLYING_ENEMY_H

#include "EnemyObject.h"
#include "FlappyTuckGraphics.h"

class FlyingEnemy : public EnemyObject {
public:
    FlyingEnemy(float x, float y, float speedX);
    ~FlyingEnemy() override = default;

    void update(int screenSpeed, unsigned long deltaTime) override;
    // Draw, isOffScreen, collidesWith, getX, getWidth inherited or use EnemyObject's
};

#endif // FLYING_ENEMY_H
