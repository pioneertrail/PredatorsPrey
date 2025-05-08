#include "MovementController.h"
#include <algorithm>
#include <random>
#include <cmath>

// Use the same random generator as main.cpp
extern std::mt19937 gen;

// Constants moved from AIController.h
const int MAX_STEPS_IN_DIRECTION = 5;
const int WANDER_TRAIL_LENGTH = 8;

namespace MovementController {

int calculate_effective_speed(Sprite& sprite) {
    int effective_speed = sprite.speed;
    
    if (sprite.type == Sprite::Type::PREDATOR) {
        bool can_use_stamina = (sprite.currentStamina > 0 && 
                               sprite.currentState == Sprite::AIState::SEEKING);
        
        if (can_use_stamina) {
            effective_speed = 2;
            sprite.currentStamina--;
            sprite.staminaRechargeCounter = 0; // Reset recharge counter
        } else {
            effective_speed = 1;
            
            // Handle stamina recharge (only if not resting)
            if (sprite.currentStamina < sprite.maxStamina) {
                sprite.staminaRechargeCounter++;
                if (sprite.staminaRechargeCounter >= sprite.staminaRechargeTime) {
                    sprite.currentStamina = sprite.maxStamina; // Fully recharge
                    sprite.staminaRechargeCounter = 0;
                }
            }
        }
    }
    
    return effective_speed;
}

std::vector<Vec2D> get_valid_moves(const Sprite& sprite, const World& world, int effective_speed) {
    std::vector<Vec2D> move_options_base;
    std::vector<Vec2D> valid_move_choices;
    
    // Predators prefer diagonal moves for better coverage (8-directional)
    if (sprite.type == Sprite::Type::PREDATOR) {
        move_options_base = {
            {1,0}, {0,1}, {-1,0}, {0,-1},  // Cardinal directions
            {1,1}, {1,-1}, {-1,1}, {-1,-1}, // Diagonal directions
            {0,0} // Staying still (lowest priority)
        };
    } else {
        // Prey use simpler movement options
        move_options_base = {{0,1}, {0,-1}, {1,0}, {-1,0}, {0,0}}; // Cardinal + stay still
    }
    
    for (const auto& offset : move_options_base) {
        // Skip staying still option for predators unless absolutely necessary
        if (sprite.type == Sprite::Type::PREDATOR && offset.x == 0 && offset.y == 0) {
            continue;
        }
        
        Vec2D test_pos = {
            sprite.position.x + offset.x * effective_speed, 
            sprite.position.y + offset.y * effective_speed
        };
        
        if (world.is_walkable(test_pos)) {
            if (sprite.type == Sprite::Type::PREDATOR) {
                bool on_trail = false;
                for (const auto& trail_pos : sprite.recentWanderTrail) {
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
    
    return valid_move_choices;
}

Vec2D follow_path(Sprite& sprite, const World& world) {
    Vec2D target_pos = sprite.position;
    
    if (!sprite.currentPath.empty() && sprite.pathFollowStep < sprite.currentPath.size()) {
        Vec2D next_step = sprite.currentPath[sprite.pathFollowStep];
        if (world.is_walkable(next_step)) {
            target_pos = next_step;
            sprite.pathFollowStep++;
            if (sprite.pathFollowStep >= sprite.currentPath.size()) {
                sprite.currentPath.clear();
                sprite.turnsSincePathReplan = 0; // Reset replan counter
            }
        } else { 
            // Path step is no longer walkable (dynamic obstacle appeared)
            sprite.currentPath.clear();
            sprite.turnsSincePathReplan = 0;
        }
    } else if (!sprite.currentPath.empty() && sprite.pathFollowStep >= sprite.currentPath.size()){
         sprite.currentPath.clear(); 
         sprite.turnsSincePathReplan = 0;
    }
    
    return target_pos;
}

void move_randomly(Sprite& sprite, const World& world) {
    // If sprite is stunned, handle stunned state and return
    if (sprite.isStunned) {
        sprite.stunDuration--;
        if (sprite.stunDuration <= 0) {
            sprite.isStunned = false;
            sprite.currentState = Sprite::AIState::WANDERING;
        }
        return; // Don't move if stunned
    }

    // Handle Predator Resting state - no movement, faster stamina recharge
    if (sprite.type == Sprite::Type::PREDATOR && sprite.currentState == Sprite::AIState::RESTING) {
        if (sprite.currentStamina < sprite.maxStamina) {
            // Faster recharge: e.g., 1 stamina point every 2-3 frames
            sprite.staminaRechargeCounter++;
            if (sprite.staminaRechargeCounter >= 2) { // Recharge 1 stamina every 2 steps
                sprite.currentStamina = std::min(sprite.currentStamina + 1, sprite.maxStamina);
                sprite.staminaRechargeCounter = 0;
            }
        }
        sprite.restingDuration++;
        return; // No movement while resting
    }

    Vec2D potential_pos = sprite.position;
    bool chose_new_direction = false;

    // First check if we can continue in the same direction (biased random walk)
    if (sprite.stepsInCurrentDirection < MAX_STEPS_IN_DIRECTION && (sprite.lastMoveDirection.x != 0 || sprite.lastMoveDirection.y != 0)) {
        // Determine effective speed
        int effective_speed = calculate_effective_speed(sprite);
        
        // Use correct speed when moving in the current direction
        Vec2D continued_pos = {
            sprite.position.x + sprite.lastMoveDirection.x * effective_speed,
            sprite.position.y + sprite.lastMoveDirection.y * effective_speed
        };
        
        if (world.is_walkable(continued_pos)) {
            potential_pos = continued_pos;
            sprite.stepsInCurrentDirection++;
        } else {
            // If full speed move is blocked, try half-speed move for predators
            if (sprite.type == Sprite::Type::PREDATOR && effective_speed > 1) {
                Vec2D half_speed_pos = {
                    sprite.position.x + sprite.lastMoveDirection.x,
                    sprite.position.y + sprite.lastMoveDirection.y
                };
                
                if (world.is_walkable(half_speed_pos)) {
                    potential_pos = half_speed_pos;
                    sprite.stepsInCurrentDirection++;
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
        int effective_speed = calculate_effective_speed(sprite);
        std::vector<Vec2D> valid_move_choices = get_valid_moves(sprite, world, effective_speed);
        
        // For predators: if no valid full-speed moves found, try half-speed moves
        if (sprite.type == Sprite::Type::PREDATOR && valid_move_choices.empty() && effective_speed > 1) {
            for (const auto& offset : get_valid_moves(sprite, world, 1)) {
                valid_move_choices.push_back(offset);
            }
        }
        
        // If predator has no non-trail options, allow it to pick any walkable (including trail)
        if (sprite.type == Sprite::Type::PREDATOR && valid_move_choices.empty()) {
            std::vector<Vec2D> move_options_base;
            
            if (sprite.type == Sprite::Type::PREDATOR) {
                move_options_base = {
                    {1,0}, {0,1}, {-1,0}, {0,-1},  // Cardinal directions
                    {1,1}, {1,-1}, {-1,1}, {-1,-1}, // Diagonal directions
                    {0,0} // Staying still (lowest priority)
                };
            } else {
                move_options_base = {{0,1}, {0,-1}, {1,0}, {-1,0}, {0,0}}; // Cardinal + stay still
            }
            
            for (const auto& offset : move_options_base) {
                Vec2D test_pos = {
                    sprite.position.x + offset.x * effective_speed, 
                    sprite.position.y + offset.y * effective_speed
                };
                
                if (world.is_walkable(test_pos)) { 
                    valid_move_choices.push_back(offset); 
                }
            }
            
            // If still no valid moves at full speed, try half speed
            if (valid_move_choices.empty() && effective_speed > 1) {
                for (const auto& offset : move_options_base) {
                    Vec2D test_pos = {
                        sprite.position.x + offset.x, 
                        sprite.position.y + offset.y
                    };
                    
                    if (world.is_walkable(test_pos)) { 
                        valid_move_choices.push_back(offset); 
                    }
                }
            }
        }
        
        // Avoid immediately reversing direction unless no other choice
        if (valid_move_choices.size() > 1 && (sprite.lastMoveDirection.x != 0 || sprite.lastMoveDirection.y != 0)) {
            Vec2D opposite_dir = {-sprite.lastMoveDirection.x, -sprite.lastMoveDirection.y};
            
            // Only remove if it's not the only option left (excluding {0,0} if present)
            auto it = std::remove_if(valid_move_choices.begin(), valid_move_choices.end(),
                               [&](const Vec2D& choice){ return choice == opposite_dir; });
                               
            if (it != valid_move_choices.begin() || 
                (it == valid_move_choices.begin() && *it != Vec2D{0,0} && valid_move_choices.end() - it > 1)) {
                valid_move_choices.erase(it, valid_move_choices.end());
            }
        }

        if (valid_move_choices.empty()) { 
            valid_move_choices.push_back({0,0}); // Default to staying still if truly stuck
        }
        
        // Select a random move from valid options
        Vec2D move_offset = valid_move_choices[std::uniform_int_distribution<>(0, static_cast<int>(valid_move_choices.size()) - 1)(gen)];
        
        // Use effective speed including stamina
        potential_pos = {
            sprite.position.x + move_offset.x * effective_speed, 
            sprite.position.y + move_offset.y * effective_speed
        };
        
        // If potential position is not walkable, fall back to reduced speed
        if (!world.is_walkable(potential_pos) && effective_speed > 1) {
            potential_pos = {
                sprite.position.x + move_offset.x,
                sprite.position.y + move_offset.y
            };
        }
        
        sprite.lastMoveDirection = move_offset;
        sprite.stepsInCurrentDirection = (move_offset.x == 0 && move_offset.y == 0) ? MAX_STEPS_IN_DIRECTION : 1; // Reset if still or new dir

        if (sprite.type == Sprite::Type::PREDATOR && (move_offset.x != 0 || move_offset.y != 0)) {
            // Add to wander trail only if it's a new position
            if (sprite.recentWanderTrail.empty() || sprite.recentWanderTrail.front() != potential_pos) {
                sprite.recentWanderTrail.insert(sprite.recentWanderTrail.begin(), potential_pos);
                if (sprite.recentWanderTrail.size() > WANDER_TRAIL_LENGTH) { 
                    sprite.recentWanderTrail.pop_back(); 
                }
            }
        }
    }
    
    // Final sanity check - clamp position to world bounds
    sprite.position = potential_pos;
    sprite.position.x = std::max(0, std::min(sprite.position.x, world.width - 1));
    sprite.position.y = std::max(0, std::min(sprite.position.y, world.height - 1));
}

} // namespace MovementController 