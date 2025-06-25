#ifndef FLAPPY_BIRD_GRAPHICS_H
#define FLAPPY_BIRD_GRAPHICS_H

#include <pgmspace.h>
#include "../../../Helper/GraphicAssetTypes.h" // For GraphicAssetData

// --- Coin Graphics (Example: 4x4 static, or simple animation) ---
#define COIN_SPRITE_WIDTH 4
#define COIN_SPRITE_HEIGHT 4
const unsigned char bmp_flappy_coin[] PROGMEM = {
    0x60, // 0110
    0xF0, // 1111
    0xF0, // 1111
    0x60  // 0110
}; // Single frame 4x4 coin

// --- Power-up Graphics (Example: 6x6 static) ---
#define POWERUP_SPRITE_WIDTH 6
#define POWERUP_SPRITE_HEIGHT 6

// Ghost Power-up (Placeholder: 'G')
const unsigned char bmp_flappy_powerup_ghost[] PROGMEM = {
    0x7E, // 01111110
    0x81, // 10000001
    0x81, // 10000001
    0x8F, // 10001111
    0x81, // 10000001
    0x7E  // 01111110
};

// Slow-Mo Power-up (Placeholder: 'S')
const unsigned char bmp_flappy_powerup_slowmo[] PROGMEM = {
    0x7E, // 01111110
    0x80, // 10000000
    0x7E, // 01111110
    0x01, // 00000001
    0x01, // 00000001
    0x7E  // 01111110
};

// --- Enemy Bird Graphics (Example: 5x3, 2 frames like BirdManager) ---
#define ENEMY_BIRD_SPRITE_WIDTH 5
#define ENEMY_BIRD_SPRITE_HEIGHT 3
#define ENEMY_BIRD_FRAME_COUNT 2
#define ENEMY_BIRD_FRAME_DURATION_MS 200
#define ENEMY_BIRD_BYTES_PER_FRAME ((ENEMY_BIRD_SPRITE_WIDTH + 7) / 8 * ENEMY_BIRD_SPRITE_HEIGHT) // = 1 * 3 = 3 bytes

const unsigned char bmp_flappy_enemy_bird[] PROGMEM = {
    0xE0, 0xA0, 0xE0, // Frame 1: >-<
    0x00, 0xE0, 0x00  // Frame 2:  V  (simple)
};

// --- Fragile Pipe Section Graphics (if different from normal pipe) ---
// (For now, fragility will be a state, drawing might just be a different color or pattern)

// --- Pipe-Mounted Hazard Graphics (Example: Small spike 3x3) ---
#define PIPE_HAZARD_WIDTH 3
#define PIPE_HAZARD_HEIGHT 3
const unsigned char bmp_flappy_pipe_hazard[] PROGMEM = {
    0x40, // ..X
    0xE0, // XXX
    0x40  // ..X
};


namespace FlappyTuckGraphics {
    // Helper to get GraphicAssetData instances
    inline GraphicAssetData getCoinAsset() {
        return GraphicAssetData(bmp_flappy_coin, COIN_SPRITE_WIDTH, COIN_SPRITE_HEIGHT);
    }
    inline GraphicAssetData getPowerUpGhostAsset() {
        return GraphicAssetData(bmp_flappy_powerup_ghost, POWERUP_SPRITE_WIDTH, POWERUP_SPRITE_HEIGHT);
    }
    inline GraphicAssetData getPowerUpSlowMoAsset() {
        return GraphicAssetData(bmp_flappy_powerup_slowmo, POWERUP_SPRITE_WIDTH, POWERUP_SPRITE_HEIGHT);
    }
    inline GraphicAssetData getEnemyBirdAsset() {
        return GraphicAssetData(bmp_flappy_enemy_bird, ENEMY_BIRD_SPRITE_WIDTH, ENEMY_BIRD_SPRITE_HEIGHT, ENEMY_BIRD_FRAME_COUNT, ENEMY_BIRD_BYTES_PER_FRAME, ENEMY_BIRD_FRAME_DURATION_MS);
    }
    inline GraphicAssetData getPipeHazardAsset() {
        return GraphicAssetData(bmp_flappy_pipe_hazard, PIPE_HAZARD_WIDTH, PIPE_HAZARD_HEIGHT);
    }
}

#endif // FLAPPY_BIRD_GRAPHICS_H
