#ifndef SPRITE_H
#define SPRITE_H

#include <string> // For color strings
#include <vector>     // For std::vector (used in currentPath)
#include "Vec2D.h" // Include the new Vec2D header

// ANSI Color Codes
namespace Color {
    const std::string RESET   = "\033[0m";
    const std::string BLACK   = "\033[30m";
    const std::string RED     = "\033[31m";
    const std::string GREEN   = "\033[32m";
    const std::string YELLOW  = "\033[33m";
    const std::string BLUE    = "\033[34m";
    const std::string MAGENTA = "\033[35m";
    const std::string CYAN    = "\033[36m";
    const std::string WHITE   = "\033[37m";
}

// Forward declare if we need a more complex GameWorld/Screen representation later

struct Sprite {
    Vec2D position;  // Current top-left position
    Vec2D size;      // Width and height
    char displayChar = '?'; // Character to represent the sprite in text output
    std::string colorCode = Color::WHITE; // Default color
    int speed = 1;       // Movement speed in units per update
    Vec2D lastKnownPreyPosition; // Added for predator to remember last sighting
                                 // Could be specific to a Predator class if we had one.
    Vec2D lastMoveDirection = {0,0};         // Added for biased random walk
    int stepsInCurrentDirection = 0;        // Added for biased random walk

    // For Predator Path Caching
    std::vector<Vec2D> currentPath;
    int pathFollowStep = 0;
    int turnsSincePathReplan = 0;

    // For Predator Patrolling/Smarter Wandering
    std::vector<Vec2D> recentWanderTrail; // Stores last few unique positions during wandering

    // Optional: Add type (predator/prey) or other properties
    enum class Type { PREDATOR, PREY };
    Type type;

    enum class AIState { 
        WANDERING, // For both predator and prey
        SEEKING,   // Predator specific: actively hunting prey
        SEARCHING_LKP,  // Predator: moving to last known prey position
        FLEEING    // Prey specific: actively evading predator
    };
    AIState currentState = AIState::WANDERING; // Default state

    // Helper to get the character with its color codes
    std::string getDisplayString() const {
        return colorCode + displayChar + Color::RESET;
    }
};

#endif // SPRITE_H 