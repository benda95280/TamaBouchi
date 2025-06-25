#ifndef ENEMY_OBJECT_H
#define ENEMY_OBJECT_H

#include "GameObject.h"
#include "../../../Helper/GraphicAssetTypes.h"

enum class EnemyType {
    FLYING_BIRD
    // Add more enemy types here
};

class EnemyObject : public GameObject {
public:
    EnemyObject(float x, float y, EnemyType type, const GraphicAssetData& asset);
    virtual ~EnemyObject() override = default;

    virtual void update(int speed, unsigned long deltaTime) override = 0; // Enemies might have different speed logic
    void draw(Renderer& renderer) override; // Common draw logic for spritesheet/static
    bool isOffScreen() const override;
    bool collidesWith(float birdX, float birdY, float birdRadius) const override;

    int getX() const override { return static_cast<int>(_x); }
    int getWidth() const override { return _asset.width; }
    int getHeight() const { return _asset.height; }

    EnemyType getEnemyType() const { return _type; }
    GameObjectType getGameObjectType() const override { return GameObjectType::ENEMY; }

protected:
    float _x, _y;
    float _vx, _vy; // Enemies can have their own velocity components
    EnemyType _type;
    GraphicAssetData _asset;
    int _currentFrame;
    unsigned long _lastFrameUpdateTime;
};

#endif // ENEMY_OBJECT_H
