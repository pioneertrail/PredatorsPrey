#include "PredatorAI.h"
#include "MovementController.h"
#include "Pathfinding.h"
#include <random>
#include <algorithm>

// Use the same random generator as main.cpp for consistency
extern std::mt19937 gen;

namespace PredatorAI {

// Constants
const int PREDATOR_VISION_RADIUS = 60;
const int REPLAN_PATH_INTERVAL = 2;
const int STUCK_THRESHOLD = 3;
const int POSITION_HISTORY_SIZE = 5;
const size_t MAX_TRACKED_PREDATORS = 10;

// Static data for stuck detection
static std::vector<Vec2D> position_history[MAX_TRACKED_PREDATORS];
static int stuck_counters[MAX_TRACKED_PREDATORS] = {0};
static bool initialized = false;

// Forward declaration of helper function
static const Sprite* find_closest_prey(const Sprite& predator, const std::vector<Sprite>& all_prey, int& dist_to_closest);

bool detect_and_resolve_stuck(Sprite& predator, const std::vector<Sprite>& all_predators, const World& world) {
    // Initialize the position history tracking if needed
    if (!initialized) {
        for (size_t i = 0; i < MAX_TRACKED_PREDATORS; ++i) {
            position_history[i].resize(POSITION_HISTORY_SIZE, {-1, -1});
        }
        initialized = true;
    }
    
    // Find this predator's index in the all_predators list
    int predator_index = -1;
    for (size_t i = 0; i < all_predators.size() && i < MAX_TRACKED_PREDATORS; ++i) {
        if (&predator == &all_predators[i]) {
            predator_index = static_cast<int>(i);
            break;
        }
    }
    
    if (predator_index < 0 || predator_index >= static_cast<int>(MAX_TRACKED_PREDATORS)) {
        return false; // Couldn't track this predator, assume not stuck
    }
    
    // Update position history
    std::rotate(position_history[predator_index].begin(), 
                position_history[predator_index].begin() + 1, 
                position_history[predator_index].end());
    position_history[predator_index][0] = predator.position;
    
    // Check for oscillation or stationary behavior
    bool is_oscillating = false;
    bool is_stationary = (position_history[predator_index][0] == position_history[predator_index][1]);
    
    if (!is_stationary && POSITION_HISTORY_SIZE >= 4) {
        if ((position_history[predator_index][0] == position_history[predator_index][2]) && 
            (position_history[predator_index][1] == position_history[predator_index][3])) {
            is_oscillating = true;
        }
    }
    
    // Update stuck counter
    if (is_stationary || is_oscillating) {
        stuck_counters[predator_index]++;
    } else {
        stuck_counters[predator_index] = 0;
    }
    
    // If predator is stuck, try to resolve it
    if (stuck_counters[predator_index] > STUCK_THRESHOLD) {
        predator.currentState = Sprite::AIState::WANDERING;
        predator.currentPath.clear();
        predator.recentWanderTrail.clear();
        
        bool unstuck = false;
        
        // Try small random moves first
        Vec2D random_moves[] = {{1,0}, {-1,0}, {0,1}, {0,-1}, {1,1}, {-1,-1}, {1,-1}, {-1,1}};
        std::shuffle(std::begin(random_moves), std::end(random_moves), gen);
        
        for (const auto& move : random_moves) {
            Vec2D new_pos = {
                predator.position.x + move.x * predator.speed,
                predator.position.y + move.y * predator.speed
            };
            
            if (world.is_walkable(new_pos)) {
                predator.position = new_pos;
                position_history[predator_index][0] = new_pos;
                unstuck = true;
                break;
            }
        }
        
        // If still stuck, try larger moves
        if (!unstuck && stuck_counters[predator_index] > STUCK_THRESHOLD + 2) {
            Vec2D larger_moves[] = {{3,0}, {-3,0}, {0,3}, {0,-3}, {2,2}, {-2,-2}, {2,-2}, {-2,2}};
            std::shuffle(std::begin(larger_moves), std::end(larger_moves), gen);
            
            for (const auto& move : larger_moves) {
                Vec2D new_pos = {
                    predator.position.x + move.x,
                    predator.position.y + move.y
                };
                
                if (world.is_walkable(new_pos)) {
                    predator.position = new_pos;
                    position_history[predator_index][0] = new_pos;
                    unstuck = true;
                    break;
                }
            }
        }
        
        // If still stuck, search a wider area
        if (!unstuck && stuck_counters[predator_index] > STUCK_THRESHOLD + 5) {
            for (int dy = -5; dy <= 5 && !unstuck; ++dy) {
                for (int dx = -5; dx <= 5 && !unstuck; ++dx) {
                    if (dx == 0 && dy == 0) continue;
                    
                    Vec2D new_pos = {
                        predator.position.x + dx,
                        predator.position.y + dy
                    };
                    
                    if (world.is_walkable(new_pos)) {
                        predator.position = new_pos;
                        position_history[predator_index][0] = new_pos;
                        unstuck = true;
                    }
                }
            }
        }
        
        // If we managed to unstick the predator, reset counters
        if (unstuck) {
            stuck_counters[predator_index] = 0;
            for (auto& pos : position_history[predator_index]) {
                pos = predator.position;
            }
            return true;
        }
    }
    
    return false;
}

void handle_state_transitions(Sprite& predator, const Sprite* target_prey, 
                             bool prey_in_sight, Sprite::AIState previous_state) {
    if (predator.currentState == Sprite::AIState::WANDERING) {
        if (prey_in_sight) { 
            predator.currentState = Sprite::AIState::SEEKING; 
            predator.lastKnownPreyPosition = target_prey->position; 
            predator.currentPath.clear(); 
            predator.turnsSincePathReplan = REPLAN_PATH_INTERVAL; 
            predator.restingDuration = 0; 
        } else if (predator.currentStamina < predator.maxStamina / 2) { 
            predator.currentState = Sprite::AIState::RESTING;
            predator.restingDuration = 0;
            predator.currentPath.clear();
        }
    } else if (predator.currentState == Sprite::AIState::SEEKING) {
        if (!target_prey) { // Lost sight
           predator.currentState = Sprite::AIState::SEARCHING_LKP;
           // lastKnownPreyPosition should already be set from when SEEKING started
        } else { // Still see prey
           predator.lastKnownPreyPosition = target_prey->position; // Update LKP
           predator.turnsSincePathReplan++;
           // Check if needs to rest
           if(predator.currentStamina <= 0) {
               predator.currentState = Sprite::AIState::RESTING;
               predator.restingDuration = 0;
               predator.currentPath.clear();
           }
        }
    } else if (predator.currentState == Sprite::AIState::SEARCHING_LKP) {
        if (prey_in_sight) { // Found prey again
            predator.currentState = Sprite::AIState::SEEKING;
            predator.lastKnownPreyPosition = target_prey->position;
            predator.currentPath.clear();
            predator.turnsSincePathReplan = REPLAN_PATH_INTERVAL;
        } else if (predator.position == predator.lastKnownPreyPosition || predator.currentPath.empty()) { 
            // Reached LKP or path ended/failed
            predator.currentState = Sprite::AIState::WANDERING;
        } else { // Continue following path to LKP
             predator.turnsSincePathReplan++;
        }
    } else if (predator.currentState == Sprite::AIState::RESTING) {
        if (prey_in_sight) { // Prey appeared while resting!
            predator.currentState = Sprite::AIState::SEEKING;
            predator.lastKnownPreyPosition = target_prey->position;
            predator.currentPath.clear(); 
            predator.turnsSincePathReplan = REPLAN_PATH_INTERVAL;
            predator.restingDuration = 0; 
        } else if (predator.currentStamina >= predator.maxStamina || 
                  predator.restingDuration > predator.maxRestingDuration) { 
            // Fully rested or rested too long
            predator.currentState = Sprite::AIState::WANDERING;
            predator.restingDuration = 0;
        } else {
             // Continue resting (logic handled in move_randomly)
             predator.restingDuration++;
        }
    }
    
    // Clear wander trail if transitioning out of wandering
    if (predator.currentState != Sprite::AIState::WANDERING && previous_state == Sprite::AIState::WANDERING) {
        predator.recentWanderTrail.clear(); 
    }
}

void generate_path(Sprite& predator, const Sprite* target_prey, const World& world) {
    if (predator.currentState == Sprite::AIState::SEEKING && target_prey) {
        bool need_new_path = predator.currentPath.empty() || 
                            predator.turnsSincePathReplan >= REPLAN_PATH_INTERVAL;
                            
        if (need_new_path) {
            Vec2D path_goal = target_prey->position;
            
            // Try to predict prey movement
            if (target_prey->lastMoveDirection.x != 0 || target_prey->lastMoveDirection.y != 0) {
                Vec2D predicted_target_pos = {
                    target_prey->position.x + target_prey->lastMoveDirection.x * target_prey->speed,
                    target_prey->position.y + target_prey->lastMoveDirection.y * target_prey->speed
                };
                
                if (predicted_target_pos.x >= 0 && predicted_target_pos.x < world.width && 
                    predicted_target_pos.y >= 0 && predicted_target_pos.y < world.height && 
                    world.is_walkable(predicted_target_pos)) {
                    path_goal = predicted_target_pos; 
                }
            }
            
            predator.currentPath = find_path(predator.position, path_goal, world.obstacles, world.width, world.height);
            predator.pathFollowStep = 0;
            predator.turnsSincePathReplan = 0;
        }
    } else if (predator.currentState == Sprite::AIState::SEARCHING_LKP) {
        bool need_new_path = predator.currentPath.empty() || 
                            predator.turnsSincePathReplan >= REPLAN_PATH_INTERVAL;
                            
        if (need_new_path) {
            predator.currentPath = find_path(predator.position, predator.lastKnownPreyPosition, 
                                            world.obstacles, world.width, world.height);
            predator.pathFollowStep = 0;
            predator.turnsSincePathReplan = 0;
        }
    }
}

static const Sprite* find_closest_prey(const Sprite& predator, const std::vector<Sprite>& all_prey, int& dist_to_closest) {
    const Sprite* closest = nullptr;
    int min_dist_sq = std::numeric_limits<int>::max();
    dist_to_closest = std::numeric_limits<int>::max();

    if (all_prey.empty()) {
        return nullptr;
    }

    for (const auto& prey : all_prey) {
        int dist_sq = squared_distance(predator.position, prey.position);
        if (dist_sq < min_dist_sq) {
            min_dist_sq = dist_sq;
            closest = &prey;
        }
    }

    if (closest) {
        dist_to_closest = manhattan_distance(predator.position, closest->position);
        if (dist_to_closest > PREDATOR_VISION_RADIUS) {
            return nullptr;
        }
    }
    
    return closest;
}

void update_predator(Sprite& predator, const std::vector<Sprite>& all_predators, 
                    const std::vector<Sprite>& all_prey, const World& world) {
    // Skip AI update if stunned (handled in move_randomly)
    if (predator.isStunned) {
        MovementController::move_randomly(predator, world);
        return;
    }
    
    // 1. Find closest prey and check if it's in vision range
    int dist_to_closest_prey = 0;
    const Sprite* target_prey = find_closest_prey(predator, all_prey, dist_to_closest_prey);
    bool prey_in_sight = (target_prey != nullptr);
    
    // 2. Store previous state for transition logic
    Sprite::AIState previous_state = predator.currentState;
    
    // 3. Check if predator is stuck and try to resolve it if so
    bool was_stuck = detect_and_resolve_stuck(predator, all_predators, world);
    if (was_stuck) {
        return; // If predator was stuck and we had to intervene, skip rest of update
    }
    
    // 4. Update predator state
    handle_state_transitions(predator, target_prey, prey_in_sight, previous_state);
    
    // 5. Generate or update path based on current state
    generate_path(predator, target_prey, world);
    
    // 6. Move the predator according to its current state
    if (predator.currentState == Sprite::AIState::RESTING) {
        // No movement in resting state, handled by move_randomly
        MovementController::move_randomly(predator, world);
    } else if (predator.currentState == Sprite::AIState::SEEKING || 
              predator.currentState == Sprite::AIState::SEARCHING_LKP) {
        if (!predator.currentPath.empty()) {
            // Try to follow path using higher speed if seeking and has stamina
            int speed = (predator.currentState == Sprite::AIState::SEEKING && predator.currentStamina > 0) ? 2 : 1;
            Vec2D candidate_pos = predator.position;
            bool moved_along_path = false;
            int steps_taken_on_path = 0;
            Vec2D temp_current_pos = predator.position;
            
            // Try to move along the path using the appropriate speed
            for (int i = 0; i < speed && (predator.pathFollowStep + i) < predator.currentPath.size(); ++i) {
                Vec2D check_step = predator.currentPath[predator.pathFollowStep + i];
                if (manhattan_distance(temp_current_pos, check_step) == 1 && world.is_walkable(check_step)) {
                    temp_current_pos = check_step;
                    candidate_pos = temp_current_pos;
                    steps_taken_on_path++;
                    moved_along_path = true;
                } else {
                    predator.currentPath.clear();
                    predator.turnsSincePathReplan = REPLAN_PATH_INTERVAL;
                    moved_along_path = false;
                    break;
                }
            }
            
            if (moved_along_path) {
                predator.position = candidate_pos;
                predator.pathFollowStep += steps_taken_on_path;
                
                // Consume stamina if sprinting
                if (predator.currentState == Sprite::AIState::SEEKING && speed > 1) {
                    predator.currentStamina--;
                }
                
                // Check if we've reached the end of the path
                if (predator.pathFollowStep >= predator.currentPath.size()) {
                    predator.currentPath.clear();
                    predator.turnsSincePathReplan = REPLAN_PATH_INTERVAL;
                }
            } else {
                // Path exists but couldn't move along it (obstacle) - try random move
                MovementController::move_randomly(predator, world);
            }
        } else {
            // No path - use random movement
            MovementController::move_randomly(predator, world);
        }
    } else if (predator.currentState == Sprite::AIState::WANDERING) {
        // Use random movement with trail tracking
        MovementController::move_randomly(predator, world);
    }
}

} // namespace PredatorAI 