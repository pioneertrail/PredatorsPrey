#include "Pathfinding.h"
#include "PathfindingHelpers.h"

#include <queue>         // For A* priority queue (std::priority_queue)
#include <unordered_map> // For A* came_from map and cost maps (std::unordered_map)
#include <algorithm>     // For std::reverse
#include <cmath>         // For std::abs

// Helper function for find_path to check walkability within Pathfinding.cpp context
static bool is_path_walkable(int r, int c, int world_width, int world_height, const std::unordered_set<Vec2D>& obstacles) {
    if (r < 0 || r >= world_height || c < 0 || c >= world_width) {
        return false; // Out of bounds
    }
    if (obstacles.count({c, r})) { // Check if position (x,y) is in the obstacle set
        return false; // Obstacle present
    }
    return true;
}

std::vector<Vec2D> find_path(
    const Vec2D& start,
    const Vec2D& goal,
    const std::unordered_set<Vec2D>& obstacles,
    int world_width,
    int world_height
) {
    std::vector<Vec2D> path;
    std::priority_queue<AStarNode, std::vector<AStarNode>, std::greater<AStarNode>> open_set;
    std::unordered_map<Vec2D, Vec2D> came_from; // Using Vec2D as key directly thanks to std::hash<Vec2D>
    std::unordered_map<Vec2D, int> g_cost;

    g_cost[start] = 0;

    AStarNode start_node;
    start_node.pos = start;
    start_node.parent = start; 
    start_node.gCost = 0;
    start_node.hCost = PathfindingHelpers::manhattan_distance(start, goal);

    open_set.push(start_node);

    // Add diagonal movement options for more efficient pathfinding
    const Vec2D neighbors_offset[] = {
        {0, 1}, {0, -1}, {1, 0}, {-1, 0},  // Cardinal directions
        {1, 1}, {1, -1}, {-1, 1}, {-1, -1} // Diagonal directions
    };

    while (!open_set.empty()) {
        AStarNode current = open_set.top();
        open_set.pop();

        if (current.pos == goal) {
            // Path reconstruction
            std::vector<Vec2D> reconstructed_path;
            Vec2D trace_back_node = goal;
            while(trace_back_node != start) {
                reconstructed_path.push_back(trace_back_node);
                if (came_from.find(trace_back_node) == came_from.end()) { 
                    return {}; // Should not happen if goal is reachable and start != goal
                }
                trace_back_node = came_from[trace_back_node];
            }
            reconstructed_path.push_back(start);
            std::reverse(reconstructed_path.begin(), reconstructed_path.end());
            
            // Validate the path
            if (PathfindingHelpers::validate_and_repair_path(reconstructed_path, obstacles, world_width, world_height)) {
                return reconstructed_path;
            }
            return {}; // Path validation failed
        }

        for (const auto& offset : neighbors_offset) {
            Vec2D neighbor_pos = {current.pos.x + offset.x, current.pos.y + offset.y};

            if (!is_path_walkable(neighbor_pos.y, neighbor_pos.x, world_width, world_height, obstacles)) {
                continue;
            }

            int tentative_g_cost = current.gCost + 1;

            if (g_cost.find(neighbor_pos) == g_cost.end() || tentative_g_cost < g_cost[neighbor_pos]) {
                came_from[neighbor_pos] = current.pos;
                g_cost[neighbor_pos] = tentative_g_cost;
                
                AStarNode neighbor_node;
                neighbor_node.pos = neighbor_pos;
                neighbor_node.parent = current.pos;
                neighbor_node.gCost = tentative_g_cost;
                neighbor_node.hCost = PathfindingHelpers::manhattan_distance(neighbor_pos, goal);
                
                open_set.push(neighbor_node);
            }
        }
    }
    return {}; // Return empty path if goal not reachable
} 