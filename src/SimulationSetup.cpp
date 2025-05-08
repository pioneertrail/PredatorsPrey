#include "SimulationSetup.h"
#include <algorithm>
#include <cstdlib>   // For getenv function
#include <limits>

namespace SimulationSetup {

int get_max_steps() {
    // Try to get MAX_STEPS from environment
    char* env_val_buffer = nullptr;
    size_t buffer_size = 0;
    errno_t err = _dupenv_s(&env_val_buffer, &buffer_size, "MAX_STEPS");

    if (err == 0 && env_val_buffer != nullptr) {
        try {
            int steps = std::stoi(env_val_buffer);
            free(env_val_buffer); // Free the buffer allocated by _dupenv_s
            return steps;
        } catch (const std::exception&) {
            // In case of conversion error, use default
            free(env_val_buffer); // Free the buffer in case of error too
            return 100000;
        }
    }
    // Default if not set or if _dupenv_s failed (env_val_buffer would be null)
    if (env_val_buffer) { // Should be null if err != 0, but good practice
        free(env_val_buffer);
    }
    return 100000;
}

std::vector<Sprite> initialize_predators() {
    std::vector<Sprite> predators;
    predators.reserve(NUM_PREDATORS);
    
    for (int i = 0; i < NUM_PREDATORS; ++i) {
        Sprite p;
        // Spread predators around more
        p.position = {10 + i * 20, 5 + i * 5}; // Better distribution
        p.displayChar = 'P';
        p.colorCode = Color::RED;
        p.currentState = Sprite::AIState::WANDERING;
        p.type = Sprite::Type::PREDATOR;
        p.speed = 2; // Base speed for predators
        p.turnsSincePathReplan = 0;
        p.pathFollowStep = 0;
        p.stepsInCurrentDirection = 0;
        p.lastMoveDirection = {0,0};
        
        // Initialize stamina properties
        p.maxStamina = 5;
        p.currentStamina = 5;
        p.staminaRechargeTime = 10;
        p.staminaRechargeCounter = 0;
        
        predators.push_back(p);
    }
    
    return predators;
}

std::vector<Sprite> initialize_prey(const World& world) {
    std::vector<Sprite> prey_sprites;
    prey_sprites.reserve(NUM_PREY);
    
    for (int i = 0; i < NUM_PREY; ++i) {
        Sprite p;
        // Distribute prey more widely
        p.position = {10 + (i % 3) * 15, 10 + (i / 3) * 5}; // Adjusted for better spread
        while (!world.is_walkable(p.position)) { // Ensure not starting in an obstacle
            p.position.x = (p.position.x + 1) % world.width;
            if (p.position.x == 0) p.position.y = (p.position.y + 1) % world.height;
        }
        p.displayChar = 'Y';
        p.colorCode = Color::YELLOW;
        p.currentState = Sprite::AIState::WANDERING;
        p.type = Sprite::Type::PREY;
        p.speed = 1; // Standard prey speed
        p.stepsInCurrentDirection = 0;
        p.lastMoveDirection = {0,0};
        
        // Initialize evasion properties
        p.evasionChance = 0.35f;  // 35% chance to evade
        
        prey_sprites.push_back(p);
    }
    
    return prey_sprites;
}

} // namespace SimulationSetup 