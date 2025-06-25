
// --- START OF FILE GEM_u8g2Rewrite.h ---
#pragma once

#include <GEM_u8g2.h> // Include the base class header

// Forward declaration if needed by GEM internals, unlikely but good practice
class U8G2;

static const unsigned char logo_bits [] U8X8_PROGMEM = {
    0x8f,0x4f,0xf4,0x00,0x40,0xf4,0x00,0x40,0xf5,0x98,0x47,0xf5,
    0x00,0x40,0xf4,0x00,0x40,0xf4,0x9f,0x4f,0xf4,0x00,0x00,0xf0
  };

// GEM_u8g2Rewrite inherits publicly from GEM_u8g2
class GEM_u8g2Rewrite : public GEM_u8g2 {
public:
    // Inherit constructors from the base class
    using GEM_u8g2::GEM_u8g2;

    GEM_u8g2& init() override {
        _u8g2.setDrawColor(1);
        _u8g2.setFontPosTop();
      
        if (_splashDelay > 0) {
        
          _u8g2.drawXBMP((_u8g2.getDisplayWidth() - _splash.width) / 2, (_u8g2.getDisplayHeight() - _splash.height) / 2, _splash.width, _splash.height, _splash.image);

      
          if (_enableVersion) {
            delay(_splashDelay / 2);
              _u8g2.drawXBMP((_u8g2.getDisplayWidth() - _splash.width) / 2, (_u8g2.getDisplayHeight() - _splash.height) / 2, _splash.width, _splash.height, _splash.image);
              _u8g2.setFont(_fontFamilies.small);
              byte x = _u8g2.getDisplayWidth() - strlen(GEM_VER)*4;
              byte y = _u8g2.getDisplayHeight() - 7;
              if (_splash.image != logo_bits) {
                _u8g2.setCursor(x - 12, y);
                _u8g2.print("GEM");
              } else {
                _u8g2.setCursor(x, y);
              }
              _u8g2.print(GEM_VER);
            delay(_splashDelay / 2);
          } else {
            delay(_splashDelay);
          }
      
          _u8g2.clear();
      
        }
      
        return *this;
      }

    // New drawing function that only draws elements into the buffer
    GEM_u8g2Rewrite& drawMenuElements() {
        // Call the internal drawing methods from the base class
        // IMPORTANT: These need to be accessible (protected/public in GEM_u8g2)
        drawTitleBar();
        printMenuItems();
        drawMenuPointer();
        drawScrollbar();

        // Accessing protected/private members might require adjustments
        // depending on how GEM_u8g2 is structured.
        if (drawMenuCallback != nullptr) {
            drawMenuCallback();
        }
        return *this; // Return reference to self
    }

    // --- Override drawMenu ---
    // Override the original drawMenu. Now, any internal call to drawMenu()
    // within the base class (like after registerKeyPress) will execute this
    // function instead.
    GEM_u8g2& drawMenu() override {
        // Instead of doing the full clear/draw/send cycle,
        // just draw the elements into the current buffer.
        drawMenuElements();
        return *this;
    }
    // --- End Override ---
};

// --- END OF FILE GEM_u8g2Rewrite.h ---


