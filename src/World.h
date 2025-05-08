#ifndef WORLD_H
#define WORLD_H

#include <vector>
#include <string>
#include <unordered_set>
#include "Vec2D.h"      // For Vec2D struct
#include "Sprite.h" // Include Sprite.h for Color namespace

struct World {
    // Constants
    const int width = 60;  // Reduced from 80 for better console rendering
    const int height = 20; // Kept the same height
    const char obstacleChar = '#';
    const std::string obstacleColor = Color::WHITE;
    const char safeZoneChar = '~'; // Character for safe zone tiles
    const std::string safeZoneColor = Color::GREEN; // Color for safe zone tiles

    // Data
    std::unordered_set<Vec2D> obstacles;
    std::vector<Vec2D> safe_zone_centers; // Added for safe zones
    const int safe_zone_radius = 2;      // Added for safe zones

    // Constructor to initialize obstacles
    World(); 

    // Methods
    // Check if a position is within bounds AND not an obstacle
    bool is_walkable(int r, int c) const;
    bool is_walkable(const Vec2D& pos) const; // Overload for convenience
    bool is_valid(const Vec2D& pos) const;

    // Method to initialize/re-initialize obstacles (could be called by constructor)
    void initialize_obstacles();

    const std::vector<Vec2D>& get_safe_zone_centers() const; // Added
    bool is_in_safe_zone(const Vec2D& pos) const;           // Added

private:
    // Helper function to clean up obstacles after initial generation
    void clean_up_obstacles();
    void add_random_obstacles([[maybe_unused]] int count);
};

#endif // WORLD_H 