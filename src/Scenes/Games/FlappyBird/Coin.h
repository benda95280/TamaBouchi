#ifndef COIN_H
#define COIN_H

#include "CollectibleObject.h"
#include "FlappyBirdGraphics.h" // For graphics definitions

class Coin : public CollectibleObject {
public:
    Coin(float x, float y);
    ~Coin() override = default;

    // Specific draw or update if needed, otherwise inherits from CollectibleObject
};

#endif // COIN_H
