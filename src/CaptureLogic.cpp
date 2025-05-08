#include "CaptureLogic.h"
#include <algorithm>
#include <cmath>

namespace CaptureLogic {

Vec2D calculate_escape_position(const Sprite& prey, const Sprite& predator, const World& world) {
    // Calculate the direction vector from predator to prey
    int dx = prey.position.x - predator.position.x;
    int dy = prey.position.y - predator.position.y;
    
    // If dx or dy is zero, pick a random direction for that component
    int escape_dx = (dx == 0) ? (std::uniform_int_distribution<int>(0, 1)(gen) * 2 - 1) : ((dx > 0) ? 1 : -1);
    int escape_dy = (dy == 0) ? (std::uniform_int_distribution<int>(0, 1)(gen) * 2 - 1) : ((dy > 0) ? 1 : -1);
    
    // Move prey 2-3 spaces away to escape
    int escape_distance = std::uniform_int_distribution<int>(2, 3)(gen);
    Vec2D escape_pos = {
        prey.position.x + escape_dx * escape_distance,
        prey.position.y + escape_dy * escape_distance
    };
    
    // Ensure escape position is within bounds and walkable
    escape_pos.x = std::max(0, std::min(escape_pos.x, world.width - 1));
    escape_pos.y = std::max(0, std::min(escape_pos.y, world.height - 1));
    
    // If position isn't walkable, find a nearby position that is
    if (!world.is_walkable(escape_pos)) {
        // Try nearby positions in a small radius
        for (int r = 1; r <= 3; ++r) {
            for (int y_off = -r; y_off <= r; ++y_off) {
                for (int x_off = -r; x_off <= r; ++x_off) {
                    // Skip the original position
                    if (x_off == 0 && y_off == 0) continue;
                    
                    Vec2D test_pos = {
                        prey.position.x + x_off,
                        prey.position.y + y_off
                    };
                    
                    // Ensure within bounds
                    test_pos.x = std::max(0, std::min(test_pos.x, world.width - 1));
                    test_pos.y = std::max(0, std::min(test_pos.y, world.height - 1));
                    
                    if (world.is_walkable(test_pos)) {
                        return test_pos; // Found a valid position
                    }
                }
            }
        }
        
        // If we couldn't find a walkable position, just return the prey's current position
        return prey.position;
    }
    
    return escape_pos;
}

bool attempt_capture(Sprite& predator, Sprite& prey, const World& world,
                    std::vector<std::pair<Vec2D, std::string>>& evasion_messages,
                    int predator_index) {
    // Skip if predator is stunned
    if (predator.isStunned) return false;
    
    // Check distance between predator and prey
    int dx = prey.position.x - predator.position.x;
    int dy = prey.position.y - predator.position.y;
    
    // If within capture distance (adjacent cells including diagonals)
    if (std::abs(dx) <= 1 && std::abs(dy) <= 1) {
        // Calculate dynamic evasion chance based on fear
        float dynamicEvasionChance = prey.evasionChance + (prey.currentFear / prey.maxFear) * 0.15f; // Max 15% bonus from fear
        dynamicEvasionChance = std::min(dynamicEvasionChance, 0.9f); // Cap evasion at 90%

        // Check for evasion
        float evasion_roll = std::uniform_real_distribution<float>(0.0f, 1.0f)(gen);
        
        if (evasion_roll <= dynamicEvasionChance) {
            // Evasion successful! Stun the predator
            predator.isStunned = true;
            predator.stunDuration = 2; // Stun for 2 frames
            predator.currentState = Sprite::AIState::STUNNED;
            
            // Calculate and apply escape position
            Vec2D escape_pos = calculate_escape_position(prey, predator, world);
            prey.position = escape_pos;
            
            // Store message about evasion for display
            evasion_messages.push_back({prey.position, "Prey escaped from Predator " + std::to_string(predator_index + 1)});
            
            return false; // Prey escaped
        } else {
            // No evasion - prey is captured
            return true;
        }
    }
    
    return false; // Not close enough to capture
}

std::pair<size_t, std::vector<std::pair<Vec2D, std::string>>> 
process_captures(std::vector<Sprite>& predators, 
                std::vector<Sprite>& prey_sprites,
                const World& world) {
    size_t initial_prey_count = prey_sprites.size();
    std::vector<size_t> indices_to_remove;
    std::vector<std::pair<Vec2D, std::string>> evasion_messages;
    
    // Check each prey against each predator
    for (size_t i = 0; i < prey_sprites.size(); ++i) {
        auto& prey = prey_sprites[i];
        
        // Check if any predator can capture this prey
        for (size_t p_idx = 0; p_idx < predators.size(); ++p_idx) {
            auto& predator = predators[p_idx];
            
            if (attempt_capture(predator, prey, world, evasion_messages, p_idx)) {
                // Prey was captured
                indices_to_remove.push_back(i);
                break; // This prey is captured, no need to check other predators
            }
        }
    }
    
    // Remove captured prey in reverse order to maintain valid indices
    std::sort(indices_to_remove.begin(), indices_to_remove.end(), std::greater<size_t>());
    for (size_t index : indices_to_remove) {
        if (index < prey_sprites.size()) { // Safety check
            prey_sprites.erase(prey_sprites.begin() + index);
        }
    }
    
    // Return number of captures and evasion messages
    size_t captures = initial_prey_count - prey_sprites.size();
    return {captures, evasion_messages};
}

} // namespace CaptureLogic 