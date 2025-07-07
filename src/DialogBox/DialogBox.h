#ifndef DIALOG_BOX_H
#define DIALOG_BOX_H
#include <Arduino.h>
#include <vector>
#include "Renderer.h" // Needs Renderer reference
#include <U8g2lib.h>  // Needs U8g2 pointer
class DialogBox
{
public:
    DialogBox(Renderer &renderer);
    ~DialogBox(); // Add destructor to free memory

    // Set the content and activate the dialog (permanent)
    // Clears previous content. Use addText for multi-line.
    void show(const char *initialMessage = "");

    // --- NEW: Show temporary message ---
    // Displays the message for a fixed duration, then closes automatically.
    // If the message has more lines than visible, it will auto-scroll.
    void showTemporary(const char *message, unsigned long durationMs = 3000);

    // Add a line of text (handles '\n' for multiple lines)
    // If the dialog is currently showing a temporary message, adding text
    // will make it permanent and stop auto-scrolling.
    // Returns *this to allow chaining.
    DialogBox &addText(const char *text);

    // Update handles temporary message timeout, dot animation, and auto-scrolling
    void update(unsigned long currentTime); // Pass current time

    // Draw the dialog box, scrollbar, arrows, and visible text
    // Now supports <b>bold</b> and <u>underline</u> tags.
    void draw();

    // Manually close the dialog
    void close();

    // Check if the dialog is currently active
    bool isActive() const;

    // --- NEW: Check if the dialog is temporary ---
    bool isTemporary() const;

    // Scroll control methods
    void scrollUp(int lines = 1);
    void scrollDown(int lines = 1);
    bool isAtBottom() const;
    bool isAtTop() const; // Added for completeness
private:
    Renderer &_renderer;
    // --- MODIFIED: From fixed char arrays to a dynamically allocated buffer ---
    static const int MAX_DIALOG_LINES = 15;
    static const int MAX_LINE_LENGTH = 40; // Includes null terminator
    char* _lineBuffers = nullptr; // Pointer for dynamic allocation
    int _numActiveLines = 0;
    int _lineCapacity = 0; // To track if buffer is allocated
    // --- END MODIFIED ---

    int _topLineIndex = 0;
    int _visibleLines = 0;
    bool _active = false;

    // Calculated layout properties
    int _boxX = 0;
    int _boxY = 0;
    int _boxW = 0;
    int _boxH = 0;
    int _textX = 0;
    int _textY = 0;
    int _lineHeight = 0;
    int _scrollbarX = 0;
    int _scrollbarY = 0;
    int _scrollbarH = 0;

    // Configuration
    const uint8_t *_font = u8g2_font_5x7_tf;
    int _padding = 4;
    int _scrollbarWidth = 2;
    int _cornerRadius = 3;
    int _textStartYOffset = 1;

    // Scroll state indicators
    bool _canScrollUp = false;
    bool _canScrollDown = false;

    // Temporary Message State
    bool _isTemporary = false;
    unsigned long _tempMessageEndTime = 0;
    int _dotAnimationState = 0;
    unsigned long _lastDotUpdateTime = 0;
    static const unsigned long DOT_ANIMATION_INTERVAL_MS = 500;

    // --- Auto-scroll for temporary messages ---
    bool _isAutoScrolling = false;
    unsigned long _lastAutoScrollTime = 0;
    unsigned long _dynamicScrollIntervalMs = 1500;
    static const unsigned long MIN_AUTO_SCROLL_INTERVAL_MS = 500;
    static const unsigned long MAX_AUTO_SCROLL_INTERVAL_MS = 3000;
    // --- End Auto-scroll ---

    // Internal helpers
    void calculateLayout();
    void updateScrollability();
    void drawScrollbar();
    void drawScrollArrows();
    void drawTemporaryDots();
    void processAndAddLine(const String &textLine); // Still takes String for ease of splitting, internal storage is char[]
    void _setInitialContent(const char *text);
    void drawFormattedLine(const char *lineStr, int yPos, int clipRightX);

    // --- NEW: Dynamic buffer management ---
    void allocateLineBuffers();
    void deallocateLineBuffers();
};
#endif // DIALOG_BOX_H