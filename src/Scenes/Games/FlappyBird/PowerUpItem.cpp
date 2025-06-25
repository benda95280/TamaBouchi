#include "PowerUpItem.h"

PowerUpItem::PowerUpItem(float x, float y, CollectibleType type)
    : CollectibleObject(x, y, type, 
        (type == CollectibleType::POWERUP_GHOST ? FlappyBirdGraphics::getPowerUpGhostAsset() : 
         type == CollectibleType::POWERUP_SLOWMO ? FlappyBirdGraphics::getPowerUpSlowMoAsset() : 
         FlappyBirdGraphics::getCoinAsset() /* Fallback/Error asset */) ) { // Fallback to coin if type is unknown
    // PowerUp-specific initialization if any
}
