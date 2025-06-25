#ifndef GRAPHIC_ASSET_TYPES_H
#define GRAPHIC_ASSET_TYPES_H

#include <pgmspace.h> // For const unsigned char* PROGMEM
#include <stddef.h>   // For size_t

// This struct is now general and can be used by any system needing bitmap/spritesheet data.
struct GraphicAssetData {
    const unsigned char* bitmap = nullptr;
    int width = 0;
    int height = 0;
    int frameCount = 0;        // For spritesheets
    size_t bytesPerFrame = 0;  // For spritesheets
    unsigned long frameDurationMs = 0; // For spritesheets

    // Default constructor
    GraphicAssetData() = default;

    // Constructor for static bitmaps
    GraphicAssetData(const unsigned char* bmp, int w, int h)
        : bitmap(bmp), width(w), height(h), frameCount(0), bytesPerFrame(0), frameDurationMs(0) {}

    // Constructor for spritesheets
    GraphicAssetData(const unsigned char* sheet, int frameW, int frameH, int fCount, size_t bytesPerFrameVal, unsigned long frameDuration)
        : bitmap(sheet), width(frameW), height(frameH), frameCount(fCount), bytesPerFrame(bytesPerFrameVal), frameDurationMs(frameDuration) {}

    bool isSpritesheet() const { return bitmap != nullptr && frameCount > 1 && bytesPerFrame > 0 && frameDurationMs > 0; }
    bool isValid() const { return bitmap != nullptr && width > 0 && height > 0; }
};

#endif // GRAPHIC_ASSET_TYPES_H