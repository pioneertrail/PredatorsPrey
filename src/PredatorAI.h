#ifndef PREDATOR_AI_H
#define PREDATOR_AI_H

#include "Sprite.h"
#include "World.h"
#include <vector>

namespace PredatorAI {
    // Update a predator's AI state and position
    void update_predator(Sprite& predator, const std::vector<Sprite>& all_predators, 
                         const std::vector<Sprite>& all_prey, const World& world);
    
    // Handle predator's state transitions based on current situation
    void handle_state_transitions(Sprite& predator, const Sprite* target_prey, 
                                 bool prey_in_sight, Sprite::AIState previous_state);
    
    // Path generation for predator based on current state
    void generate_path(Sprite& predator, const Sprite* target_prey, const World& world);
    
    // Handle predator stuck detection and resolution
    bool detect_and_resolve_stuck(Sprite& predator, const std::vector<Sprite>& all_predators, const World& world);
    
    // Constants
    extern const int PREDATOR_VISION_RADIUS;
    extern const int REPLAN_PATH_INTERVAL;
    extern const int STUCK_THRESHOLD;
}

#endif // PREDATOR_AI_H 