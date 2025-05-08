#include "AIController.h"
#include "MovementController.h"
#include "PredatorAI.h"
#include "PreyAI.h"
#include "PathfindingHelpers.h"
#include <random>       // For AI logic randomness
#include <algorithm>    // For std::shuffle, std::max, std::min
#include <cmath>        // For std::abs
#include <limits> // Required for std::numeric_limits

// Use the same random generator as main.cpp for now.
// This should ideally be passed in or managed differently.
extern std::mt19937 gen; 

namespace AIController {

// Helper function to find the closest sprite from a list
const Sprite* find_closest_sprite(const Vec2D& current_pos, const std::vector<Sprite>& candidates,
                                 int& out_distance, int max_dist) {
    const Sprite* closest = nullptr;
    int min_dist_sq = std::numeric_limits<int>::max();
    out_distance = std::numeric_limits<int>::max();

    if (candidates.empty()) {
        return nullptr;
    }

    for (const auto& candidate : candidates) {
        int dist_sq = PathfindingHelpers::squared_distance(current_pos, candidate.position);
        if (dist_sq < min_dist_sq) {
            min_dist_sq = dist_sq;
            closest = &candidate;
        }
    }

    if (closest) {
        out_distance = PathfindingHelpers::manhattan_distance(current_pos, closest->position);
        if (out_distance > max_dist) {
            return nullptr;
        }
    }
    return closest;
}

// Implementation delegates to new modules
void move_randomly(Sprite& sprite, const World& world) {
    MovementController::move_randomly(sprite, world);
}

// Implementation delegates to new module
Vec2D handle_predator_path_following(Sprite& predator, const World& world) {
    return MovementController::follow_path(predator, world);
}

// Implementation delegates to new module
bool validate_and_repair_path(std::vector<Vec2D>& path,
                             const std::unordered_set<Vec2D>& obstacles, int width, int height) {
    return PathfindingHelpers::validate_and_repair_path(path, obstacles, width, height);
}

// Main update function - dispatches to the appropriate AI module
void update_sprite_ai(Sprite& sprite_to_update, const std::vector<Sprite>& all_predators,
                     const std::vector<Sprite>& all_prey, const World& world) {
    if (sprite_to_update.type == Sprite::Type::PREDATOR) {
        PredatorAI::update_predator(sprite_to_update, all_predators, all_prey, world);
    } else if (sprite_to_update.type == Sprite::Type::PREY) {
        PreyAI::update_prey(sprite_to_update, all_predators, world);
    }

    // Ensure position is valid (redundant safety check)
    sprite_to_update.position.x = std::max(0, std::min(sprite_to_update.position.x, world.width - 1));
    sprite_to_update.position.y = std::max(0, std::min(sprite_to_update.position.y, world.height - 1));
}

} // namespace AIController 