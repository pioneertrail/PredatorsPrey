#include "GridRenderer.h"
#include <iostream>

// ANSI escape codes
const std::string ANSI_MOVE_CURSOR_TO_START = "\033[H";
const std::string ANSI_CLEAR_SCREEN = "\033[2J";
const std::string ANSI_CLEAR_SCREEN_BELOW = "\033[J"; // Clear from cursor to end of screen

// Add more colors for differentiation
namespace ColorExt {
    const std::string BRIGHT_RED = "\033[91m";
    const std::string BRIGHT_GREEN = "\033[92m";
    const std::string BRIGHT_YELLOW = "\033[93m";
    const std::string BRIGHT_BLUE = "\033[94m";
    const std::string BRIGHT_MAGENTA = "\033[95m";
    const std::string BRIGHT_CYAN = "\033[96m";
}

namespace GridRenderer {

const char pathChar = '.';

std::vector<std::string> prepare_display_grid(
    const std::vector<Sprite>& predators,
    const std::vector<Sprite>& prey_sprites,
    const World& world,
    bool show_paths
) {
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
            } else if (world.is_in_safe_zone({c,r})) { // Check after obstacles
                current_display_rows[r][c] = world.safeZoneChar;
            }
        }
    }

    // Draw Paths (if enabled) - Draw AFTER obstacles/zones but BEFORE sprites
    if (show_paths) {
        auto draw_path = [&](const std::vector<Sprite>& sprites) {
            for (const auto& sprite : sprites) {
                if (!sprite.currentPath.empty()) {
                    for (size_t i = 0; i < sprite.currentPath.size(); ++i) {
                        const Vec2D& pos = sprite.currentPath[i];
                        // Check bounds and ensure we don't overwrite obstacles or existing sprites
                        if (pos.y >= 0 && pos.y < world.height && pos.x >= 0 && pos.x < world.width) {
                            if (current_display_rows[pos.y][pos.x] == ' ' || current_display_rows[pos.y][pos.x] == world.safeZoneChar) {
                                // Only draw path on empty space or safe zones
                                current_display_rows[pos.y][pos.x] = pathChar;
                            } // Else: Don't draw path over obstacles or other sprites
                        }
                    }
                }
            }
        };
        draw_path(predators);
        draw_path(prey_sprites);
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
    
    return current_display_rows;
}

void draw_grid_to_console(
    const std::vector<std::string>& current_display_rows,
    const std::vector<Sprite>& predators,
    const std::vector<Sprite>& prey_sprites,
    const World& world,
    bool& first_frame
) {
    // Predator colors for differentiation
    const std::string predator_colors[] = {
        Color::RED, 
        ColorExt::BRIGHT_MAGENTA,
        ColorExt::BRIGHT_CYAN
    };
    
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
            char ch_on_grid = current_display_rows[r][c];
            std::string current_color = Color::RESET;
            char char_to_print = ch_on_grid;
            bool color_set = false;

            // Apply colors based on character type
            if (ch_on_grid == world.obstacleChar) {
                current_color = world.obstacleColor;
                color_set = true;
            } else if (ch_on_grid == world.safeZoneChar) { // Safe Zone character
                current_color = world.safeZoneColor;
                color_set = true;
            } else if (ch_on_grid == pathChar) { // Path character
                current_color = Color::CYAN; // Use Cyan for paths
                color_set = true;
            } else if (ch_on_grid == 'Y') { // Prey
                // Find the prey sprite at this location
                const Sprite* current_prey = nullptr;
                for (const auto& p_sprite : prey_sprites) {
                    if (p_sprite.position.x == c && p_sprite.position.y == r) {
                        current_prey = &p_sprite;
                        break;
                    }
                }
                if (current_prey) {
                    if (current_prey->currentState == Sprite::AIState::FLEEING) {
                        if (current_prey->currentFear > current_prey->maxFear * 0.75f) {
                            current_color = ColorExt::BRIGHT_YELLOW;
                            char_to_print = '!';
                        } else {
                            current_color = Color::YELLOW;
                            char_to_print = 'Y';
                        }
                    } else { // WANDERING or other states for prey
                        current_color = Color::YELLOW;
                        char_to_print = 'Y';
                    }
                } else { // Should not happen if grid char is 'Y' from placement phase
                    current_color = Color::YELLOW;
                    char_to_print = 'Y';
                }
                color_set = true;
            } else if (ch_on_grid >= '1' && ch_on_grid <= '9') { // Predator
                int pred_idx = ch_on_grid - '1';
                if (pred_idx < static_cast<int>(predators.size())) {
                    const auto& predator = predators[pred_idx];
                    char_to_print = ch_on_grid; // Default to predator number

                    switch (predator.currentState) {
                        case Sprite::AIState::SEEKING:
                            current_color = (predator.currentStamina > 0) ? ColorExt::BRIGHT_RED : Color::RED;
                            break;
                        case Sprite::AIState::RESTING:
                            current_color = Color::CYAN;
                            char_to_print = 'R';
                            break;
                        case Sprite::AIState::SEARCHING_LKP:
                            current_color = Color::MAGENTA;
                            char_to_print = '?';
                            break;
                        case Sprite::AIState::STUNNED:
                            current_color = ColorExt::BRIGHT_BLUE; // Using bright blue for stunned
                            char_to_print = 's';
                            break;
                        case Sprite::AIState::WANDERING:
                        default:
                            current_color = (pred_idx < 3) ? predator_colors[pred_idx] : Color::RED;
                            break;
                    }
                } else { // Should not happen if grid char is a valid predator number
                    current_color = Color::RED;
                }
                color_set = true;
            }

            if (color_set) {
                std::cout << current_color << char_to_print << Color::RESET;
            } else {
                std::cout << char_to_print; // Just a space or other character
            }
        }
        std::cout << "|" << std::endl;
    }
    
    // Draw bottom border
    std::cout << "+" << std::string(world.width, '-') << "+" << std::endl;
}

} // namespace GridRenderer 