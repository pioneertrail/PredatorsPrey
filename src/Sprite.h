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

    // Stamina system for predators
    int maxStamina = 5;         // Maximum stamina for predator sprint
    int currentStamina = 5;     // Current stamina available 
    int staminaRechargeTime = 10; // Frames needed to recharge stamina
    int staminaRechargeCounter = 0; // Counter for stamina recharge
    int restingDuration = 0;        // How many frames predator has been resting
    int maxRestingDuration = 15; // Max frames predator will rest before wandering (made non-const to fix C2280)

    // Evasion system for prey
    float evasionChance = 0.35f;  // 35% chance to evade capture
    bool isStunned = false;      // Whether sprite is currently stunned
    int stunDuration = 0;        // How many more frames the sprite remains stunned

    // Fear system for prey
    float currentFear = 0.0f;
    float maxFear = 100.0f;
    float fearIncreaseRate = 10.0f;
    float fearDecreaseRate = 0.5f;  // Removed const qualifier

    // For Safe Zone seeking by Prey
    bool is_heading_to_safe_zone = false;
    // Vec2D targeted_safe_zone_center; // Might not be needed if path stores goal

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
        WANDERING,      // For both predator and prey
        SEEKING,        // Predator specific: actively hunting prey
        SEARCHING_LKP,  // Predator: moving to last known prey position
        FLEEING,        // Prey specific: actively evading predator
        STUNNED,        // New state for when predator is stunned after failed capture
        RESTING         // Predator specific: resting to regain stamina faster
    };
    AIState currentState = AIState::WANDERING; // Default state

    // Helper to get the character with its color codes
    std::string getDisplayString() const {
        return colorCode + displayChar + Color::RESET;
    }
};

#endif // SPRITE_H 