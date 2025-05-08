#ifndef PATHFINDING_HELPERS_H
#define PATHFINDING_HELPERS_H

#include "Vec2D.h"
#include <vector>
#include <unordered_set>

namespace PathfindingHelpers {
    // Validate and repair a path if possible, return false if invalid
    bool validate_and_repair_path(
        std::vector<Vec2D>& path, 
        const std::unordered_set<Vec2D>& obstacles, 
        int width, 
        int height
    );
    
    // Calculate squared distance between two points (faster than Manhattan for some comparisons)
    int squared_distance(const Vec2D& a, const Vec2D& b);
    
    // Calculate Manhattan distance between two points (used for AI behavior decisions)
    int manhattan_distance(const Vec2D& a, const Vec2D& b);
    
    // Check if there's a direct line of sight between two points
    bool has_line_of_sight(
        const Vec2D& from, 
        const Vec2D& to, 
        const std::unordered_set<Vec2D>& obstacles,
        int width,
        int height
    );
}

#endif // PATHFINDING_HELPERS_H 