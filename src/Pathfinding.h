#ifndef PATHFINDING_H
#define PATHFINDING_H

#include <vector>
#include <unordered_set> // For passing obstacles
#include "Vec2D.h"       // For Vec2D struct

// --- A* Pathfinding Data Structures ---
struct AStarNode {
    Vec2D pos;      // Current position
    Vec2D parent;   // Position of the node we came from
    int gCost;      // Cost from start node to this node
    int hCost;      // Heuristic cost estimate from this node to goal
    int fCost() const { return gCost + hCost; } // Total estimated cost

    // Needed for priority queue ordering (min-heap based on fCost)
    bool operator>(const AStarNode& other) const {
        if (fCost() != other.fCost()) {
             return fCost() > other.fCost();
        }
        return hCost > other.hCost;
    }
    bool operator==(const AStarNode& other) const {
        return pos == other.pos;
    }
};

// --- A* Pathfinding Function Declarations ---

// Calculates Manhattan distance between two points.
int manhattan_distance(const Vec2D& p1, const Vec2D& p2);

// Finds a path from start to goal using A* algorithm.
// Returns a vector of Vec2D points representing the path (including start, excluding goal if goal is the last step taken from).
// Returns an empty vector if no path is found.
std::vector<Vec2D> find_path(
    const Vec2D& start,
    const Vec2D& goal,
    const std::unordered_set<Vec2D>& obstacles,
    int world_width,
    int world_height
);

// Checks for a clear line of sight between two points, considering obstacles.
// Returns true if LoS is clear, false otherwise.
bool has_line_of_sight(
    const Vec2D& start,
    const Vec2D& end,
    const std::unordered_set<Vec2D>& obstacles,
    int world_width,
    int world_height
);

#endif // PATHFINDING_H 