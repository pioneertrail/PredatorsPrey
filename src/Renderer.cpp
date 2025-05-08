#include "Renderer.h"
#include <iostream> // For std::cout
#include <iomanip>  // For formatting

// ANSI escape codes
const std::string ANSI_MOVE_CURSOR_TO_START = "\033[H";
const std::string ANSI_CLEAR_SCREEN = "\033[2J";
const std::string ANSI_CLEAR_SCREEN_BELOW = "\033[J"; // Clear from cursor to end of screen

// Add more colors for differentiation
namespace ColorExt {
    const std::string BRIGHT_RED = "\033[91m";
    const std::string BRIGHT_GREEN = "\033[92m";
    const std::string BRIGHT_BLUE = "\033[94m";
    const std::string BRIGHT_MAGENTA = "\033[95m";
    const std::string BRIGHT_CYAN = "\033[96m";
}

namespace Renderer {

void render_to_console(
    const std::vector<Sprite>& predators,
    const std::vector<Sprite>& prey_sprites,
    const World& world,
    int current_step,
    int max_steps,
    std::vector<std::string>& previous_display_rows, 
    bool& first_frame
) {
    // Predator colors for differentiation
    const std::string predator_colors[] = {
        Color::RED, 
        ColorExt::BRIGHT_MAGENTA,
        ColorExt::BRIGHT_CYAN
    };
    
    // Initialize display rows with exactly world.width chars per row
    std::vector<std::string> current_display_rows(world.height);
    
    // Create a basic grid of spaces
    for (int r = 0; r < world.height; ++r) {
        current_display_rows[r] = std::string(world.width, ' ');
    }
    
    // Add obstacles first (no color codes yet, just character placement)
    for (int r = 0; r < world.height; ++r) {
        for (int c = 0; c < world.width; ++c) {
            if (world.obstacles.count({c, r})) {
                current_display_rows[r][c] = world.obstacleChar;
            }
        }
    }

    // Add prey next (character placement only)
    for (const auto& prey : prey_sprites) {
        if (prey.position.y >= 0 && prey.position.y < world.height && 
            prey.position.x >= 0 && prey.position.x < world.width) {
            if (!world.obstacles.count(prey.position)) {
                current_display_rows[prey.position.y][prey.position.x] = prey.displayChar;
            }
        }
    }

    // Add predators last (character placement only)
    for (size_t i = 0; i < predators.size(); ++i) {
        const auto& predator = predators[i];
        if (predator.position.y >= 0 && predator.position.y < world.height && 
            predator.position.x >= 0 && predator.position.x < world.width) {
            char predator_num = static_cast<char>('1' + i); // Convert index to character 1, 2, 3...
            current_display_rows[predator.position.y][predator.position.x] = predator_num;
        }
    }

    // Clear screen first frame, just move cursor for subsequent frames
    if (first_frame) {
        std::cout << ANSI_CLEAR_SCREEN << ANSI_MOVE_CURSOR_TO_START << std::flush;
        first_frame = false;
    } else {
        std::cout << ANSI_MOVE_CURSOR_TO_START << std::flush;
    }
    
    // Draw top border
    std::cout << "+" << std::string(world.width, '-') << "+" << std::endl;
    
    // Draw rows with borders and apply colors
    for (int r = 0; r < world.height; ++r) {
        std::cout << "|";
        // Print each character with appropriate color
        for (int c = 0; c < world.width; ++c) {
            char ch = current_display_rows[r][c];
            
            // Apply colors based on character type
            if (ch == world.obstacleChar) {
                std::cout << world.obstacleColor << ch << Color::RESET;
            } else if (ch == 'Y') { // Prey
                std::cout << Color::YELLOW << ch << Color::RESET;
            } else if (ch >= '1' && ch <= '9') { // Predator
                int pred_idx = ch - '1';
                std::string color = (pred_idx < 3) ? predator_colors[pred_idx] : Color::RED;
                std::cout << color << ch << Color::RESET;
            } else {
                std::cout << ch; // Just a space or other character
            }
        }
        std::cout << "|" << std::endl;
    }
    
    // Draw bottom border
    std::cout << "+" << std::string(world.width, '-') << "+" << std::endl;
    
    // Save current display for next frame comparison
    previous_display_rows = current_display_rows;
    
    // Reset color and print status information
    std::cout << Color::RESET;
    
    // Status line
    std::cout << "Step: " << std::setw(4) << current_step << "/" << max_steps
              << "   Predators: " << predators.size()
              << "   Prey: " << prey_sprites.size() << std::endl;
    
    // Predator status line - limit to first 3 predators
    for (size_t i = 0; i < predators.size() && i < 3; ++i) {
        const auto& p = predators[i];
        std::string pred_state_str;
        std::string color = (i < 3) ? predator_colors[i] : Color::RED;
        
        // Convert state enum to readable string
        switch (p.currentState) {
            case Sprite::AIState::SEEKING: pred_state_str = "SEEKING"; break;
            case Sprite::AIState::SEARCHING_LKP: pred_state_str = "SEARCH_LKP"; break;
            case Sprite::AIState::WANDERING: default: pred_state_str = "WANDERING"; break;
        }
        
        // Display predator info with color
        std::cout << color << "Predator " << (i+1) << ": "
                  << "(" << p.position.x << "," << p.position.y << ") "
                  << "[" << pred_state_str << "]" << Color::RESET;
        
        // Add separator between predators
        if (i < predators.size() - 1 && i < 2) std::cout << " | ";
    }
    std::cout << std::endl;
}

} // namespace Renderer 