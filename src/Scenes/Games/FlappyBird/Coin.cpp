#include "Coin.h"

Coin::Coin(float x, float y) 
    : CollectibleObject(x, y, CollectibleType::COIN, FlappyBirdGraphics::getCoinAsset()) {
    // Coin-specific initialization if any
}

// If Coin needs specific update or draw logic, implement it here.
// Otherwise, it will use CollectibleObject's methods.
