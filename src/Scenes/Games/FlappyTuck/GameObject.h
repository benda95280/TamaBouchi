#pragma once

#include "Renderer.h"
#include <Arduino.h> // For basic types if needed

// Forward declaration
class FlappyTuckScene; // To allow passing scene context if absolutely needed (try to avoid)

// <<< NEW: GameObjectType Enum >>>
enum class GameObjectType {
    GENERIC,
    PIPE,
    BIRD_PLAYER, // Though player is not a GameObject in current Flappy design
    COLLECTIBLE, // For coins and power-ups
    ENEMY
};
// <<< END NEW >>>


class GameObject {
public:
    // Virtual destructor is crucial for polymorphism with pointers/references
    virtual ~GameObject() = default;

    // --- Core Virtual Methods (to be overridden by derived classes) ---

    // Update object state (e.g., position)
    // Pass speed from scene, deltaTime might be useful later
    virtual void update(int speed, unsigned long deltaTime) = 0;

    // Draw the object
    virtual void draw(Renderer& renderer) = 0;

    // Check if the object is off the left side of the screen
    virtual bool isOffScreen() const = 0;

    // Check collision with the bird (specific to Flappy Tuck for now)
    virtual bool collidesWith(float birdX, float birdY, float birdRadius) const = 0;

    // Getters for common properties needed by the scene (like positioning/scoring)
    virtual int getX() const = 0;
    virtual int getWidth() const = 0;

    // <<< NEW: Getter for GameObjectType >>>
    virtual GameObjectType getGameObjectType() const { return GameObjectType::GENERIC; }
    // <<< END NEW >>>


    // --- Scoring Related Methods ---

    // Does passing this object grant score?
    virtual bool needsScoreCheck() const { return false; } // Default: no score

    // Mark this object as having been scored
    virtual void markScored() {} // Default: do nothing

    // Check if this object has been scored
    virtual bool hasBeenScored() const { return true; } // Default: always considered scored (if not scoreable)

protected:
    // Allow derived classes access to common properties if needed
    int x = 0; // Keep for Pipe, but other objects might use float _x
    Renderer& _renderer; // <<< ADDED: Renderer reference for new GameObjects

    // Constructor for GameObject to initialize renderer
    GameObject(Renderer& rendererRef) : _renderer(rendererRef), x(0) {} // Initialize x here too
    // Default constructor for classes like Pipe that don't use _renderer directly yet
    // Make this one also initialize _renderer safely, perhaps pass from FlappyTuckScene.
    // For now, the existing Pipe constructor doesn't call a GameObject constructor.
    GameObject() : _renderer(*(Renderer*)nullptr), x(0) {} // Needs a valid ref or handle this differently
};
