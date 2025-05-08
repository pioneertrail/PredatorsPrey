#ifndef RENDERER_H
#define RENDERER_H

#include <vector>
#include <string>
#include "Sprite.h"
#include "World.h"

// Handles console rendering.
// Could be a class to manage its own state (previous_display_rows, first_frame).
namespace Renderer {

    // Renders the current game state to the console.
    // Needs access to renderer state (passed by reference or managed internally if Renderer becomes a class).
    void render_to_console(
        const std::vector<Sprite>& predators,
        const std::vector<Sprite>& prey_sprites,
        const World& world,
        int current_step, // Added for displaying progress
        int max_steps,    // Added for displaying progress
        std::vector<std::string>& previous_display_rows, // Pass state by ref for now
        bool& first_frame,                          // Pass state by ref for now
        bool showPaths // Added debug flag
    );

} // namespace Renderer

#endif // RENDERER_H 