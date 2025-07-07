#include "DialogBox.h"
#include <algorithm>         // For std::max, std::min
#include <vector>            // Include vector for word splitting
#include "SerialForwarder.h" // Include for logging
#include "../DebugUtils.h"   // Include for debug utilities
#include <cstring>           // For strncpy, strlen, strncmp
// Forward declare global logger pointer
extern SerialForwarder *forwardedSerial_ptr;
// Constants for scroll arrow rendering
const int ARROW_HEIGHT = 5;
const int ARROW_BASE_WIDTH_DIVISOR = 2;
const int ARROW_VERTICAL_MARGIN = 2;
const int ARROW_TOTAL_SPACE_PER_END = ARROW_HEIGHT + ARROW_VERTICAL_MARGIN;
// --- Helper to split string by space ---
void splitStringToWords(const String &text, std::vector<String> &words)
{
    int lastSpace = -1;
    for (int i = 0; i < text.length(); ++i)
    {
        if (text.charAt(i) == ' ')
        {
            if (i > lastSpace + 1)
            {
                words.push_back(text.substring(lastSpace + 1, i));
            }
            lastSpace = i;
        }
    }
    if (lastSpace + 1 < text.length())
    {
        words.push_back(text.substring(lastSpace + 1));
    }
}
// --- Helper to split string by newline ---
void splitStringByNewline(const String &text, std::vector<String> &lines)
{
    int lastPos = 0;
    for (int i = 0; i < text.length(); ++i)
    {
        if (text.charAt(i) == '\n' || text.charAt(i) == '\r')
        {
            lines.push_back(text.substring(lastPos, i));
            if (text.charAt(i) == '\r' && i + 1 < text.length() && text.charAt(i + 1) == '\n')
            {
                i++;
            }
            lastPos = i + 1;
        }
    }
    if (lastPos < text.length())
    {
        lines.push_back(text.substring(lastPos));
    }
    if (text.isEmpty() && lines.empty())
    {
        lines.push_back("");
    }
}
DialogBox::DialogBox(Renderer &renderer) : _renderer(renderer)
{
    calculateLayout();
}
DialogBox::~DialogBox()
{
    deallocateLineBuffers();
}

// --- NEW: Dynamic Buffer Management ---
void DialogBox::allocateLineBuffers()
{
    if (_lineBuffers != nullptr)
    {
        // Already allocated, just reset the line count
        _numActiveLines = 0;
        return;
    }
    debugPrint("DIALOG_BOX", "Allocating line buffers (600 bytes).");
    _lineBuffers = new (std::nothrow) char[MAX_DIALOG_LINES * MAX_LINE_LENGTH];
    if (_lineBuffers == nullptr) {
        debugPrint("DIALOG_BOX", "FATAL: Failed to allocate memory for DialogBox line buffers!");
        _lineCapacity = 0;
        return;
    }
    _lineCapacity = MAX_DIALOG_LINES;
    _numActiveLines = 0;
}
void DialogBox::deallocateLineBuffers()
{
    if (_lineBuffers != nullptr)
    {
        debugPrint("DIALOG_BOX", "Deallocating line buffers.");
        delete[] _lineBuffers;
        _lineBuffers = nullptr;
    }
    _lineCapacity = 0;
    _numActiveLines = 0;
}
// --- END NEW ---

void DialogBox::calculateLayout()
{
    U8G2 *u8g2 = _renderer.getU8G2();
    if (!u8g2)
        return;

    _boxW = _renderer.getWidth();
    _boxH = _renderer.getHeight() / 2;
    _boxX = _renderer.getXOffset();
    _boxY = _renderer.getYOffset() + _renderer.getHeight() - _boxH;

    u8g2->setFont(_font);
    _lineHeight = u8g2->getMaxCharHeight() + 1;
    if (_lineHeight <= 0)
        _lineHeight = 8;

    int textContentAreaYStart = _boxY + _padding + _textStartYOffset;
    int textContentAreaYEnd = _boxY + _boxH - _padding;
    int availableTextH = textContentAreaYEnd - textContentAreaYStart;
    _visibleLines = (availableTextH > 0 && _lineHeight > 0) ? availableTextH / _lineHeight : 0;

    _scrollbarX = _boxX + _boxW - _scrollbarWidth - _padding - 2;

    _scrollbarY = _boxY + _padding + ARROW_TOTAL_SPACE_PER_END;
    _scrollbarH = _boxH - (2 * _padding) - (2 * ARROW_TOTAL_SPACE_PER_END);
    if (_scrollbarH < 5)
        _scrollbarH = 5;

    _textX = _boxX + _padding;
    _textY = textContentAreaYStart;

    updateScrollability();
}
void DialogBox::updateScrollability()
{
    if (!_active || _numActiveLines <= _visibleLines)
    {
        _canScrollUp = false;
        _canScrollDown = false;
    }
    else
    {
        _canScrollUp = (_topLineIndex > 0);
        _canScrollDown = (_topLineIndex < _numActiveLines - _visibleLines);
    }
}
void DialogBox::_setInitialContent(const char *text)
{
    // Buffers are now allocated on demand, but this logic stays mostly the same
    _numActiveLines = 0; 
    if (!_lineBuffers) { // Should have been allocated by show()
        debugPrint("DIALOG_BOX", "Error: _setInitialContent called before buffers were allocated!");
        return;
    }

    if (!text || text[0] == '\0')
    {
        if (_numActiveLines < _lineCapacity)
        {                                           
            _lineBuffers[_numActiveLines * MAX_LINE_LENGTH] = '\0'; // Store an empty string
            _numActiveLines++;
        }
        updateScrollability();
        return;
    }
    String inputText = text; 
    std::vector<String> inputLines;
    splitStringByNewline(inputText, inputLines);

    for (const String &line : inputLines)
    {
        if (_numActiveLines >= _lineCapacity)
        {
            debugPrint("DIALOG_BOX", "Warning: MAX_DIALOG_LINES reached in _setInitialContent. Truncating.");
            break;
        }
        processAndAddLine(line); // processAndAddLine now populates _lineBuffers
    }
    _topLineIndex = 0;
    updateScrollability();
}
void DialogBox::show(const char *initialMessage)
{
    if (_active)
    {
        debugPrint("DIALOG_BOX", "show called while already active. Ignoring.");
        return;
    }
    
    allocateLineBuffers(); // MODIFIED: Allocate memory on show
    if (_lineCapacity == 0) return; // Allocation failed

    debugPrint("DIALOG_BOX", "Showing permanent message.");
    _isTemporary = false;
    _isAutoScrolling = false;
    _active = true;
    _setInitialContent(initialMessage);
    calculateLayout();
}
void DialogBox::showTemporary(const char *message, unsigned long durationMs)
{
    if (_active)
    {
        debugPrint("DIALOG_BOX", "showTemporary called while already active. Ignoring.");
        return;
    }

    allocateLineBuffers(); // MODIFIED: Allocate memory on show
    if (_lineCapacity == 0) return; // Allocation failed

    debugPrintf("DIALOG_BOX", "Showing temporary message for %lu ms", durationMs);
    _isTemporary = true;
    _active = true;
    _setInitialContent(message);
    _tempMessageEndTime = millis() + durationMs;
    _dotAnimationState = 0;
    _lastDotUpdateTime = millis();
    calculateLayout();

    if (_numActiveLines > _visibleLines && _canScrollDown)
    {
        _isAutoScrolling = true;
        _lastAutoScrollTime = millis();
        int linesToScroll = _numActiveLines - _visibleLines;
        if (linesToScroll > 0 && durationMs > 0)
        {
            unsigned long remainingDurationForScrolling = durationMs > MIN_AUTO_SCROLL_INTERVAL_MS ? durationMs - MIN_AUTO_SCROLL_INTERVAL_MS : 0;
            if (remainingDurationForScrolling > 0)
            {
                _dynamicScrollIntervalMs = remainingDurationForScrolling / linesToScroll;
            }
            else
            {
                _dynamicScrollIntervalMs = durationMs;
            }
            _dynamicScrollIntervalMs = std::max(MIN_AUTO_SCROLL_INTERVAL_MS, _dynamicScrollIntervalMs);
            _dynamicScrollIntervalMs = std::min(MAX_AUTO_SCROLL_INTERVAL_MS, _dynamicScrollIntervalMs);
        }
        else
        {
            _dynamicScrollIntervalMs = 1500;
        }
        debugPrintf("DIALOG_BOX", "Temporary message auto-scroll enabled. Interval: %lu ms", _dynamicScrollIntervalMs);
    }
    else
    {
        _isAutoScrolling = false;
        _dynamicScrollIntervalMs = 1500;
    }
}
void DialogBox::processAndAddLine(const String &textLine)
{
    U8G2 *u8g2 = _renderer.getU8G2();
    if (!u8g2 || !_lineBuffers)
    {
        if (_numActiveLines < _lineCapacity)
        {
            strncpy(&_lineBuffers[_numActiveLines * MAX_LINE_LENGTH], textLine.c_str(), MAX_LINE_LENGTH - 1);
            _lineBuffers[(_numActiveLines * MAX_LINE_LENGTH) + MAX_LINE_LENGTH - 1] = '\0';
            _numActiveLines++;
        }
        else
        {
            debugPrint("DIALOG_BOX", "Warning: MAX_DIALOG_LINES reached in processAndAddLine (no u8g2/buffer). Truncating.");
        }
        return;
    }

    u8g2->setFont(_font);
    int maxTextWidth = _boxW - (3 * _padding) - _scrollbarWidth - 2;
    if (maxTextWidth <= 0)
    {
        if (_numActiveLines < _lineCapacity)
        {
            strncpy(&_lineBuffers[_numActiveLines * MAX_LINE_LENGTH], textLine.c_str(), MAX_LINE_LENGTH - 1);
            _lineBuffers[(_numActiveLines * MAX_LINE_LENGTH) + MAX_LINE_LENGTH - 1] = '\0';
            _numActiveLines++;
        }
        else
        {
            debugPrint("DIALOG_BOX", "Warning: MAX_DIALOG_LINES reached in processAndAddLine (no text width). Truncating.");
        }
        return;
    }

    if (textLine.isEmpty())
    {
        if (_numActiveLines < _lineCapacity)
        {
            _lineBuffers[_numActiveLines * MAX_LINE_LENGTH] = '\0';
            _numActiveLines++;
        }
        else
        {
            debugPrint("DIALOG_BOX", "Warning: MAX_DIALOG_LINES reached while adding empty line. Truncating.");
        }
        return;
    }

    String currentWrappedLine = "";
    String remainingText = textLine;

    while (remainingText.length() > 0)
    {
        if (_numActiveLines >= _lineCapacity)
        { 
            debugPrint("DIALOG_BOX", "Warning: MAX_DIALOG_LINES reached during line wrapping. Truncating.");
            break;
        }

        if (u8g2->getStrWidth(remainingText.c_str()) <= maxTextWidth)
        {
            currentWrappedLine = remainingText;
            remainingText = "";
        }
        else
        {
            int wrapAt = -1;
            for (int i = remainingText.length() - 1; i >= 0; --i)
            {
                if (u8g2->getStrWidth(remainingText.substring(0, i).c_str()) <= maxTextWidth)
                {
                    int lastSpace = remainingText.substring(0, i).lastIndexOf(' ');
                    if (lastSpace != -1 && u8g2->getStrWidth(remainingText.substring(0, lastSpace).c_str()) > 0)
                    {
                        wrapAt = lastSpace;
                    }
                    else
                    {
                        wrapAt = i;
                    }
                    break;
                }
            }
            if (wrapAt <= 0)
            {
                for (int i = 1; i <= remainingText.length(); ++i)
                {
                    if (u8g2->getStrWidth(remainingText.substring(0, i).c_str()) > maxTextWidth)
                    {
                        wrapAt = i - 1;
                        break;
                    }
                }
                if (wrapAt <= 0)
                    wrapAt = remainingText.length();
            }
            currentWrappedLine = remainingText.substring(0, wrapAt);
            remainingText = remainingText.substring(wrapAt);
            remainingText.trim();
        }
        // MODIFIED: Use new buffer access method
        char* currentLinePtr = &_lineBuffers[_numActiveLines * MAX_LINE_LENGTH];
        strncpy(currentLinePtr, currentWrappedLine.c_str(), MAX_LINE_LENGTH - 1);
        currentLinePtr[MAX_LINE_LENGTH - 1] = '\0';
        _numActiveLines++;
        currentWrappedLine = "";
    }
}
DialogBox &DialogBox::addText(const char *text)
{
    if (!text || !_lineBuffers)
        return *this;
    if (_isTemporary)
    {
        debugPrint("DIALOG_BOX", "Converting temporary to permanent due to addText. Auto-scroll disabled.");
        _isTemporary = false;
        _isAutoScrolling = false;
    }

    String inputText = text; 
    std::vector<String> inputLines;
    splitStringByNewline(inputText, inputLines);

    for (const String &line : inputLines)
    {
        if (_numActiveLines >= _lineCapacity)
        { 
            debugPrint("DIALOG_BOX", "Warning: MAX_DIALOG_LINES reached in addText. Truncating.");
            break;
        }
        processAndAddLine(line);
    }
    _topLineIndex = std::max(0, std::min(_topLineIndex, _numActiveLines - _visibleLines));
    updateScrollability();
    if (!_isTemporary)
    {
        _isAutoScrolling = false;
    }
    return *this;
}
void DialogBox::update(unsigned long currentTime)
{
    if (!_active)
        return;

    if (_isTemporary)
    {
        bool autoScrollJustFinishedReading = false;
        if (_isAutoScrolling == false && _canScrollDown == false && _lastAutoScrollTime != 0)
        {
            if (currentTime < _tempMessageEndTime && currentTime < _lastAutoScrollTime + _dynamicScrollIntervalMs)
            {
                autoScrollJustFinishedReading = true;
            }
        }

        if (!autoScrollJustFinishedReading && currentTime >= _tempMessageEndTime)
        {
            debugPrint("DIALOG_BOX", "Temporary message timed out. Closing.");
            close();
            return;
        }

        if (currentTime - _lastDotUpdateTime >= DOT_ANIMATION_INTERVAL_MS)
        {
            _lastDotUpdateTime = currentTime;
            _dotAnimationState = (_dotAnimationState + 1) % 3;
        }

        if (_isAutoScrolling && _canScrollDown)
        {
            if (currentTime - _lastAutoScrollTime >= _dynamicScrollIntervalMs)
            {
                _topLineIndex++;
                int maxTopIndex = (_numActiveLines > _visibleLines) ? (_numActiveLines - _visibleLines) : 0;
                _topLineIndex = std::min(std::max(0, maxTopIndex), _topLineIndex);
                updateScrollability();

                _lastAutoScrollTime = currentTime;
                debugPrintf("DIALOG_BOX", "Auto-scrolled. New top: %d", _topLineIndex);

                if (!_canScrollDown)
                {
                    _isAutoScrolling = false;
                    _tempMessageEndTime = std::max(_tempMessageEndTime, currentTime + _dynamicScrollIntervalMs);
                    debugPrintf("DIALOG_BOX", "Auto-scroll reached bottom. Extended end time to: %lu", _tempMessageEndTime);
                }
            }
        }
        else if (_isAutoScrolling && !_canScrollDown)
        {
            _isAutoScrolling = false;
            if (_lastAutoScrollTime != 0)
            {
                _tempMessageEndTime = std::max(_tempMessageEndTime, currentTime + _dynamicScrollIntervalMs);
                debugPrintf("DIALOG_BOX", "Auto-scroll stopped (no more scroll). Extended end time to: %lu", _tempMessageEndTime);
                _lastAutoScrollTime = 0;
            }
        }
    }
}
void DialogBox::drawScrollbar()
{
    U8G2 *u8g2 = _renderer.getU8G2();
    if (!u8g2)
        return;

    u8g2->setDrawColor(0);
    u8g2->drawBox(_scrollbarX - 1, _scrollbarY - 1, _scrollbarWidth + 2, _scrollbarH + 2);
    u8g2->setDrawColor(1);
    u8g2->drawBox(_scrollbarX, _scrollbarY, _scrollbarWidth, _scrollbarH);

    if (_canScrollUp || _canScrollDown)
    {
        int totalLines = _numActiveLines;
        int scrollRange = totalLines - _visibleLines;
        if (scrollRange <= 0)
            return;

        int minHandleHeight = 5;
        int handleHeight = std::max(minHandleHeight, (_scrollbarH * _visibleLines) / totalLines);
        handleHeight = std::min(handleHeight, _scrollbarH);

        int handleY = _scrollbarY + ((_scrollbarH - handleHeight) * _topLineIndex) / scrollRange;
        handleY = std::min(_scrollbarY + _scrollbarH - handleHeight, handleY);
        handleY = std::max(_scrollbarY, handleY);

        u8g2->setDrawColor(0);
        u8g2->drawBox(_scrollbarX, handleY, _scrollbarWidth, handleHeight);
    }
}
void DialogBox::drawScrollArrows()
{
    U8G2 *u8g2 = _renderer.getU8G2();
    if (!u8g2)
        return;

    int arrowCenterX = _scrollbarX + _scrollbarWidth / 2;
    int arrowHalfBase = ARROW_HEIGHT / ARROW_BASE_WIDTH_DIVISOR;

    // Up Arrow
    int upArrowTipY = _boxY + _padding + ARROW_VERTICAL_MARGIN;
    int upArrowBaseY = upArrowTipY + ARROW_HEIGHT;
    if (_canScrollUp)
    {
        u8g2->setDrawColor(0);
        u8g2->drawTriangle(arrowCenterX - arrowHalfBase, upArrowBaseY,
                           arrowCenterX + arrowHalfBase, upArrowBaseY,
                           arrowCenterX, upArrowTipY);
    }
    else
    {
        u8g2->setDrawColor(1);
        u8g2->drawTriangle(arrowCenterX - arrowHalfBase, upArrowBaseY,
                           arrowCenterX + arrowHalfBase, upArrowBaseY,
                           arrowCenterX, upArrowTipY);
        u8g2->setDrawColor(0);
        u8g2->drawTriangle(arrowCenterX - arrowHalfBase, upArrowBaseY,
                           arrowCenterX + arrowHalfBase, upArrowBaseY,
                           arrowCenterX, upArrowTipY);
    }

    // Down Arrow
    int downArrowBaseY = _boxY + _boxH - _padding - ARROW_VERTICAL_MARGIN;
    int downArrowTipY = downArrowBaseY - ARROW_HEIGHT;
    if (_canScrollDown)
    {
        u8g2->setDrawColor(0);
        u8g2->drawTriangle(arrowCenterX - arrowHalfBase, downArrowTipY,
                           arrowCenterX + arrowHalfBase, downArrowTipY,
                           arrowCenterX, downArrowBaseY);
    }
    else
    {
        u8g2->setDrawColor(1);
        u8g2->drawTriangle(arrowCenterX - arrowHalfBase, downArrowTipY,
                           arrowCenterX + arrowHalfBase, downArrowTipY,
                           arrowCenterX, downArrowBaseY);
        u8g2->setDrawColor(0);
        u8g2->drawTriangle(arrowCenterX - arrowHalfBase, downArrowTipY,
                           arrowCenterX + arrowHalfBase, downArrowTipY,
                           arrowCenterX, downArrowBaseY);
    }
}
void DialogBox::drawTemporaryDots()
{
    U8G2 *u8g2 = _renderer.getU8G2();
    if (!u8g2)
        return;

    const char *dots;
    switch (_dotAnimationState)
    {
    case 0:
        dots = ".";
        break;
    case 1:
        dots = "..";
        break;
    case 2:
    default:
        dots = "...";
        break;
    }

    u8g2->setFont(_font);
    int dotsWidth = u8g2->getStrWidth(dots);
    int dotsX = _scrollbarX - _padding - dotsWidth - 1;
    int dotsY = _boxY + _boxH - _padding - _lineHeight + _textStartYOffset + 1 - 2;

    u8g2->setDrawColor(0);
    u8g2->setFontPosTop();
    u8g2->drawStr(dotsX, dotsY, dots);
}
void DialogBox::drawFormattedLine(const char *lineStr, int yPos, int clipRightX)
{
    U8G2 *u8g2 = _renderer.getU8G2();
    if (!u8g2 || !lineStr)
        return;

    int currentX = _textX;
    bool isBold = false;
    bool isUnderline = false;

    // Temporary buffer for segments between tags
    char segmentBuffer[MAX_LINE_LENGTH];
    int segmentLen = 0;

    u8g2->setClipWindow(_textX, yPos, clipRightX, yPos + _lineHeight);

    for (unsigned int i = 0; lineStr[i] != '\0'; ++i)
    {
        bool tagFound = false;
        // Check for tags
        if (strncmp(lineStr + i, "<b>", 3) == 0)
        {
            tagFound = true;
            isBold = true;
            i += 2;
        }
        else if (strncmp(lineStr + i, "</b>", 4) == 0)
        {
            tagFound = true;
            isBold = false;
            i += 3;
        }
        else if (strncmp(lineStr + i, "<u>", 3) == 0)
        {
            tagFound = true;
            isUnderline = true;
            i += 2;
        }
        else if (strncmp(lineStr + i, "</u>", 4) == 0)
        {
            tagFound = true;
            isUnderline = false;
            i += 3;
        }

        if (tagFound)
        { // If a tag was processed, draw the preceding segment
            if (segmentLen > 0)
            {
                segmentBuffer[segmentLen] = '\0'; // Null-terminate the segment
                u8g2->drawStr(currentX, yPos, segmentBuffer);
                int segmentWidth = u8g2->getStrWidth(segmentBuffer);
                if (isBold && !tagFound)
                    u8g2->drawStr(currentX + 1, yPos, segmentBuffer); // Apply bold only if it was active for this segment
                if (isUnderline && !tagFound)
                    u8g2->drawHLine(currentX, yPos + _lineHeight - 2, segmentWidth);
                currentX += segmentWidth;
                segmentLen = 0; // Reset segment buffer
            }
        }
        else
        { // Not a tag, add character to current segment
            if (segmentLen < MAX_LINE_LENGTH - 1)
            {
                segmentBuffer[segmentLen++] = lineStr[i];
            }
        }
    }

    // Draw any remaining segment
    if (segmentLen > 0)
    {
        segmentBuffer[segmentLen] = '\0';
        u8g2->drawStr(currentX, yPos, segmentBuffer);
        int segmentWidth = u8g2->getStrWidth(segmentBuffer);
        if (isBold)
            u8g2->drawStr(currentX + 1, yPos, segmentBuffer);
        if (isUnderline)
            u8g2->drawHLine(currentX, yPos + _lineHeight - 2, segmentWidth);
    }
    u8g2->setMaxClipWindow(); // Reset clip window
}
void DialogBox::draw()
{
    if (!_active || !_lineBuffers)
        return;
    U8G2 *u8g2 = _renderer.getU8G2();
    if (!u8g2)
        return;

    uint8_t originalColor = u8g2->getDrawColor();

    u8g2->setDrawColor(1);
    u8g2->drawRBox(_boxX, _boxY, _boxW, _boxH, _cornerRadius);
    u8g2->setDrawColor(0);
    u8g2->drawRFrame(_boxX, _boxY, _boxW, _boxH, _cornerRadius);
    u8g2->drawRFrame(_boxX + 2, _boxY + 2, _boxW - 4, _boxH - 4, _cornerRadius);

    u8g2->setDrawColor(0);
    u8g2->setFont(_font);
    u8g2->setFontPosTop();

    int currentY = _textY;
    int linesToDraw = std::min(_numActiveLines - _topLineIndex, _visibleLines);
    int textClipRight = _scrollbarX - _padding;

    for (int i = 0; i < linesToDraw; ++i)
    {
        int lineIndex = _topLineIndex + i;
        if (lineIndex >= 0 && lineIndex < _numActiveLines)
        {
            // MODIFIED: Use new buffer access method
            const char* currentLinePtr = &_lineBuffers[lineIndex * MAX_LINE_LENGTH];
            drawFormattedLine(currentLinePtr, currentY, textClipRight);
            currentY += _lineHeight;
        }
    }

    drawScrollbar();
    drawScrollArrows();

    if (_isTemporary)
    {
        drawTemporaryDots();
    }

    u8g2->setDrawColor(originalColor);
}
bool DialogBox::isActive() const { return _active; }
bool DialogBox::isTemporary() const { return _active && _isTemporary; }
void DialogBox::close()
{
    debugPrint("DIALOG_BOX", "Closing.");
    _active = false;
    _isTemporary = false;
    _isAutoScrolling = false;
    _topLineIndex = 0;
    updateScrollability();
    _lastAutoScrollTime = 0;
    
    deallocateLineBuffers(); // MODIFIED: Free memory on close
}
void DialogBox::scrollUp(int lines)
{
    if (!_active || !_canScrollUp)
        return;
    debugPrint("DIALOG_BOX", "Scrolling Up (manual or programmatic).");
    _topLineIndex -= lines;
    _topLineIndex = std::max(0, _topLineIndex);
    updateScrollability();
    if (_isTemporary && _isAutoScrolling)
    {
        _isAutoScrolling = false;
        debugPrint("DIALOG_BOX", "Manual scroll intervention, auto-scroll disabled.");
    }
}
void DialogBox::scrollDown(int lines)
{
    if (!_active || !_canScrollDown)
        return;
    debugPrint("DIALOG_BOX", "Scrolling Down (manual or programmatic).");
    _topLineIndex += lines;
    int maxTopIndex = (_numActiveLines > _visibleLines) ? (_numActiveLines - _visibleLines) : 0;
    _topLineIndex = std::min(std::max(0, maxTopIndex), _topLineIndex);
    updateScrollability();
    if (_isTemporary && _isAutoScrolling)
    {
        _isAutoScrolling = false;
        debugPrint("DIALOG_BOX", "Manual scroll intervention, auto-scroll disabled.");
    }
}
bool DialogBox::isAtBottom() const { return !_canScrollDown; }
bool DialogBox::isAtTop() const { return !_canScrollUp; }