#ifndef AICONTROLLER_H
#define AICONTROLLER_H

#include <vector>
#include <unordered_set>
#include <limits>
#include "Sprite.h"
#include "Vec2D.h"
#include "World.h"

// Contains AI logic update functions.
// Currently designed as free functions operating on Sprite and World data.
// Could be refactored into a class later if more state is needed.
namespace AIController {

    // Constants (moved from main.cpp)
    const int PREDATOR_VISION_RADIUS = 60;
    const int PREY_AWARENESS_RADIUS = 5;
    const int MAX_STEPS_IN_DIRECTION = 5;
    const int REPLAN_PATH_INTERVAL = 2;
    const int WANDER_TRAIL_LENGTH = 8;
    const int MAX_DIST_TO_CONSIDER_SAFE_ZONE = 25;     // Added for prey safe zone seeking
    const float SAFE_ZONE_FEAR_DECAY_MULTIPLIER = 2.0f; // Added for prey safe zone seeking

    // Updates the AI state and position for a single sprite, considering all other sprites.
    void update_sprite_ai(Sprite& sprite_to_update, const std::vector<Sprite>& all_predators, const std::vector<Sprite>& all_prey, const World& world);

    // Helper function for random movement (could be private if AIController was a class)
    void move_randomly(Sprite& s, const World& world);

    // Helper function for predator path following (could be private if AIController was a class)
    Vec2D handle_predator_path_following(Sprite& predator, const World& world);
    
    // Validates a path and returns true if it's valid, false otherwise
    bool validate_and_repair_path(std::vector<Vec2D>& path, 
                                 const std::unordered_set<Vec2D>& obstacles, int width, int height);

    // Helper function to find the closest sprite from a list
    const Sprite* find_closest_sprite(const Vec2D& current_pos, const std::vector<Sprite>& candidates,  
                                     int& out_distance, int max_dist = std::numeric_limits<int>::max());

} // namespace AIController

#endif // AICONTROLLER_H 