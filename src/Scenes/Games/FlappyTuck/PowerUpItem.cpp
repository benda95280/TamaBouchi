#include "PowerUpItem.h"

PowerUpItem::PowerUpItem(float x, float y, CollectibleType type)
    : CollectibleObject(x, y, type, 
        (type == CollectibleType::POWERUP_GHOST ? FlappyTuckGraphics::getPowerUpGhostAsset() : 
         type == CollectibleType::POWERUP_SLOWMO ? FlappyTuckGraphics::getPowerUpSlowMoAsset() : 
         FlappyTuckGraphics::getCoinAsset() /* Fallback/Error asset */) ) { // Fallback to coin if type is unknown
    // PowerUp-specific initialization if any
}
