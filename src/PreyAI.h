#ifndef PREY_AI_H
#define PREY_AI_H

#include "Sprite.h"
#include "World.h"
#include <vector>

namespace PreyAI {
    // Update a prey's AI state and position
    void update_prey(Sprite& prey, const std::vector<Sprite>& all_predators, const World& world);
    
    // Handle prey's state transitions based on current situation
    void handle_state_transitions(Sprite& prey, const Sprite* closest_predator, 
                                 bool predator_in_awareness_radius, bool predator_has_los);
    
    // Update prey's fear level based on predator proximity
    void update_fear(Sprite& prey, bool predator_in_awareness_radius, 
                    bool predator_has_los, const World& world);
    
    // Handle prey fleeing logic
    Vec2D calculate_flee_position(Sprite& prey, const Sprite* closest_predator, const World& world);
    
    // Find path to nearest safe zone
    bool find_path_to_safe_zone(Sprite& prey, const Sprite* closest_predator, const World& world);
    
    // Constants
    extern const int PREY_AWARENESS_RADIUS;
    extern const int MAX_DIST_TO_CONSIDER_SAFE_ZONE;
    extern const float SAFE_ZONE_FEAR_DECAY_MULTIPLIER;
}

#endif // PREY_AI_H 