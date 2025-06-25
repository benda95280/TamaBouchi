#include "EffectsManager.h"
#include "Renderer.h" 
#include "SerialForwarder.h" 
#include <U8g2lib.h> 
#include <algorithm> // For std::min, std::max, std::swap
#include <vector>    
#include "../DebugUtils.h"

// Access global logger (ensure this is correctly defined/initialized in your project)
// extern SerialForwarder* forwardedSerial_ptr; // No longer directly used here, use debugPrintf

EffectsManager::EffectsManager(Renderer& renderer) : 
    _renderer(renderer),
    _isShaking(false),
    _shakeEndTime(0),
    _shakeMagnitude(1),
    _isFadingOutToBlack(false),
    _isFadeOutToBlackCompleted(false), 
    _fadeOutToBlackStartTime(0),
    _fadeOutToBlackDurationMs(2000),
    _fadeOutTargetCoverage(0.0f),
    _nextChunkToBlackenIndex(0) // Initialize new member
{
    _fadeMaskWidthChunks = (_renderer.getWidth() + FADE_PIXEL_CHUNK_SIZE - 1) / FADE_PIXEL_CHUNK_SIZE;
    _fadeMaskHeightChunks = (_renderer.getHeight() + FADE_PIXEL_CHUNK_SIZE - 1) / FADE_PIXEL_CHUNK_SIZE;
    
    if (_fadeMaskWidthChunks > 0 && _fadeMaskHeightChunks > 0) {
        _fadeOverlayMask.resize(_fadeMaskWidthChunks * _fadeMaskHeightChunks, false); 
        _shuffledChunkIndices.resize(_fadeMaskWidthChunks * _fadeMaskHeightChunks); // Resize shuffled indices too
    } else {
        // Handle edge case where dimensions are too small for any chunks
        _fadeMaskWidthChunks = 0;
        _fadeMaskHeightChunks = 0;
    }
    debugPrintf("EFFECTS_MANAGER", "EffectsManager: Fade mask initialized to %d x %d chunks.\n", _fadeMaskWidthChunks, _fadeMaskHeightChunks);
}

void EffectsManager::triggerScreenShake(int magnitude, unsigned long durationMs) {
    _isShaking = true;
    _shakeEndTime = millis() + durationMs;
    _shakeMagnitude = magnitude > 0 ? magnitude : 1; 
    debugPrintf("EFFECTS_MANAGER", "EffectsManager: Screen Shake Triggered (Mag: %d, Dur: %lu ms)\n", _shakeMagnitude, durationMs);
}

void EffectsManager::applyEffectOffset(int& xOffset, int& yOffset) {
    if (_isShaking) {
        xOffset += random(-_shakeMagnitude, _shakeMagnitude + 1);
        yOffset += random(-_shakeMagnitude, _shakeMagnitude + 1);
    }
}

bool EffectsManager::isShaking() const {
    return _isShaking;
}

void EffectsManager::resetFadeOut() {
    _isFadingOutToBlack = false;
    _isFadeOutToBlackCompleted = false; 
    _fadeOutTargetCoverage = 0.0f;
    if (!_fadeOverlayMask.empty()) { // Only fill if not empty
        std::fill(_fadeOverlayMask.begin(), _fadeOverlayMask.end(), false);
    }
    _nextChunkToBlackenIndex = 0; // Reset shuffle progress
    _shuffledChunkIndices.clear(); // Clear if not needed or resize if always used
    debugPrint("EFFECTS_MANAGER", "EffectsManager: FadeOutToBlack reset.\n");
}

void EffectsManager::triggerFadeOutToBlack(unsigned long durationMs) {
    _isFadingOutToBlack = true;
    _isFadeOutToBlackCompleted = false; 
    _fadeOutToBlackDurationMs = durationMs > 0 ? durationMs : 1; 
    _fadeOutToBlackStartTime = millis();
    _fadeOutTargetCoverage = 0.0f; 
    
    if (!_fadeOverlayMask.empty()) {
        std::fill(_fadeOverlayMask.begin(), _fadeOverlayMask.end(), false); 
    }

    // --- Initialize and Shuffle Indices ---
    int totalChunks = _fadeMaskWidthChunks * _fadeMaskHeightChunks;
    if (totalChunks > 0) {
        _shuffledChunkIndices.resize(totalChunks);
        for(int i = 0; i < totalChunks; ++i) {
            _shuffledChunkIndices[i] = i;
        }
        // Fisher-Yates Shuffle
        for (int i = totalChunks - 1; i > 0; --i) {
            int j = random(0, i + 1); // random index from 0 to i
            std::swap(_shuffledChunkIndices[i], _shuffledChunkIndices[j]);
        }
    } else {
        _shuffledChunkIndices.clear();
    }
    _nextChunkToBlackenIndex = 0;
    // --- End Shuffle ---
    
    debugPrintf("EFFECTS_MANAGER", "EffectsManager: FadeOutToBlack Triggered (Dur: %lu ms, %d total chunks)\n", _fadeOutToBlackDurationMs, totalChunks);
}

bool EffectsManager::isFadingOutToBlack() const {
    return _isFadingOutToBlack && !_isFadeOutToBlackCompleted;
}

bool EffectsManager::isFadeOutToBlackCompleted() const { 
    return _isFadeOutToBlackCompleted; 
}

void EffectsManager::drawFadeOutOverlay() {
    if (!_isFadingOutToBlack || _fadeOverlayMask.empty()) { 
        return;
    }

    U8G2* u8g2 = _renderer.getU8G2();
    if (!u8g2) return;

    uint8_t originalColor = u8g2->getDrawColor();
    u8g2->setDrawColor(0); // Typically black for fade out

    int renderXOffset = _renderer.getXOffset();
    int renderYOffset = _renderer.getYOffset();

    for (int y_chunk = 0; y_chunk < _fadeMaskHeightChunks; ++y_chunk) {
        for (int x_chunk = 0; x_chunk < _fadeMaskWidthChunks; ++x_chunk) {
            if (_fadeOverlayMask[y_chunk * _fadeMaskWidthChunks + x_chunk]) { 
                u8g2->drawBox(renderXOffset + x_chunk * FADE_PIXEL_CHUNK_SIZE,
                              renderYOffset + y_chunk * FADE_PIXEL_CHUNK_SIZE,
                              FADE_PIXEL_CHUNK_SIZE, FADE_PIXEL_CHUNK_SIZE);
            }
        }
    }
    u8g2->setDrawColor(originalColor); 
}


void EffectsManager::update(unsigned long currentTime) {
    if (_isShaking && currentTime >= _shakeEndTime) {
        _isShaking = false;
    }

    if (_isFadingOutToBlack && !_isFadeOutToBlackCompleted) {
        float currentTargetCoveragePercent = 0.0f;
        if (_fadeOutToBlackDurationMs == 0) {
             currentTargetCoveragePercent = 1.0f;
        } else {
            float progress = (float)(currentTime - _fadeOutToBlackStartTime) / _fadeOutToBlackDurationMs;
            currentTargetCoveragePercent = std::max(0.0f, std::min(1.0f, progress));
        }
        _fadeOutTargetCoverage = currentTargetCoveragePercent; 

        int totalChunks = _fadeMaskWidthChunks * _fadeMaskHeightChunks;
        if (totalChunks == 0) { 
            _isFadeOutToBlackCompleted = true; 
            return;
        }

        int targetTotalBlackChunks = static_cast<int>(currentTargetCoveragePercent * totalChunks);
        // _nextChunkToBlackenIndex already tracks how many are black from the shuffle
        int chunksToTurnBlackThisUpdate = targetTotalBlackChunks - _nextChunkToBlackenIndex;

        if (chunksToTurnBlackThisUpdate > 0 && _nextChunkToBlackenIndex < totalChunks) {
            for (int k = 0; k < chunksToTurnBlackThisUpdate; ++k) {
                if (_nextChunkToBlackenIndex >= totalChunks) break; // Should not happen if targetTotalBlackChunks is correct
                _fadeOverlayMask[_shuffledChunkIndices[_nextChunkToBlackenIndex]] = true;
                _nextChunkToBlackenIndex++;
            }
        }
        
        if (currentTargetCoveragePercent >= 1.0f) {
            if (!_isFadeOutToBlackCompleted) { 
                // Ensure all remaining chunks are black if timer is up
                while(_nextChunkToBlackenIndex < totalChunks) {
                     _fadeOverlayMask[_shuffledChunkIndices[_nextChunkToBlackenIndex]] = true;
                    _nextChunkToBlackenIndex++;
                }
                _isFadeOutToBlackCompleted = true;
                debugPrint("EFFECTS_MANAGER", "EffectsManager: FadeOutToBlack Reached Full Coverage.");
            }
        }
    }
}