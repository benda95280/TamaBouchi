#ifndef EFFECTS_MANAGER_H
#define EFFECTS_MANAGER_H

#include <Arduino.h> 
#include <vector> 
#include <algorithm> // <<< NEW INCLUDE for std::swap (or use a manual swap)

// Forward declaration
class Renderer;
// class U8G2; // U8G2 is accessed via Renderer, so not needed here

class EffectsManager {
public:
    EffectsManager(Renderer& renderer);

    // --- Screen Shake ---
    void triggerScreenShake(int magnitude = 1, unsigned long durationMs = 200);
    void applyEffectOffset(int& xOffset, int& yOffset); 
    bool isShaking() const;

    // --- Fade Out To Black (Persistent Pixel Dissolve Overlay) ---
    void triggerFadeOutToBlack(unsigned long durationMs = 2000);
    bool isFadingOutToBlack() const; 
    bool isFadeOutToBlackCompleted() const; 
    void drawFadeOutOverlay();          
    void resetFadeOut(); 

    void update(unsigned long currentTime); 

private:
    Renderer& _renderer; 

    // Screen Shake State
    bool _isShaking = false;
    unsigned long _shakeEndTime = 0;
    int _shakeMagnitude = 1;
    static const unsigned long SCREEN_SHAKE_DEFAULT_DURATION_MS = 200;

    // Fade Out To Black State
    bool _isFadingOutToBlack = false;
    bool _isFadeOutToBlackCompleted = false; 
    unsigned long _fadeOutToBlackStartTime = 0;
    unsigned long _fadeOutToBlackDurationMs = 2000;
    float _fadeOutTargetCoverage = 0.0f; 

    std::vector<bool> _fadeOverlayMask; 
    int _fadeMaskWidthChunks = 0;
    int _fadeMaskHeightChunks = 0;
    // Consider making FADE_PIXEL_CHUNK_SIZE configurable or slightly larger
    static const int FADE_PIXEL_CHUNK_SIZE = 2; // Try 4 or 8 if still laggy

    // --- NEW MEMBERS FOR FISHER-YATES FADE ---
    std::vector<int> _shuffledChunkIndices;
    int _nextChunkToBlackenIndex = 0;
    // --- END NEW MEMBERS ---
};

#endif // EFFECTS_MANAGER_H