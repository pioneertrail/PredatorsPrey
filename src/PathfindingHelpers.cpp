#include "PathfindingHelpers.h"
#include <cmath>

namespace PathfindingHelpers {

int squared_distance(const Vec2D& a, const Vec2D& b) {
    int dx = a.x - b.x;
    int dy = a.y - b.y;
    return dx * dx + dy * dy;
}

int manhattan_distance(const Vec2D& a, const Vec2D& b) {
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

bool has_line_of_sight(
    const Vec2D& from, 
    const Vec2D& to, 
    const std::unordered_set<Vec2D>& obstacles,
    int width,
    int height
) {
    // Simple Bresenham's algorithm for line tracing
    int x0 = from.x;
    int y0 = from.y;
    int x1 = to.x;
    int y1 = to.y;
    
    bool steep = std::abs(y1 - y0) > std::abs(x1 - x0);
    if (steep) {
        std::swap(x0, y0);
        std::swap(x1, y1);
    }
    
    if (x0 > x1) {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }
    
    int dx = x1 - x0;
    int dy = std::abs(y1 - y0);
    int error = dx / 2;
    
    int y = y0;
    int ystep = (y0 < y1) ? 1 : -1;
    
    for (int x = x0; x <= x1; x++) {
        Vec2D p = steep ? Vec2D{y, x} : Vec2D{x, y};
        
        // Check bounds
        if (p.x < 0 || p.x >= width || p.y < 0 || p.y >= height) {
            return false;
        }
        
        // Check if point is obstacle
        if (obstacles.count(p) > 0) {
            return false;
        }
        
        error -= dy;
        if (error < 0) {
            y += ystep;
            error += dx;
        }
    }
    
    return true;
}

bool validate_and_repair_path(
    std::vector<Vec2D>& path, 
    const std::unordered_set<Vec2D>& obstacles, 
    int width, 
    int height
) {
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

} // namespace PathfindingHelpers 