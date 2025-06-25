#ifndef COLLECTIBLE_OBJECT_H
#define COLLECTIBLE_OBJECT_H

#include "GameObject.h"
#include "../../../Helper/GraphicAssetTypes.h" // For GraphicAssetData

enum class CollectibleType {
    COIN,
    POWERUP_GHOST,
    POWERUP_SLOWMO
    // Add more power-up types here
};

class CollectibleObject : public GameObject {
public:
    CollectibleObject(float x, float y, CollectibleType type, const GraphicAssetData& asset);
    virtual ~CollectibleObject() override = default;

    void update(int speed, unsigned long deltaTime) override;
    void draw(Renderer& renderer) override;
    bool isOffScreen() const override;
    bool collidesWith(float birdX, float birdY, float birdRadius) const override;
    
    int getX() const override { return static_cast<int>(_x); }
    int getWidth() const override { return _asset.width; }
    int getHeight() const { return _asset.height; } // Added for consistency

    CollectibleType getCollectibleType() const { return _type; }
    GameObjectType getGameObjectType() const override { return GameObjectType::COLLECTIBLE; }


protected:
    float _x, _y;
    CollectibleType _type;
    GraphicAssetData _asset; // Store a copy of the asset data
    int _currentFrame;
    unsigned long _lastFrameUpdateTime;
};

#endif // COLLECTIBLE_OBJECT_H
