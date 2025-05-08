#include "GameLogic.h"
#include "AIController.h"
#include "CaptureLogic.h"
#include "Renderer.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <conio.h>  // For _kbhit() and _getch()

// Rendering constants
const int FRAME_DELAY_MS = 100;

// Renderer state (managed globally for now)
static std::vector<std::string> previous_display_rows;
static bool first_frame = true;

namespace GameLogic {

bool handle_user_input(bool& show_paths) {
    if (_kbhit()) {
        int key = _getch(); // Store result as int
        if (key == 'p' || key == 'P') {
            show_paths = !show_paths;
            return true;
        }
    }
    return false;
}

bool process_simulation_step(std::vector<Sprite>& predators,
                            std::vector<Sprite>& prey_sprites,
                            World& world,
                            int current_step,
                            int max_steps) {
    // Process predators first - they are the priority
    for (auto& predator_sprite : predators) {
        // Update each predator - the AI controller handles speed
        AIController::update_sprite_ai(predator_sprite, predators, prey_sprites, world);
        
        // Check for captures after the move
        auto [captures, evasion_messages] = CaptureLogic::process_captures(predators, prey_sprites, world);
        
        // If any captures or evasions occurred, render and show messages
        if (captures > 0 || !evasion_messages.empty()) {
            // Render to show the capture/evasion
            Renderer::render_to_console(predators, prey_sprites, world, current_step, max_steps, previous_display_rows, first_frame, false);
            
            std::cout << "\033[" << world.height + 3 << ";1H"; // Position cursor below status line
            
            if (captures > 0) {
                std::cout << "Prey captured! " << captures << " prey caught. " << prey_sprites.size() << " remaining.\033[K" << std::endl;
            }
            
            if (!evasion_messages.empty()) {
                for (const auto& msg : evasion_messages) {
                    std::cout << msg.second << " at position (" << msg.first.x << "," << msg.first.y << ")\033[K" << std::endl;
                }
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(FRAME_DELAY_MS)); // Brief pause to show capture
            
            if (prey_sprites.empty()) {
                return false; // Exit early if all prey captured
            }
        }
    }
    
    // Exit early if all prey captured
    if (prey_sprites.empty()) {
        return false;
    }
    
    // Update prey after predators have moved
    for (auto& prey_sprite : prey_sprites) {
        AIController::update_sprite_ai(prey_sprite, predators, prey_sprites, world);
    }
    
    // Check for final captures after prey have moved
    auto [captures, evasion_messages] = CaptureLogic::process_captures(predators, prey_sprites, world);
    
    if (captures > 0 || !evasion_messages.empty()) {
        // Render to show the capture/evasion
        Renderer::render_to_console(predators, prey_sprites, world, current_step, max_steps, previous_display_rows, first_frame, false);
        std::cout << "\033[" << world.height + 3 << ";1H"; // Position cursor below status line
        
        if (captures > 0) {
            std::cout << "Prey captured after prey movement!\033[K" << std::endl;
        }
        
        if (!evasion_messages.empty()) {
            for (const auto& msg : evasion_messages) {
                std::cout << msg.second << " at position (" << msg.first.x << "," << msg.first.y << ")\033[K" << std::endl;
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(FRAME_DELAY_MS)); // Brief pause to show capture
    }
    
    if (prey_sprites.empty()) {
        // Render one last time to show capture, then exit
        Renderer::render_to_console(predators, prey_sprites, world, current_step, max_steps, previous_display_rows, first_frame, false);
        std::cout << "\033[H\033[J"; // Clear screen
        std::cout << "All prey captured!" << std::endl;
        return false;
    }
    
    return true; // Continue simulation
}

int run_simulation(std::vector<Sprite>& predators, 
                  std::vector<Sprite>& prey_sprites,
                  World& world, 
                  int max_steps) {
    int current_step = 0;
    bool show_paths = false;

    // Hide cursor
    std::cout << "\033[?25l" << std::flush;
    
    // Game loop
    while (!prey_sprites.empty() && current_step < max_steps) {
        // Process one simulation step
        bool continue_simulation = process_simulation_step(predators, prey_sprites, world, current_step, max_steps);
        
        if (!continue_simulation) {
            break;
        }
        
        // Handle user input
        handle_user_input(show_paths);
        
        // Render the current state
        Renderer::render_to_console(predators, prey_sprites, world, current_step, max_steps, previous_display_rows, first_frame, show_paths);
        
        // Delay for visualization
        std::this_thread::sleep_for(std::chrono::milliseconds(FRAME_DELAY_MS));
        current_step++;
    }
    
    // Show cursor again
    std::cout << "\033[?25h" << std::flush;
    
    // Print final status message
    if (prey_sprites.empty()) {
        std::cout << "Simulation ended: All prey captured after " << current_step << " steps." << std::endl;
    } else {
        std::cout << "Simulation ended: MAX_STEPS reached after " << current_step << " steps. " << prey_sprites.size() << " prey remaining." << std::endl;
    }
    
    return current_step;
}

} // namespace GameLogic 