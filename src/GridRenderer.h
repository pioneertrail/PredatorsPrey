#ifndef GRID_RENDERER_H
#define GRID_RENDERER_H

#include <vector>
#include <string>
#include "Sprite.h"
#include "World.h"

namespace GridRenderer {
    // Initialize the display grid with all characters that will be displayed
    // Returns a vector of strings representing the grid with characters (without colors)
    std::vector<std::string> prepare_display_grid(
        const std::vector<Sprite>& predators,
        const std::vector<Sprite>& prey_sprites,
        const World& world,
        bool show_paths
    );
    
    // Draw the grid to the console with borders and appropriate colors
    void draw_grid_to_console(
        const std::vector<std::string>& current_display_rows,
        const std::vector<Sprite>& predators,
        const std::vector<Sprite>& prey_sprites,
        const World& world,
        bool& first_frame
    );
    
    // Define pathChar for visibility in other modules if needed
    extern const char pathChar;
}

#endif // GRID_RENDERER_H 