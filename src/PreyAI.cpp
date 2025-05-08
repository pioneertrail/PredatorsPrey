#include "PreyAI.h"
#include "MovementController.h"
#include "Pathfinding.h"
#include <random>
#include <algorithm>
#include <limits>

// Use the same random generator as main.cpp for consistency
extern std::mt19937 gen;

namespace PreyAI {

// Constants
const int PREY_AWARENESS_RADIUS = 5;
const int MAX_DIST_TO_CONSIDER_SAFE_ZONE = 25;
const float SAFE_ZONE_FEAR_DECAY_MULTIPLIER = 2.0f;

// Forward declaration of helper function
static const Sprite* find_closest_predator(const Sprite& prey, const std::vector<Sprite>& all_predators, int& distance);

static const Sprite* find_closest_predator(const Sprite& prey, const std::vector<Sprite>& all_predators, int& distance) {
    const Sprite* closest = nullptr;
    int min_dist_sq = std::numeric_limits<int>::max();
    distance = std::numeric_limits<int>::max();

    if (all_predators.empty()) {
        return nullptr;
    }

    for (const auto& predator : all_predators) {
        int dist_sq = squared_distance(prey.position, predator.position);
        if (dist_sq < min_dist_sq) {
            min_dist_sq = dist_sq;
            closest = &predator;
        }
    }

    if (closest) {
        distance = manhattan_distance(prey.position, closest->position);
    }
    
    return closest;
}

void update_fear(Sprite& prey, bool predator_in_awareness_radius, 
                bool predator_has_los, const World& world) {
    if (predator_in_awareness_radius && predator_has_los) {
        // Increase fear when predator is visible
        prey.currentFear = std::min(prey.currentFear + prey.fearIncreaseRate, prey.maxFear);
    } else {
        // Decrease fear when no predator is visible
        float decay_rate = prey.fearDecreaseRate;
        
        // Fear decays faster in safe zones
        if (world.is_in_safe_zone(prey.position)) {
            decay_rate *= SAFE_ZONE_FEAR_DECAY_MULTIPLIER;
        }
        
        prey.currentFear = std::max(prey.currentFear - decay_rate, 0.0f);
    }
}

void handle_state_transitions(Sprite& prey, const Sprite* closest_predator,
                             bool predator_in_awareness_radius, bool predator_has_los) {
    if (prey.currentState == Sprite::AIState::WANDERING) {
        if (predator_in_awareness_radius && predator_has_los) {
            prey.currentState = Sprite::AIState::FLEEING;
            prey.is_heading_to_safe_zone = false;
            prey.currentPath.clear();
        }
    } else if (prey.currentState == Sprite::AIState::FLEEING) {
        if (!predator_in_awareness_radius || !predator_has_los) {
            prey.currentState = Sprite::AIState::WANDERING;
            prey.is_heading_to_safe_zone = false;
            prey.currentPath.clear();
        }
    }
}

bool find_path_to_safe_zone(Sprite& prey, const Sprite* closest_predator, const World& world) {
    if (!closest_predator) return false;
    
    Vec2D best_safe_zone_target = {-1, -1};
    std::vector<Vec2D> path_to_safe_zone;
    int shortest_path_len_to_safe_zone = std::numeric_limits<int>::max();
    
    // Check each safe zone center
    for (const auto& zone_center : world.get_safe_zone_centers()) {
        // Skip zones that are too far away
        if (manhattan_distance(prey.position, zone_center) > MAX_DIST_TO_CONSIDER_SAFE_ZONE) {
            continue;
        }
        
        // Find path to this safe zone
        std::vector<Vec2D> current_path = find_path(prey.position, zone_center, 
                                                   world.obstacles, world.width, world.height);
        
        // Check if this is a valid path and shorter than current best
        if (!current_path.empty() && static_cast<int>(current_path.size()) < shortest_path_len_to_safe_zone) {
            // Make sure the first step isn't towards the predator
            if (current_path.size() > 1) {
                Vec2D first_step_dir = {
                    current_path[1].x - prey.position.x, 
                    current_path[1].y - prey.position.y
                };
                Vec2D predator_dir = {
                    closest_predator->position.x - prey.position.x, 
                    closest_predator->position.y - prey.position.y
                };
                
                // Check if step direction is not towards predator
                // (dot product <= 0 means angle between vectors is >= 90 degrees)
                if ((first_step_dir.x * predator_dir.x + first_step_dir.y * predator_dir.y) <= 0) {
                    shortest_path_len_to_safe_zone = static_cast<int>(current_path.size());
                    path_to_safe_zone = current_path;
                    best_safe_zone_target = zone_center;
                }
            }
        }
    }
    
    // If we found a valid path to a safe zone, set it
    if (!path_to_safe_zone.empty()) {
        prey.currentPath = path_to_safe_zone;
        prey.is_heading_to_safe_zone = true;
        prey.pathFollowStep = 0;
        return true;
    }
    
    return false;
}

Vec2D calculate_flee_position(Sprite& prey, const Sprite* closest_predator, const World& world) {
    if (!closest_predator) return prey.position;
    
    Vec2D best_evade_move_offset = {0, 0};
    int max_dist_found_no_los_break = -1;
    bool found_los_break_move = false;
    int best_dist_with_los_break = -1;
    
    // Define possible movement options
    std::vector<Vec2D> evade_options = {
        {0,1}, {0,-1}, {1,0}, {-1,0}, {1,1}, {1,-1}, {-1,1}, {-1,-1}, {0,0}
    };
    
    // Randomize options for less predictable movement
    std::shuffle(evade_options.begin(), evade_options.end(), gen);
    
    // Evaluate each possible move
    for (const auto& offset : evade_options) {
        Vec2D potential_pos = {
            prey.position.x + offset.x * prey.speed,
            prey.position.y + offset.y * prey.speed
        };
        
        if (world.is_walkable(potential_pos)) {
            int new_dist_to_pred = manhattan_distance(closest_predator->position, potential_pos);
            bool breaks_los = !has_line_of_sight(potential_pos, closest_predator->position, 
                                                world.obstacles, world.width, world.height);
            
            // Prioritize moves that break line of sight
            if (breaks_los) {
                if (!found_los_break_move || new_dist_to_pred > best_dist_with_los_break) {
                    best_evade_move_offset = offset;
                    best_dist_with_los_break = new_dist_to_pred;
                    found_los_break_move = true;
                }
            } 
            // If no LOS-breaking move found yet, prioritize moves that increase distance
            else if (!found_los_break_move) {
                if (new_dist_to_pred > max_dist_found_no_los_break) {
                    max_dist_found_no_los_break = new_dist_to_pred;
                    best_evade_move_offset = offset;
                } 
                // Prefer moving over staying still if distance is the same
                else if (new_dist_to_pred == max_dist_found_no_los_break && 
                         (best_evade_move_offset.x == 0 && best_evade_move_offset.y == 0) && 
                         (offset.x != 0 || offset.y != 0)) {
                    best_evade_move_offset = offset;
                }
            }
        }
    }
    
    Vec2D next_pos = {
        prey.position.x + best_evade_move_offset.x * prey.speed,
        prey.position.y + best_evade_move_offset.y * prey.speed
    };
    
    // Sanity check - if position isn't walkable, stay put
    if (!world.is_walkable(next_pos)) {
        next_pos = prey.position;
    }
    
    return next_pos;
}

void update_prey(Sprite& prey, const std::vector<Sprite>& all_predators, const World& world) {
    // 1. Find closest predator
    int dist_to_closest_predator = 0;
    const Sprite* closest_predator = find_closest_predator(prey, all_predators, dist_to_closest_predator);
    
    // 2. Check if predator is in awareness radius and has line of sight
    bool predator_in_awareness_radius = (closest_predator != nullptr && 
                                       dist_to_closest_predator <= PREY_AWARENESS_RADIUS);
    bool predator_has_los_to_prey = false;
    
    if (closest_predator && predator_in_awareness_radius) {
        predator_has_los_to_prey = has_line_of_sight(prey.position, closest_predator->position, 
                                                   world.obstacles, world.width, world.height);
    }
    
    // 3. Update fear level
    update_fear(prey, predator_in_awareness_radius, predator_has_los_to_prey, world);
    
    // 4. Update state based on predator presence
    handle_state_transitions(prey, closest_predator, predator_in_awareness_radius, predator_has_los_to_prey);
    
    // 5. Execute movement based on current state
    Vec2D next_pos = prey.position;
    
    if (prey.currentState == Sprite::AIState::FLEEING && closest_predator) {
        // Check if we're already heading to a safe zone
        if (!prey.is_heading_to_safe_zone) {
            // Try to find path to a safe zone
            find_path_to_safe_zone(prey, closest_predator, world);
        }
        
        // If we have a path to safe zone, follow it
        if (prey.is_heading_to_safe_zone && !prey.currentPath.empty()) {
            Vec2D next_step = prey.currentPath[prey.pathFollowStep];
            
            if (world.is_walkable(next_step)) {
                next_pos = next_step;
                prey.pathFollowStep++;
                
                // Check if we've reached the end of the path
                if (prey.pathFollowStep >= prey.currentPath.size()) {
                    prey.currentPath.clear();
                    prey.is_heading_to_safe_zone = false;
                    
                    // If we're in a safe zone and predator is far enough, start wandering
                    if (world.is_in_safe_zone(prey.position) && 
                        dist_to_closest_predator > PREY_AWARENESS_RADIUS / 2) {
                        prey.currentState = Sprite::AIState::WANDERING;
                    }
                }
            } else {
                // Path is blocked - clear and calculate flee position
                prey.currentPath.clear();
                prey.is_heading_to_safe_zone = false;
                next_pos = calculate_flee_position(prey, closest_predator, world);
            }
        } 
        // If no path to safe zone or not heading to one, calculate flee position
        else if (!prey.is_heading_to_safe_zone || next_pos == prey.position) {
            next_pos = calculate_flee_position(prey, closest_predator, world);
        }
    } else {
        // Use random movement for wandering
        MovementController::move_randomly(prey, world);
        next_pos = prey.position; // position is updated directly in move_randomly
    }
    
    prey.position = next_pos;
}

} // namespace PreyAI 