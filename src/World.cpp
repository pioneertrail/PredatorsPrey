#include "World.h"
#include "Pathfinding.h" // Include for manhattan_distance
#include <random>
#include <ctime>
#include <iostream>
#include <algorithm>
#include <functional>
#include <cmath> // For std::abs

// For random number generation (needs to be accessible or passed in)
// Option 1: Redeclare static generator here (simplest for now, but not ideal)
// Option 2: Pass generator by reference to initialize_obstacles
// Option 3: Make World responsible for its own generator
// Let's go with Option 1 for now to keep changes minimal, but note this isn't best practice.
static std::random_device rd_world; // Avoid name clash with main.cpp's static rd
static std::mt19937 gen_world(rd_world());
static std::uniform_real_distribution<float> dist(0.0f, 1.0f); // Define dist

// Helper function for Manhattan distance - REMOVED, now in Pathfinding.h/.cpp

const char OBSTACLE_CHAR = '#';
const std::string OBSTACLE_COLOR = Color::WHITE;

// Define World constructor and obstacle initialization
World::World() {
    // initialize_obstacles(); // Called by user (main.cpp) after world creation
}

void World::initialize_obstacles() {
    obstacles.clear();
    safe_zone_centers.clear(); // Clear previous safe zones

    // Create border walls
    for (int r = 0; r < height; ++r) {
        obstacles.insert({0, r});            // Left wall
        obstacles.insert({width - 1, r});    // Right wall
    }
    for (int c = 0; c < width; ++c) {
        obstacles.insert({c, 0});            // Top wall
        obstacles.insert({c, height - 1});   // Bottom wall
    }
    
    // Add some random obstacles, but with less density to allow better navigation
    float obstacle_probability = 0.06f;  // Reduced from typical 0.1-0.2 to make more open space
    
    for (int y = 2; y < height - 2; ++y) {
        for (int x = 2; x < width - 2; ++x) {
            if (dist(gen_world) < obstacle_probability) {
                obstacles.insert({x, y});
                
                // Only extend obstacles occasionally, creating smaller clusters
                if (dist(gen_world) < 0.4f && x + 1 < width - 2) { 
                    obstacles.insert({x + 1, y}); 
                }
                if (dist(gen_world) < 0.4f && y + 1 < height - 2) { 
                    obstacles.insert({x, y + 1}); 
                }
            }
        }
    }
    
    // Ensure starting areas are clear (no obstacles in corners)
    const int clear_radius = 5;
    
    // Clear top-left corner
    for (int y = 1; y < clear_radius; ++y) {
        for (int x = 1; x < clear_radius; ++x) {
            obstacles.erase({x, y});
        }
    }
    
    // Clear bottom-right corner
    for (int y = height - clear_radius; y < height - 1; ++y) {
        for (int x = width - clear_radius; x < width - 1; ++x) {
            obstacles.erase({x, y});
        }
    }
    
    // Clear center
    int center_x = width / 2;
    int center_y = height / 2;
    int center_radius = 3;
    
    for (int y = center_y - center_radius; y <= center_y + center_radius; ++y) {
        for (int x = center_x - center_radius; x <= center_x + center_radius; ++x) {
            if (x > 0 && x < width - 1 && y > 0 && y < height - 1) {
                obstacles.erase({x, y});
            }
        }
    }
    
    // Remove obvious dead ends and isolated obstacles
    clean_up_obstacles();

    // Initialize a few safe zones (example locations)
    // Ensure these are walkable initially, though they might overlap with random obstacles later.
    // It's better to pick areas that are likely to remain open.
    safe_zone_centers.push_back({10, 10}); 
    safe_zone_centers.push_back({width - 10, height - 10});
    safe_zone_centers.push_back({width / 2, 5});

    // Optional: Remove any obstacles that fall within the immediate center of a safe zone
    for(const auto& center : safe_zone_centers){
        obstacles.erase(center);
    }
}

bool World::is_walkable(int r, int c) const {
    if (r < 0 || r >= height || c < 0 || c >= width) {
        return false;
    }
    return obstacles.count({c, r}) == 0;
}

// Overload for convenience
bool World::is_walkable(const Vec2D& pos) const {
    return is_walkable(pos.y, pos.x);
}

// Helper function to clean up obstacles
void World::clean_up_obstacles() {
    std::vector<Vec2D> to_remove;
    
    // Find isolated obstacles (ones surrounded by 6+ empty cells)
    for (const auto& obs : obstacles) {
        if (obs.x <= 0 || obs.x >= width - 1 || obs.y <= 0 || obs.y >= height - 1) {
            continue; // Skip border walls
        }
        
        int empty_neighbors = 0;
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                if (dx == 0 && dy == 0) continue;
                
                Vec2D neighbor = {obs.x + dx, obs.y + dy};
                if (obstacles.count(neighbor) == 0) {
                    empty_neighbors++;
                }
            }
        }
        
        if (empty_neighbors >= 6) {
            to_remove.push_back(obs);
        }
    }
    
    // Remove isolated obstacles
    for (const auto& pos : to_remove) {
        obstacles.erase(pos);
    }
    
    // Find dead ends (cells with 3+ adjacent obstacles)
    to_remove.clear();
    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            if (obstacles.count({x, y}) == 0) {
                // Count obstacles around this open cell
                int adjacent_obstacles = 0;
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        if (dx == 0 && dy == 0) continue;
                        
                        if (obstacles.count({x + dx, y + dy}) > 0) {
                            adjacent_obstacles++;
                        }
                    }
                }
                
                // If this is effectively a dead end, remove an adjacent obstacle
                if (adjacent_obstacles >= 5) {
                    // Check cardinal directions (prefer to keep diagonals as passages)
                    int dx_dirs[] = {1, 0, -1, 0};
                    int dy_dirs[] = {0, 1, 0, -1};
                    
                    for (int i = 0; i < 4; ++i) {
                        Vec2D neighbor = {x + dx_dirs[i], y + dy_dirs[i]};
                        if (obstacles.count(neighbor) > 0 &&
                            neighbor.x > 0 && neighbor.x < width - 1 && 
                            neighbor.y > 0 && neighbor.y < height - 1) {
                            to_remove.push_back(neighbor);
                            break;
                        }
                    }
                }
            }
        }
    }
    
    // Remove obstacles creating dead ends
    for (const auto& pos : to_remove) {
        obstacles.erase(pos);
    }
}

const std::vector<Vec2D>& World::get_safe_zone_centers() const {
    return safe_zone_centers;
}

bool World::is_in_safe_zone(const Vec2D& pos) const {
    for (const auto& center : safe_zone_centers) {
        // Using Manhattan distance for simplicity to define a square-ish zone
        if (manhattan_distance(pos, center) <= safe_zone_radius) {
            return true;
        }
    }
    return false;
}

// Private helper to add random obstacles
void World::add_random_obstacles([[maybe_unused]] int count) {
    // Implementation of add_random_obstacles method
} 