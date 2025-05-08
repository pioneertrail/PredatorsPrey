#include "Renderer.h"
#include "GridRenderer.h"
#include "StatusDisplay.h"

namespace Renderer {

void render_to_console(
    const std::vector<Sprite>& predators,
    const std::vector<Sprite>& prey_sprites,
    const World& world,
    int current_step,
    int max_steps,
    std::vector<std::string>& previous_display_rows, 
    bool& first_frame,
    bool showPaths
) {
    // Prepare the grid display data
    std::vector<std::string> current_display_rows = 
        GridRenderer::prepare_display_grid(predators, prey_sprites, world, showPaths);
    
    // Draw the grid with borders and colors
    GridRenderer::draw_grid_to_console(current_display_rows, predators, prey_sprites, world, first_frame);
    
    // Display simulation status and statistics
    StatusDisplay::display_simulation_status(predators, prey_sprites, current_step, max_steps);
    
    // Display detailed predator information
    StatusDisplay::display_predator_status(predators);
    
    // Save current display for next frame comparison
    previous_display_rows = current_display_rows;
}

} // namespace Renderer 