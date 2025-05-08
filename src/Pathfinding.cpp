#include "Pathfinding.h"

#include <queue>         // For A* priority queue (std::priority_queue)
#include <unordered_map> // For A* came_from map and cost maps (std::unordered_map)
#include <algorithm>     // For std::reverse
#include <cmath>         // For std::abs in manhattan_distance

// Calculates Manhattan distance between two points.
int manhattan_distance(const Vec2D& p1, const Vec2D& p2) {
    return std::abs(p1.x - p2.x) + std::abs(p1.y - p2.y);
}

// Calculates squared Euclidean distance between two points (avoids sqrt for efficiency).
int squared_distance(const Vec2D& p1, const Vec2D& p2) {
    int dx = p1.x - p2.x;
    int dy = p1.y - p2.y;
    return dx * dx + dy * dy;
}

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
    start_node.hCost = manhattan_distance(start, goal);

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
            Vec2D temp = goal;
            while (temp != start) {
                path.push_back(temp);
                temp = came_from[temp];
            }
            // path.push_back(start); // Path from A* typically doesn't include start if you want "next steps"
                                   // but for a full path from start to goal, it's included here and reversed.
            std::reverse(path.begin(), path.end()); // Now path is goal -> ... -> step_before_start
                                                    // if start is added, it would be start -> ... -> goal
                                                    // The prompt for find_path mentions including start.
            // Let's clarify the path construction: we want steps *from* start *to* goal.
            // Current reconstruction goes from goal backwards to just before start.
            // So, to get start -> ... -> goal, we should add start at the end and then reverse the whole thing.
            // Or, build it backwards then reverse: path = [goal, parent_of_goal, ...], then reverse to get [..., parent_of_goal, goal]
            // Path reconstruction: if current.pos == goal, reconstruct path from came_from map
            // The current reconstruction is `path = [goal, node_before_goal, ..., node_after_start]` then reversed -> `[node_after_start, ..., node_before_goal, goal]`
            // If `path.push_back(start)` was after the loop, it'd be `[goal, ..., node_after_start, start]` -> rev `[start, node_after_start, ..., goal]` which is good.
            // Let's refine path reconstruction for clarity and correctness to match typical A* output of sequence of moves.
            // The loop `while (temp != start)` adds `goal` down to the node *after* `start`.
            // So `path` contains `[goal, node_before_goal, ..., node_after_start]`.
            // `std::reverse` makes it `[node_after_start, ..., node_before_goal, goal]`.
            // To include `start`, it must be added *first* if building forward, or *last* (before reverse) if building backward.
            // Correct A* path: start -> step1 -> ... -> goal
            // Path reconstruction should be: current = goal; while current != start: path.add(current); current = came_from[current]; path.add(start); reverse(path).
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
            return reconstructed_path;
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
                neighbor_node.hCost = manhattan_distance(neighbor_pos, goal);
                
                open_set.push(neighbor_node);
            }
        }
    }
    return {}; // Return empty path if goal not reachable
}

bool has_line_of_sight(
    const Vec2D& start,
    const Vec2D& end,
    const std::unordered_set<Vec2D>& obstacles,
    int world_width,
    int world_height
) {
    if (start == end) return true; // LoS to self is always true

    int x0 = start.x;
    int y0 = start.y;
    int x1 = end.x;
    int y1 = end.y;

    int dx = std::abs(x1 - x0);
    int dy = -std::abs(y1 - y0);

    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;

    int err = dx + dy; // error value e_xy

    while (true) {
        // Check current cell (x0, y0)
        // We don't check the start cell itself, and we want to check up to, but not including, the end cell.
        // If (x0,y0) becomes the end cell, the loop should terminate before checking it as an obstacle.

        if (x0 == x1 && y0 == y1) {
            break; // Reached the end point
        }

        // The cell (x0, y0) is on the line. If it's not the start cell, check it.
        if (!(x0 == start.x && y0 == start.y)) {
            if (obstacles.count({x0, y0})) {
                return false; // Obstacle blocks LoS
            }
            // Optional: check bounds here too, though is_path_walkable in find_path does it.
            // For LoS, if a line goes out of bounds and comes back in, it might be considered blocked by the edge.
            // For simplicity, we assume line stays in bounds or is_walkable check handles it.
            // Current is_path_walkable is static, so can't be used directly. We can inline a simple bounds check.
            if (y0 < 0 || y0 >= world_height || x0 < 0 || x0 >= world_width) {
                return false; // Line goes out of bounds
            }
        }

        int e2 = 2 * err;
        if (e2 >= dy) { // e_xy+e_x > 0
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) { // e_xy+e_y < 0
            err += dx;
            y0 += sy;
        }
    }
    return true; // No obstacles found along the line (up to the cell before the end cell)
} 