#pragma once

#include "GameObject.h" // Inherit from GameObject
#include "Renderer.h"   // Corrected to use a relative path
#include <Arduino.h>

class Pipe : public GameObject { // Inherit publicly
public:
    // Constructor still takes specific pipe properties
    Pipe(int initialX, int screenHeight, int initialGapY, int initialWidth, int initialGapHeight);

    // --- Override GameObject virtual methods ---
    virtual ~Pipe() override = default; // Good practice to override default destructor

    void update(int speed, unsigned long deltaTime) override;
    void draw(Renderer& renderer) override;
    bool isOffScreen() const override;
    bool collidesWith(float birdX, float birdY, float birdRadius) const override;
    int getX() const override;
    int getWidth() const override;
    GameObjectType getGameObjectType() const override { return GameObjectType::PIPE; } 


    // --- Scoring Overrides ---
    bool needsScoreCheck() const override;
    void markScored() override;
    bool hasBeenScored() const override;

    // --- Pipe Specific Methods (can be private/protected if only used internally) ---
     // 'reset' logic might change if we replace objects instead of resetting
    void reset(int startX, int screenHeight, int newGapY, int newWidth, int newGapHeight);

    // --- NEW: Public setters for new mechanics ---
    void setMoving(bool moving, float speed, int minY, int maxY);
    void setGapChanging(bool changing, float speed, int targetGap);
    void setFragile(bool fragile);
    void setHazards(bool top, bool bottom, int hazardH = 3);

    // --- NEW: Public getters for FlappyBirdScene ---
    int getGapY() const { return gapY; }
    int getGapHeight() const { return gapHeight; }
    bool isFragile() const { return _isFragile; }
    bool isBroken() const { return _isBroken; }
    void markAsBroken() { _isBroken = true; } 


private:
    // x is now inherited from GameObject
    int gapY;
    int width; 
    int gapHeight;
    bool scored; 
    int screenHeight; 

    // --- NEW: Members for new mechanics ---
    bool _isMovingVertically = false;
    float _verticalSpeed = 0.0f; // Now a fractional speed per frame
    float _accumulatedVerticalMove = 0.0f; // Accumulator for fractional movement
    int _minY = 0;
    int _maxY = 0;
    int _verticalMoveDirection = 1; 

    bool _isGapChanging = false;
    float _gapChangeSpeed = 0.0f; // Now a fractional speed per frame
    float _accumulatedGapChange = 0.0f; // Accumulator for fractional gap change
    int _targetGapHeight = 0;
    int _initialGapHeight = 0; 

    bool _isFragile = false; 
    bool _isBroken = false;  

    bool _hasTopHazard = false;
    bool _hasBottomHazard = false;
    int _hazardHeight = 3; 
};