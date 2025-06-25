#ifndef POWERUP_ITEM_H
#define POWERUP_ITEM_H

#include "CollectibleObject.h"
#include "FlappyBirdGraphics.h"

class PowerUpItem : public CollectibleObject {
public:
    PowerUpItem(float x, float y, CollectibleType type); // Type determines asset
    ~PowerUpItem() override = default;
};

#endif // POWERUP_ITEM_H
