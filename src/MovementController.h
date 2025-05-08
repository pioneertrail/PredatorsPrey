#ifndef MOVEMENT_CONTROLLER_H
#define MOVEMENT_CONTROLLER_H

#include "Sprite.h"
#include "World.h"
#include "Vec2D.h"

namespace MovementController {
    // Move a sprite randomly, respecting movement rules and sprite state
    void move_randomly(Sprite& sprite, const World& world);
    
    // Move a sprite along a path
    Vec2D follow_path(Sprite& sprite, const World& world);
    
    // Calculate effective speed for a sprite based on type, state, and stamina
    int calculate_effective_speed(Sprite& sprite);
    
    // Find valid moves for a sprite from its current position
    std::vector<Vec2D> get_valid_moves(const Sprite& sprite, const World& world, int effective_speed);
}

#endif // MOVEMENT_CONTROLLER_H 