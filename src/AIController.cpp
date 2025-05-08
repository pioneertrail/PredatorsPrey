#include "AIController.h"
#include "Pathfinding.h" // Needs pathfinding functions
#include <random>       // For AI logic randomness
#include <algorithm>    // For std::shuffle, std::max, std::min
#include <cmath>        // For std::abs
#include <limits> // Required for std::numeric_limits

// Use the same random generator as main.cpp for now.
// This should ideally be passed in or managed differently.
extern std::mt19937 gen; 

namespace AIController {

// Helper function to find the closest sprite from a list
// Returns a pointer to the closest sprite, or nullptr if the list is empty or no sprite is within max_dist
const Sprite* find_closest_sprite(const Vec2D& current_pos, const std::vector<Sprite>& candidates, int& out_distance, int max_dist) {
    const Sprite* closest = nullptr;
    int min_dist_sq = std::numeric_limits<int>::max();
    out_distance = std::numeric_limits<int>::max();

    if (candidates.empty()) {
        return nullptr;
    }

    for (const auto& candidate : candidates) {
        // Make sure we don't target ourselves if by mistake a sprite is in its own list of targets
        // This check is more relevant if all_predators could include current_sprite, etc.
        // For now, predator targets prey and prey targets predator, so direct self-check isn't strictly needed
        // but it's good practice if lists could overlap.
        // if (&candidate == &current_sprite) continue; 

        int dist_sq = squared_distance(current_pos, candidate.position);
        if (dist_sq < min_dist_sq) {
            min_dist_sq = dist_sq;
            closest = &candidate;
        }
    }

    if (closest) {
        out_distance = manhattan_distance(current_pos, closest->position); // Use Manhattan for behavior triggers
        if (out_distance > max_dist) { // Check against max_dist for awareness/vision
            return nullptr;
        }
    }
    return closest;
}

// Implementation of move_randomly moved from main.cpp
void move_randomly(Sprite& s, const World& world) {
    // If sprite is stunned, handle stunned state and return
    if (s.isStunned) {
        s.stunDuration--;
        if (s.stunDuration <= 0) {
            s.isStunned = false;
            s.currentState = Sprite::AIState::WANDERING; // Or RESTING if stamina is low?
        }
        return; // Don't move if stunned
    }

    // Handle Predator Resting state - no movement, faster stamina recharge
    if (s.type == Sprite::Type::PREDATOR && s.currentState == Sprite::AIState::RESTING) {
        if (s.currentStamina < s.maxStamina) {
            // Faster recharge: e.g., 1 stamina point every 2-3 frames
            // Using a simple counter, 1 stamina per call for now for simplicity, adjust as needed.
            s.staminaRechargeCounter++;
            if (s.staminaRechargeCounter >= 2) { // Recharge 1 stamina every 2 steps
                s.currentStamina = std::min(s.currentStamina + 1, s.maxStamina);
                s.staminaRechargeCounter = 0;
            }
        }
        s.restingDuration++;
        // Transition out of resting handled in update_sprite_ai
        return; // No movement while resting
    }

    Vec2D potential_pos = s.position;
    bool chose_new_direction = false;

    // First check if we can continue in the same direction (biased random walk)
    if (s.stepsInCurrentDirection < MAX_STEPS_IN_DIRECTION && (s.lastMoveDirection.x != 0 || s.lastMoveDirection.y != 0)) {
        // Determine effective speed - factor in stamina for predators
        int effective_speed = s.speed;
        if (s.type == Sprite::Type::PREDATOR) {
            bool can_use_stamina = (s.currentStamina > 0 && 
                                    s.currentState == Sprite::AIState::SEEKING);
            
            if (can_use_stamina) {
                // Use sprint speed (2x)
                effective_speed = 2; 
                s.currentStamina--;
                s.staminaRechargeCounter = 0; // Reset recharge counter when stamina is used
            } else {
                // Normal speed (1x) when out of stamina or not seeking
                effective_speed = 1;
                
                // Handle stamina recharge (only if not resting, resting handles its own)
                if (s.currentStamina < s.maxStamina) {
                    s.staminaRechargeCounter++;
                    if (s.staminaRechargeCounter >= s.staminaRechargeTime) {
                        s.currentStamina = s.maxStamina; // Fully recharge
                        s.staminaRechargeCounter = 0;
                    }
                }
            }
        }
        
        // Use correct speed when moving in the current direction
        Vec2D continued_pos = {
            s.position.x + s.lastMoveDirection.x * effective_speed,
            s.position.y + s.lastMoveDirection.y * effective_speed
        };
        
        if (world.is_walkable(continued_pos)) {
            potential_pos = continued_pos;
            s.stepsInCurrentDirection++;
        } else {
            // If full speed move is blocked, try half-speed move for predators
            if (s.type == Sprite::Type::PREDATOR && effective_speed > 1) {
                Vec2D half_speed_pos = {
                    s.position.x + s.lastMoveDirection.x,
                    s.position.y + s.lastMoveDirection.y
                };
                
                if (world.is_walkable(half_speed_pos)) {
                    potential_pos = half_speed_pos;
                    s.stepsInCurrentDirection++;
                } else {
                    chose_new_direction = true;
                }
            } else {
                chose_new_direction = true;
            }
        }
    } else {
        chose_new_direction = true;
    }

    if (chose_new_direction) {
        std::vector<Vec2D> move_options_base;
        
        // Predators prefer diagonal moves for better coverage (8-directional)
        if (s.type == Sprite::Type::PREDATOR) {
            move_options_base = {
                {1,0}, {0,1}, {-1,0}, {0,-1},  // Cardinal directions
                {1,1}, {1,-1}, {-1,1}, {-1,-1}, // Diagonal directions
                {0,0} // Staying still (lowest priority)
            };
        } else {
            // Prey use simpler movement options
            move_options_base = {{0,1}, {0,-1}, {1,0}, {-1,0}, {0,0}}; // Cardinal + stay still
        }
        
        std::vector<Vec2D> valid_move_choices;
        
        // First pass: check moves using full speed
        int effective_speed = s.speed;
        if (s.type == Sprite::Type::PREDATOR) {
            bool can_use_stamina = (s.currentStamina > 0 && 
                                   s.currentState == Sprite::AIState::SEEKING);
            
            if (can_use_stamina) {
                effective_speed = 2;
                s.currentStamina--;
                s.staminaRechargeCounter = 0; // Reset recharge counter
            } else {
                effective_speed = 1;
                
                // Handle stamina recharge (only if not resting)
                if (s.currentStamina < s.maxStamina) {
                    s.staminaRechargeCounter++;
                    if (s.staminaRechargeCounter >= s.staminaRechargeTime) {
                        s.currentStamina = s.maxStamina; // Fully recharge
                        s.staminaRechargeCounter = 0;
                    }
                }
            }
        }
        
        for (const auto& offset : move_options_base) {
            // Skip staying still option for predators unless absolutely necessary
            if (s.type == Sprite::Type::PREDATOR && offset.x == 0 && offset.y == 0) {
                continue;
            }
            
            Vec2D test_pos = {
                s.position.x + offset.x * effective_speed, 
                s.position.y + offset.y * effective_speed
            };
            
            if (world.is_walkable(test_pos)) {
                if (s.type == Sprite::Type::PREDATOR) {
                    bool on_trail = false;
                    for (const auto& trail_pos : s.recentWanderTrail) {
                        if (trail_pos == test_pos) { 
                            on_trail = true; 
                            break; 
                        }
                    }
                    if (!on_trail) {
                        valid_move_choices.push_back(offset);
                    }
                } else { // Prey doesn't use a wander trail
                    valid_move_choices.push_back(offset);
                }
            }
        }
        
        // For predators: if no valid full-speed moves found, try half-speed moves
        if (s.type == Sprite::Type::PREDATOR && valid_move_choices.empty() && effective_speed > 1) {
            for (const auto& offset : move_options_base) {
                if (offset.x == 0 && offset.y == 0) continue; // Skip staying still
                
                Vec2D test_pos = {
                    s.position.x + offset.x,  // Use speed 1
                    s.position.y + offset.y   // Use speed 1
                };
                
                if (world.is_walkable(test_pos)) {
                    bool on_trail = false;
                    for (const auto& trail_pos : s.recentWanderTrail) {
                        if (trail_pos == test_pos) { 
                            on_trail = true; 
                            break; 
                        }
                    }
                    if (!on_trail) {
                        valid_move_choices.push_back(offset);
                    }
                }
            }
        }
        
        // If predator has no non-trail options, allow it to pick any walkable (including trail)
        if (s.type == Sprite::Type::PREDATOR && valid_move_choices.empty()) {
            for (const auto& offset : move_options_base) {
                Vec2D test_pos = {
                    s.position.x + offset.x * effective_speed, 
                    s.position.y + offset.y * effective_speed
                };
                
                if (world.is_walkable(test_pos)) { 
                    valid_move_choices.push_back(offset); 
                }
            }
            
            // If still no valid moves at full speed, try half speed
            if (valid_move_choices.empty() && effective_speed > 1) {
                for (const auto& offset : move_options_base) {
                    Vec2D test_pos = {
                        s.position.x + offset.x, 
                        s.position.y + offset.y
                    };
                    
                    if (world.is_walkable(test_pos)) { 
                        valid_move_choices.push_back(offset); 
                    }
                }
            }
        }
        
        // Avoid immediately reversing direction unless no other choice
        if (valid_move_choices.size() > 1 && (s.lastMoveDirection.x != 0 || s.lastMoveDirection.y != 0)) {
            Vec2D opposite_dir = {-s.lastMoveDirection.x, -s.lastMoveDirection.y};
            
            // Only remove if it's not the only option left (excluding {0,0} if present)
            auto it = std::remove_if(valid_move_choices.begin(), valid_move_choices.end(),
                               [&](const Vec2D& choice){ return choice == opposite_dir; });
                               
            if (it != valid_move_choices.begin() || 
                (it == valid_move_choices.begin() && *it != Vec2D{0,0} && valid_move_choices.end() - it > 1)) {
                valid_move_choices.erase(it, valid_move_choices.end());
            }
            
            if (valid_move_choices.empty()) { // if removing opposite made it empty, re-add options
                for (const auto& offset : move_options_base) {
                    Vec2D test_pos = {
                        s.position.x + offset.x * effective_speed, 
                        s.position.y + offset.y * effective_speed
                    };
                    
                    if (world.is_walkable(test_pos)) { 
                        valid_move_choices.push_back(offset); 
                    }
                }
            }
        }

        if (valid_move_choices.empty()) { 
            valid_move_choices.push_back({0,0}); // Default to staying still if truly stuck
        }
        
        // Select a random move from valid options
        Vec2D move_offset = valid_move_choices[std::uniform_int_distribution<>(0, (int)valid_move_choices.size() - 1)(gen)];
        
        // Use effective speed including stamina
        potential_pos = {
            s.position.x + move_offset.x * effective_speed, 
            s.position.y + move_offset.y * effective_speed
        };
        
        // If potential position is not walkable, fall back to reduced speed
        if (!world.is_walkable(potential_pos) && effective_speed > 1) {
            potential_pos = {
                s.position.x + move_offset.x,
                s.position.y + move_offset.y
            };
        }
        
        s.lastMoveDirection = move_offset;
        s.stepsInCurrentDirection = (move_offset.x == 0 && move_offset.y == 0) ? MAX_STEPS_IN_DIRECTION : 1; // Reset if still or new dir

        if (s.type == Sprite::Type::PREDATOR && (move_offset.x != 0 || move_offset.y != 0)) {
            // Add to wander trail only if it's a new position
            if (s.recentWanderTrail.empty() || s.recentWanderTrail.front() != potential_pos) {
                s.recentWanderTrail.insert(s.recentWanderTrail.begin(), potential_pos);
                if (s.recentWanderTrail.size() > WANDER_TRAIL_LENGTH) { 
                    s.recentWanderTrail.pop_back(); 
                }
            }
        }
    }
    
    // Final sanity check - clamp position to world bounds
    s.position = potential_pos;
    s.position.x = std::max(0, std::min(s.position.x, world.width - 1));
    s.position.y = std::max(0, std::min(s.position.y, world.height - 1));
}

// Implementation of handle_predator_path_following moved from main.cpp
Vec2D handle_predator_path_following(Sprite& predator, const World& world) {
    Vec2D target_pos = predator.position;
    if (!predator.currentPath.empty() && predator.pathFollowStep < predator.currentPath.size()) {
        Vec2D next_step = predator.currentPath[predator.pathFollowStep];
        if (world.is_walkable(next_step)) {
            target_pos = next_step;
            predator.pathFollowStep++;
            if (predator.pathFollowStep >= predator.currentPath.size()) {
                predator.currentPath.clear();
                predator.turnsSincePathReplan = REPLAN_PATH_INTERVAL;
            }
        } else { 
            // Path step is no longer walkable (dynamic obstacle appeared)
            predator.currentPath.clear();
            predator.turnsSincePathReplan = REPLAN_PATH_INTERVAL;
        }
    } else if (!predator.currentPath.empty() && predator.pathFollowStep >= predator.currentPath.size()){
         predator.currentPath.clear(); 
         predator.turnsSincePathReplan = REPLAN_PATH_INTERVAL;
    }
    return target_pos;
}

// New function to validate paths and fix invalid ones
bool validate_and_repair_path(std::vector<Vec2D>& path, /*const Vec2D& start, const Vec2D& goal,*/ 
                             const std::unordered_set<Vec2D>& obstacles, int width, int height) {
    if (path.empty()) {
        return false;
    }
    
    // Verify that each step in the path is walkable
    for (size_t i = 0; i < path.size(); ++i) {
        // Check path bounds
        if (path[i].x < 0 || path[i].x >= width || path[i].y < 0 || path[i].y >= height) {
            // Path contains out-of-bounds coordinates
            return false;
        }
        
        // Check if step is an obstacle
        if (obstacles.count(path[i]) > 0) {
            // Path contains an obstacle - discard this path
            return false;
        }
        
        // Check that consecutive steps are adjacent
        if (i > 0) {
            int dx = std::abs(path[i].x - path[i-1].x);
            int dy = std::abs(path[i].y - path[i-1].y);
            if (dx > 1 || dy > 1) {
                // Gap in path - discard this path
                return false;
            }
        }
    }
    
    // Path is valid
    return true;
}

void update_sprite_ai(Sprite& sprite_to_update, const std::vector<Sprite>& all_predators, const std::vector<Sprite>& all_prey, const World& world) {
    if (sprite_to_update.type == Sprite::Type::PREDATOR) {
        // --- Predator Logic ---
        int dist_to_closest_prey = 0;
        const Sprite* target_prey = find_closest_sprite(sprite_to_update.position, all_prey, dist_to_closest_prey, PREDATOR_VISION_RADIUS);
        
        bool prey_in_pred_sight = (target_prey != nullptr); // Target_prey is already filtered by PREDATOR_VISION_RADIUS
        Sprite::AIState previous_pred_state = sprite_to_update.currentState;

        // Enhanced stuck detection system - tracks position history and detects oscillation
        static const size_t MAX_TRACKED_PREDATORS = 10; // Increased from 3 to handle more predators
        static const int STUCK_THRESHOLD = 3; // Reduced to detect stuck situations faster
        static const int POSITION_HISTORY_SIZE = 5; // Track recent positions to detect oscillation
        
        static std::vector<Vec2D> position_history[MAX_TRACKED_PREDATORS]; // Track recent positions for each predator
        static int stuck_counters[MAX_TRACKED_PREDATORS] = {0}; // Count how many turns a predator hasn't made progress
        static bool initialized = false;
        
        // Initialize position history arrays if first run
        if (!initialized) {
            for (size_t i = 0; i < MAX_TRACKED_PREDATORS; ++i) {
                position_history[i].resize(POSITION_HISTORY_SIZE, {-1, -1});
            }
            initialized = true;
        }
        
        // Find which predator this is
        int predator_index = -1;
        for (size_t i = 0; i < all_predators.size() && i < MAX_TRACKED_PREDATORS; ++i) {
            if (&sprite_to_update == &all_predators[i]) {
                predator_index = static_cast<int>(i);
                break;
            }
        }
        
        // If we identified this predator's index, track its position and check for being stuck
        if (predator_index >= 0 && predator_index < MAX_TRACKED_PREDATORS) {
            // Shift history and add current position
            std::rotate(position_history[predator_index].begin(), 
                     position_history[predator_index].begin() + 1, 
                     position_history[predator_index].end());
            position_history[predator_index][0] = sprite_to_update.position;
            
            bool is_oscillating = false;
            bool is_stationary = false;
            
            // Check if predator hasn't moved at all
            is_stationary = (position_history[predator_index][0] == position_history[predator_index][1]);
            
            // Check if predator is oscillating between positions (moving but not making progress)
            if (!is_stationary && POSITION_HISTORY_SIZE >= 4) { // Need at least 4 positions to detect oscillation
                if ((position_history[predator_index][0] == position_history[predator_index][2]) && 
                    (position_history[predator_index][1] == position_history[predator_index][3])) {
                    is_oscillating = true;
                }
            }
            
            // Increment stuck counter if predator is stuck
            if (is_stationary || is_oscillating) {
                stuck_counters[predator_index]++;
            } else {
                // If making progress, reset counter
                stuck_counters[predator_index] = 0;
            }
            
            // If stuck for too many turns, try to unstick with increasingly aggressive measures
            if (stuck_counters[predator_index] > STUCK_THRESHOLD) {
                // Reset state and path
                sprite_to_update.currentState = Sprite::AIState::WANDERING;
                sprite_to_update.currentPath.clear();
                sprite_to_update.recentWanderTrail.clear();
                
                // Try to unstick with random moves of increasing distance
                bool unstuck = false;
                
                // First try adjacent moves
                Vec2D random_moves[] = {{1,0}, {-1,0}, {0,1}, {0,-1}, {1,1}, {-1,-1}, {1,-1}, {-1,1}};
                std::shuffle(std::begin(random_moves), std::end(random_moves), gen);
                
                for (const auto& move : random_moves) {
                    Vec2D new_pos = {sprite_to_update.position.x + move.x * sprite_to_update.speed, 
                                    sprite_to_update.position.y + move.y * sprite_to_update.speed};
                    if (world.is_walkable(new_pos)) {
                        sprite_to_update.position = new_pos;
                        position_history[predator_index][0] = new_pos; // Update history
                        unstuck = true;
                        break;
                    }
                }
                
                // If first attempt failed, try larger jumps (3 cells away)
                if (!unstuck && stuck_counters[predator_index] > STUCK_THRESHOLD + 2) {
                    Vec2D larger_moves[] = {{3,0}, {-3,0}, {0,3}, {0,-3}, {2,2}, {-2,-2}, {2,-2}, {-2,2}};
                    std::shuffle(std::begin(larger_moves), std::end(larger_moves), gen);
                    
                    for (const auto& move : larger_moves) {
                        Vec2D new_pos = {sprite_to_update.position.x + move.x, sprite_to_update.position.y + move.y};
                        if (world.is_walkable(new_pos)) {
                            sprite_to_update.position = new_pos;
                            position_history[predator_index][0] = new_pos; // Update history
                            unstuck = true;
                            break;
                        }
                    }
                }
                
                // Most extreme case: try to find any walkable position in a wider area
                if (!unstuck && stuck_counters[predator_index] > STUCK_THRESHOLD + 5) {
                    // Try looking in a 5x5 area around the current position
                    for (int dy = -5; dy <= 5 && !unstuck; ++dy) {
                        for (int dx = -5; dx <= 5 && !unstuck; ++dx) {
                            if (dx == 0 && dy == 0) continue; // Skip current position
                            
                            Vec2D new_pos = {sprite_to_update.position.x + dx, sprite_to_update.position.y + dy};
                            if (world.is_walkable(new_pos)) {
                                sprite_to_update.position = new_pos;
                                position_history[predator_index][0] = new_pos; // Update history
                                unstuck = true;
                            }
                        }
                    }
                }
                
                // If successfully unstuck, reset counter
                if (unstuck) {
                    stuck_counters[predator_index] = 0;
                    // Also clear the position history to avoid false oscillation detection
                    for (auto& pos : position_history[predator_index]) {
                        pos = sprite_to_update.position;
                    }
                }
            }
        }

        // State Transitions - More aggressive pursuit
        if (sprite_to_update.currentState == Sprite::AIState::WANDERING) {
            if (prey_in_pred_sight) { 
                sprite_to_update.currentState = Sprite::AIState::SEEKING; 
                sprite_to_update.lastKnownPreyPosition = target_prey->position; 
                sprite_to_update.currentPath.clear(); 
                sprite_to_update.turnsSincePathReplan = REPLAN_PATH_INTERVAL; // Force immediate plan
                sprite_to_update.restingDuration = 0; // Reset resting duration
            } else if (sprite_to_update.currentStamina < sprite_to_update.maxStamina / 2) { // Example: rest if below 50% stamina
                sprite_to_update.currentState = Sprite::AIState::RESTING;
                sprite_to_update.restingDuration = 0;
                sprite_to_update.currentPath.clear();
            }
        } else if (sprite_to_update.currentState == Sprite::AIState::SEEKING) {
            if (target_prey) {
                bool need_new_path = sprite_to_update.currentPath.empty() || sprite_to_update.turnsSincePathReplan >= REPLAN_PATH_INTERVAL;
                
                if (need_new_path) {
                    Vec2D path_goal = target_prey->position; // Default to actual
                    // Predict if prey is moving
                    if (target_prey->lastMoveDirection.x != 0 || target_prey->lastMoveDirection.y != 0) {
                        Vec2D predicted_target_pos = {
                            target_prey->position.x + target_prey->lastMoveDirection.x * target_prey->speed,
                            target_prey->position.y + target_prey->lastMoveDirection.y * target_prey->speed
                        };
                        if (predicted_target_pos.x >= 0 && predicted_target_pos.x < world.width && predicted_target_pos.y >= 0 && predicted_target_pos.y < world.height && world.is_walkable(predicted_target_pos)) {
                            path_goal = predicted_target_pos; // Use predicted if valid
                        }
                    }
                    sprite_to_update.currentPath = find_path(sprite_to_update.position, path_goal, world.obstacles, world.width, world.height);
                    sprite_to_update.pathFollowStep = 0;
                    sprite_to_update.turnsSincePathReplan = 0;
                }

                // Try to follow the path (newly generated or existing)
                if (!sprite_to_update.currentPath.empty()) {
                    int speed = sprite_to_update.speed;
                    if (sprite_to_update.currentStamina > 0) speed = 2; // Simplified sprint check for this block

                    Vec2D candidate_pos = sprite_to_update.position;
                    bool moved_along_path = false;

                    int steps_taken_on_path = 0;
                    Vec2D temp_current_pos = sprite_to_update.position;
                    // Corrected loop for SEEKING state, similar to SEARCHING_LKP
                    for(int i=0; i<speed && (sprite_to_update.pathFollowStep + i) < sprite_to_update.currentPath.size(); ++i ){
                        Vec2D check_step = sprite_to_update.currentPath[sprite_to_update.pathFollowStep+i];
                        if(manhattan_distance(temp_current_pos, check_step) == 1 && world.is_walkable(check_step)){
                            temp_current_pos = check_step; // temp_current_pos becomes the actual candidate_pos for this iteration
                            candidate_pos = temp_current_pos; // Update candidate_pos each valid step
                            steps_taken_on_path++;
                            moved_along_path = true; // Mark that at least one step was possible
                        } else {
                            // Path blocked or invalid, clear current path and force replan
                            sprite_to_update.currentPath.clear();
                            sprite_to_update.turnsSincePathReplan = REPLAN_PATH_INTERVAL; // Ensure replan on next relevant tick
                            moved_along_path = false; // No successful move along this path segment
                            break;
                        }
                    }

                    // This 'if(moved_along_path)' means the loop above found at least one valid step.
                    // 'candidate_pos' holds the furthest valid point reached in the loop.
                    // 'steps_taken_on_path' is the count of discrete steps.
                    if(moved_along_path){ // This was 'if(steps_taken_on_path > 0)' before, but moved_along_path is set in the loop.
                        sprite_to_update.position = candidate_pos; // Set position to furthest valid step
                        sprite_to_update.pathFollowStep += steps_taken_on_path; // Increment by actual steps taken

                        if (sprite_to_update.pathFollowStep >= sprite_to_update.currentPath.size()) {
                            sprite_to_update.currentPath.clear();
                            sprite_to_update.turnsSincePathReplan = REPLAN_PATH_INTERVAL;
                        }
                    } else if (need_new_path && sprite_to_update.currentPath.empty()){ // Path was needed, but find_path failed or first step invalid
                        move_randomly(sprite_to_update, world); sprite_to_update.position = sprite_to_update.position;
                    }
                    // If !moved_along_path but path exists, it implies first step is blocked or path is very short.
                    // handle_predator_path_following might be called or random move if stuck.
                    if (sprite_to_update.position == sprite_to_update.position && !sprite_to_update.currentPath.empty()){ // Still at same pos, path exists
                        Vec2D single_step_fallback = handle_predator_path_following(sprite_to_update, world); // Try single step
                        if(single_step_fallback != sprite_to_update.position) sprite_to_update.position = single_step_fallback;
                        else { move_randomly(sprite_to_update, world); sprite_to_update.position = sprite_to_update.position; }
                    }

                } else { // No path (either couldn't be generated or was invalidated and not regenerated yet)
                    move_randomly(sprite_to_update, world); // Fallback to random movement
                    sprite_to_update.position = sprite_to_update.position;
                }
            } else { // Target prey is null
                sprite_to_update.currentState = Sprite::AIState::SEARCHING_LKP;
            }
        } else if (sprite_to_update.currentState == Sprite::AIState::SEARCHING_LKP) {
            bool need_new_path = sprite_to_update.currentPath.empty() || sprite_to_update.turnsSincePathReplan >= REPLAN_PATH_INTERVAL;
            if (need_new_path) {
                sprite_to_update.currentPath = find_path(sprite_to_update.position, sprite_to_update.lastKnownPreyPosition, world.obstacles, world.width, world.height);
                sprite_to_update.pathFollowStep = 0;
                sprite_to_update.turnsSincePathReplan = 0;
            }

            if (!sprite_to_update.currentPath.empty()) {
                // Similar multi-step path following as in SEEKING, but without sprint stamina usage for LKP search by default
                int speed = sprite_to_update.speed; // Base speed for LKP search
                Vec2D candidate_pos = sprite_to_update.position;
                bool moved_along_path = false;

                int steps_taken_on_path = 0;
                Vec2D temp_current_pos = sprite_to_update.position;
                for(int i=0; i<speed && (sprite_to_update.pathFollowStep + i) < sprite_to_update.currentPath.size(); ++i ){
                    Vec2D check_step = sprite_to_update.currentPath[sprite_to_update.pathFollowStep+i];
                    if(manhattan_distance(temp_current_pos, check_step) == 1 && world.is_walkable(check_step)){
                         temp_current_pos = check_step;
                         steps_taken_on_path++;
                    } else { sprite_to_update.currentPath.clear(); sprite_to_update.turnsSincePathReplan = REPLAN_PATH_INTERVAL; break; }
                }
                if(steps_taken_on_path > 0){
                     sprite_to_update.position = sprite_to_update.currentPath[sprite_to_update.pathFollowStep + steps_taken_on_path -1];
                     sprite_to_update.pathFollowStep += steps_taken_on_path;
                     moved_along_path = true;
                }

                if (moved_along_path && sprite_to_update.pathFollowStep >= sprite_to_update.currentPath.size()) {
                    sprite_to_update.currentPath.clear();
                    sprite_to_update.turnsSincePathReplan = REPLAN_PATH_INTERVAL;
                }
                 if (!moved_along_path && sprite_to_update.currentPath.empty()){ // Path was needed, but find_path failed or first step invalid
                     move_randomly(sprite_to_update, world); sprite_to_update.position = sprite_to_update.position;
                 }
                 else if (!moved_along_path && !sprite_to_update.currentPath.empty()){ // Still at same pos, path exists
                    Vec2D single_step_fallback = handle_predator_path_following(sprite_to_update, world); // Try single step
                    if(single_step_fallback != sprite_to_update.position) sprite_to_update.position = single_step_fallback;
                    else { move_randomly(sprite_to_update, world); sprite_to_update.position = sprite_to_update.position; }
                }
            } else { // No path to LKP
                move_randomly(sprite_to_update, world);
                sprite_to_update.position = sprite_to_update.position;
            }
        } else if (sprite_to_update.currentState == Sprite::AIState::STUNNED) {
             // Stunned logic already handled in move_randomly
             // Transition out of STUNNED is also handled there.
             // If it transitions to WANDERING, and stamina is low, it might then go to RESTING in the next cycle.
        }
        
        if (sprite_to_update.currentState == Sprite::AIState::WANDERING && 
            (previous_pred_state != Sprite::AIState::WANDERING && previous_pred_state != Sprite::AIState::RESTING) ) { // Added RESTING check
            sprite_to_update.recentWanderTrail.clear(); // Clear trail when starting to wander
        }

        // Predator Action - Move using full speed
        Vec2D predator_next_pos = sprite_to_update.position; // Correctly declared here

        if (sprite_to_update.currentState == Sprite::AIState::RESTING || sprite_to_update.isStunned) {
            // No movement if resting or stunned, predator_next_pos remains sprite_to_update.position
        } 
        // Direct pursuit if very close to prey
        else if (prey_in_pred_sight && dist_to_closest_prey <= 3) {
            Vec2D pursuit_target = target_prey->position; // Default to actual
            // Predict if prey is moving
            if (target_prey->lastMoveDirection.x != 0 || target_prey->lastMoveDirection.y != 0) {
                Vec2D predicted_pos = {
                    target_prey->position.x + target_prey->lastMoveDirection.x * target_prey->speed,
                    target_prey->position.y + target_prey->lastMoveDirection.y * target_prey->speed
                };
                if (predicted_pos.x >= 0 && predicted_pos.x < world.width && predicted_pos.y >= 0 && predicted_pos.y < world.height && world.is_walkable(predicted_pos)) {
                    pursuit_target = predicted_pos; // Use predicted if valid
                }
            }
            Vec2D dir = { pursuit_target.x - sprite_to_update.position.x, pursuit_target.y - sprite_to_update.position.y };
            int dx = 0, dy = 0;
            if (dir.x > 0) dx = std::min(dir.x, sprite_to_update.speed); else if (dir.x < 0) dx = std::max(dir.x, -sprite_to_update.speed);
            if (dir.y > 0) dy = std::min(dir.y, sprite_to_update.speed); else if (dir.y < 0) dy = std::max(dir.y, -sprite_to_update.speed);
            Vec2D potential_pos = { sprite_to_update.position.x + dx, sprite_to_update.position.y + dy };

            if (world.is_walkable(potential_pos)) {
                predator_next_pos = potential_pos;
            } else if (dx != 0 && dy != 0) { // Diagonal failed, try cardinal
                Vec2D x_move = { sprite_to_update.position.x + dx, sprite_to_update.position.y };
                Vec2D y_move = { sprite_to_update.position.x, sprite_to_update.position.y + dy };
                bool x_walkable = world.is_walkable(x_move);
                bool y_walkable = world.is_walkable(y_move);
                if (x_walkable && y_walkable) {
                    predator_next_pos = (manhattan_distance(x_move, pursuit_target) < manhattan_distance(y_move, pursuit_target)) ? x_move : y_move;
                } else if (x_walkable) {
                    predator_next_pos = x_move;
                } else if (y_walkable) {
                    predator_next_pos = y_move;
                }
                 // If both fail, predator_next_pos remains current pos, will fall through to other states or random move if no path either.
            }
             // If all direct/cardinal attempts fail, predator_next_pos remains current pos, potentially triggering pathing or random move if no path either.
        } 
        // WANDERING state
        else if (sprite_to_update.currentState == Sprite::AIState::WANDERING) {
            move_randomly(sprite_to_update, world);
            predator_next_pos = sprite_to_update.position; // Position is updated by move_randomly
            sprite_to_update.currentPath.clear(); 
        }
        // Note: If any state above didn't set predator_next_pos, it defaults to sprite_to_update.position (no move or handled by move_randomly)

        // Apply the determined move for predator
        sprite_to_update.position = predator_next_pos;

    } else if (sprite_to_update.type == Sprite::Type::PREY) {
        // --- Prey Logic ---
        int dist_to_closest_predator = 0;
        const Sprite* closest_predator = find_closest_sprite(sprite_to_update.position, all_predators, dist_to_closest_predator);

        bool predator_in_awareness_radius = (closest_predator != nullptr && dist_to_closest_predator <= PREY_AWARENESS_RADIUS);
        bool predator_has_los_to_prey = false;
        if (closest_predator && predator_in_awareness_radius) {
            predator_has_los_to_prey = has_line_of_sight(sprite_to_update.position, closest_predator->position, world.obstacles, world.width, world.height);
        }

        // Fear update logic (moved up to be general for prey)
        if (predator_in_awareness_radius && predator_has_los_to_prey) {
            sprite_to_update.currentFear = std::min(sprite_to_update.currentFear + sprite_to_update.fearIncreaseRate, sprite_to_update.maxFear);
        } else {
            float decay_rate = sprite_to_update.fearDecreaseRate;
            if (world.is_in_safe_zone(sprite_to_update.position)) {
                decay_rate *= SAFE_ZONE_FEAR_DECAY_MULTIPLIER;
            }
            sprite_to_update.currentFear = std::max(sprite_to_update.currentFear - decay_rate, 0.0f);
        }

        // State Transitions
        if (sprite_to_update.currentState == Sprite::AIState::WANDERING) {
            if (predator_in_awareness_radius && predator_has_los_to_prey) {
                sprite_to_update.currentState = Sprite::AIState::FLEEING;
                sprite_to_update.is_heading_to_safe_zone = false; // Reset when starting to flee
                sprite_to_update.currentPath.clear();
            } 
        } else if (sprite_to_update.currentState == Sprite::AIState::FLEEING) {
            if (!predator_in_awareness_radius || !predator_has_los_to_prey) { 
                sprite_to_update.currentState = Sprite::AIState::WANDERING;
                sprite_to_update.is_heading_to_safe_zone = false;
                sprite_to_update.currentPath.clear();
            } else {
                 // Already fleeing, predator is still visible/close. Fear is updated above.
                 // Decision to seek safe zone will happen in action phase if not already doing so.
            }
            // If inside a safe zone and predator is not immediately threatening, consider calming down faster or switching to WANDERING.
            if (world.is_in_safe_zone(sprite_to_update.position) && sprite_to_update.is_heading_to_safe_zone) {
                 // If reached the target (or close enough) of the safe zone path
                if (!sprite_to_update.currentPath.empty() && sprite_to_update.currentPath.back() == sprite_to_update.position) {
                    sprite_to_update.is_heading_to_safe_zone = false;
                    sprite_to_update.currentPath.clear();
                    // Potentially switch to WANDERING if predator isn't too close inside the safe zone
                    if (dist_to_closest_predator > PREY_AWARENESS_RADIUS / 2) { 
                        sprite_to_update.currentState = Sprite::AIState::WANDERING; 
                    }
                }
            }
        }

        // Prey Action
        Vec2D prey_next_pos = sprite_to_update.position;
        if (sprite_to_update.currentState == Sprite::AIState::FLEEING && closest_predator) {
            // Decision: Try to find and go to a safe zone?
            if (!sprite_to_update.is_heading_to_safe_zone) {
                Vec2D best_safe_zone_target = {-1, -1};
                std::vector<Vec2D> path_to_safe_zone;
                int shortest_path_len_to_safe_zone = std::numeric_limits<int>::max();

                for (const auto& zone_center : world.get_safe_zone_centers()) {
                    if (manhattan_distance(sprite_to_update.position, zone_center) > MAX_DIST_TO_CONSIDER_SAFE_ZONE) {
                        continue; // Too far to consider
                    }
                    std::vector<Vec2D> current_path = find_path(sprite_to_update.position, zone_center, world.obstacles, world.width, world.height);
                    if (!current_path.empty() && current_path.size() < shortest_path_len_to_safe_zone) {
                        // Basic heuristic: Is the first step of this path not directly towards the predator?
                        if (current_path.size() > 1) {
                            Vec2D first_step_dir = {current_path[1].x - sprite_to_update.position.x, current_path[1].y - sprite_to_update.position.y};
                            Vec2D predator_dir = {closest_predator->position.x - sprite_to_update.position.x, closest_predator->position.y - sprite_to_update.position.y};
                            // Simple dot product style check: if directions are roughly opposite or orthogonal
                            if ((first_step_dir.x * predator_dir.x + first_step_dir.y * predator_dir.y) <= 0) { 
                                shortest_path_len_to_safe_zone = static_cast<int>(current_path.size());
                                path_to_safe_zone = current_path;
                                best_safe_zone_target = zone_center; // Storing center, path leads there
                            }
                        }
                    }
                }

                if (!path_to_safe_zone.empty()) {
                    sprite_to_update.currentPath = path_to_safe_zone;
                    sprite_to_update.is_heading_to_safe_zone = true;
                    sprite_to_update.pathFollowStep = 0; // Start following new path
                }
            }

            // Execute movement
            if (sprite_to_update.is_heading_to_safe_zone && !sprite_to_update.currentPath.empty()){
                // Follow path to safe zone
                Vec2D next_step = sprite_to_update.currentPath[sprite_to_update.pathFollowStep];
                if(world.is_walkable(next_step)){
                    prey_next_pos = next_step;
                    sprite_to_update.pathFollowStep++;
                    if(sprite_to_update.pathFollowStep >= sprite_to_update.currentPath.size()){
                        sprite_to_update.currentPath.clear();
                        sprite_to_update.is_heading_to_safe_zone = false; // Reached (or end of path)
                        // Behavior upon arrival is handled in state transition part
                    }
                } else { // Path to safe zone blocked
                    sprite_to_update.currentPath.clear();
                    sprite_to_update.is_heading_to_safe_zone = false; 
                    // Fall through to standard flee logic below
                }
            }
            
            // If not heading to a safe zone OR path to safe zone failed/ended, use standard flee logic
            if (!sprite_to_update.is_heading_to_safe_zone || prey_next_pos == sprite_to_update.position) { 
                // (Standard flee logic: LoS break / Maximize distance - from previous implementation)
                Vec2D best_evade_move_offset = {0,0};
                int max_dist_found_no_los_break = -1; 
                bool found_los_break_move = false;
                int best_dist_with_los_break = -1; 
                std::vector<Vec2D> evade_options = {{0,1}, {0,-1}, {1,0}, {-1,0}, {1,1}, {1,-1}, {-1,1}, {-1,-1}, {0,0}};
                std::shuffle(evade_options.begin(), evade_options.end(), gen);
                for(const auto& offset : evade_options) {
                    Vec2D p = {sprite_to_update.position.x + offset.x * sprite_to_update.speed,
                               sprite_to_update.position.y + offset.y * sprite_to_update.speed};
                    if (world.is_walkable(p)) {
                        int new_dist_to_pred = manhattan_distance(closest_predator->position, p);
                        bool breaks_los = !has_line_of_sight(p, closest_predator->position, world.obstacles, world.width, world.height);
                        if (breaks_los) {
                            if (!found_los_break_move || new_dist_to_pred > best_dist_with_los_break) {
                                best_evade_move_offset = offset;
                                best_dist_with_los_break = new_dist_to_pred;
                                found_los_break_move = true;
                            }
                        } else if (!found_los_break_move) {
                            if (new_dist_to_pred > max_dist_found_no_los_break) {
                                max_dist_found_no_los_break = new_dist_to_pred;
                                best_evade_move_offset = offset;
                            } else if (new_dist_to_pred == max_dist_found_no_los_break && 
                                       (best_evade_move_offset.x == 0 && best_evade_move_offset.y == 0) && 
                                       (offset.x != 0 || offset.y != 0)) {
                                best_evade_move_offset = offset;
                            }
                        }
                    }
                }
                prey_next_pos = {sprite_to_update.position.x + best_evade_move_offset.x * sprite_to_update.speed,
                                 sprite_to_update.position.y + best_evade_move_offset.y * sprite_to_update.speed};
                if (!world.is_walkable(prey_next_pos)) {
                    prey_next_pos = sprite_to_update.position;
                }
            }
        } else { // WANDERING (Prey) or no predator to flee from
            move_randomly(sprite_to_update, world); // Modifies sprite_to_update.position directly
            prey_next_pos = sprite_to_update.position; // Reflect the change
        }
        sprite_to_update.position = prey_next_pos; // Apply determined move
    }

    // Common: Clamp position to world bounds (redundant if move_randomly and pathing already do this, but safe)
    sprite_to_update.position.x = std::max(0, std::min(sprite_to_update.position.x, world.width - 1));
    sprite_to_update.position.y = std::max(0, std::min(sprite_to_update.position.y, world.height - 1));
}

} // namespace AIController 