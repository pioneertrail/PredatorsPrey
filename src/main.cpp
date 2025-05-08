#include <iostream>
#include <vector>
#include <string>
#include <chrono> // For basic timing if needed for delays
#include <thread> // For std::this_thread::sleep_for
#include <cmath> // For std::abs for distance checking
#include <random> // For prey's random movement
#include <algorithm> // For std::shuffle, std::min, std::max, std::remove_if
#include <iomanip> // For std::setw, std::left if needed for formatting status
#include <unordered_set> // For efficient obstacle lookup
#include <queue> // Added for std::priority_queue (used by Pathfinding.h indirectly via AStarNode comparison)
#include <limits>    // For std::numeric_limits
#include <cstdlib>   // For getenv function

#include "Vec2D.h"       // Added
#include "Sprite.h"
#include "Pathfinding.h" // Added
#include "World.h" // Added
#include "AIController.h" // Added
#include "Renderer.h" // Added

// Rendering constants
const int FRAME_DELAY_MS = 100;

// ANSI escape codes
const std::string ANSI_MOVE_CURSOR_TO_START = "\033[H";
const std::string ANSI_CLEAR_SCREEN = "\033[2J";
const std::string ANSI_CLEAR_SCREEN_BELOW = "\033[J"; // Clear from cursor to end of screen

// Global Random Generator (Non-static for extern linkage in AIController.cpp)
static std::random_device rd;
std::mt19937 gen(rd()); // Non-static

// Renderer State (Managed globally here for now, passed to Renderer)
static std::vector<std::string> previous_display_rows;
static bool first_frame = true;

// Removed Obstacle Data (now in World)
// static std::unordered_set<Vec2D> obstacles;
// const char OBSTACLE_CHAR = '#';
// const std::string OBSTACLE_COLOR = Color::WHITE;

// Removed global is_walkable (now World::is_walkable)
// Removed global initialize_obstacles (now World::initialize_obstacles)

// Removed AI Parameters
// const int PREDATOR_VISION_RADIUS = 40;
// ... (other AI constants) ...

// Removed AI function forward declarations

// Removed AI function implementations: move_randomly, handle_predator_path_following, update_ai

// Added function to get max steps from environment or use default
int getMaxSteps() {
    // Try to get MAX_STEPS from environment
    char* env_val_buffer = nullptr;
    size_t buffer_size = 0;
    errno_t err = _dupenv_s(&env_val_buffer, &buffer_size, "MAX_STEPS");

    if (err == 0 && env_val_buffer != nullptr) {
        try {
            int steps = std::stoi(env_val_buffer);
            free(env_val_buffer); // Free the buffer allocated by _dupenv_s
            return steps;
        } catch (const std::exception&) {
            // In case of conversion error, use default
            free(env_val_buffer); // Free the buffer in case of error too
            return 100000;
        }
    }
    // Default if not set or if _dupenv_s failed (env_val_buffer would be null)
    if (env_val_buffer) { // Should be null if err != 0, but good practice
        free(env_val_buffer);
    }
    return 100000;
}

// Get MAX_STEPS from environment variable or use default
const int MAX_STEPS = getMaxSteps();

// Constants
const int NUM_PREDATORS = 3;
const int NUM_PREY = 6;

// --- Main Function --- 
int main(int /*argc*/, char* /*argv*/[]) {
    World world;
    world.initialize_obstacles();

    std::vector<Sprite> predators;
    predators.reserve(NUM_PREDATORS);
    for (int i = 0; i < NUM_PREDATORS; ++i) {
        Sprite p;
        // Spread predators around more
        p.position = {10 + i * 20, 5 + i * 5}; // Better distribution
        p.displayChar = 'P';
        p.colorCode = Color::RED;
        p.currentState = Sprite::AIState::WANDERING;
        p.type = Sprite::Type::PREDATOR;
        p.speed = 2; // Base speed for predators
        p.turnsSincePathReplan = 0;
        p.pathFollowStep = 0;
        p.stepsInCurrentDirection = 0;
        p.lastMoveDirection = {0,0};
        
        // Initialize stamina properties
        p.maxStamina = 5;
        p.currentStamina = 5;
        p.staminaRechargeTime = 10;
        p.staminaRechargeCounter = 0;
        
        predators.push_back(p);
    }

    std::vector<Sprite> prey_sprites;
    prey_sprites.reserve(NUM_PREY);
    for (int i = 0; i < NUM_PREY; ++i) {
        Sprite p;
        // Distribute prey more widely
        p.position = {10 + (i % 3) * 15, 10 + (i / 3) * 5}; // Adjusted for better spread
        while (!world.is_walkable(p.position)) { // Ensure not starting in an obstacle
            p.position.x = (p.position.x + 1) % world.width;
            if (p.position.x == 0) p.position.y = (p.position.y + 1) % world.height;
        }
        p.displayChar = 'Y';
        p.colorCode = Color::YELLOW;
        p.currentState = Sprite::AIState::WANDERING;
        p.type = Sprite::Type::PREY;
        p.speed = 1; // Standard prey speed
        p.stepsInCurrentDirection = 0;
        p.lastMoveDirection = {0,0};
        
        // Initialize evasion properties
        p.evasionChance = 0.35f;  // 35% chance to evade 
        
        prey_sprites.push_back(p);
    }

    int current_step = 0;

    // Hide cursor
    std::cout << "\033[?25l" << std::flush;
    
    // Game loop
    while (!prey_sprites.empty() && current_step < MAX_STEPS) {
        // Process predators first - they are the priority (speed = 2)
        for (auto& predator_sprite : predators) {
            // Update each predator - the AI controller now properly handles speed
            AIController::update_sprite_ai(predator_sprite, predators, prey_sprites, world);
            
            // Check for captures after the move
            size_t initial_prey_count = prey_sprites.size();
            
            // Attempt to capture prey with evasion chance
            std::vector<size_t> indices_to_remove;
            std::vector<std::pair<Vec2D, std::string>> evasion_messages;
            
            for (size_t i = 0; i < prey_sprites.size(); ++i) {
                const auto& prey = prey_sprites[i];
                
                // Check if any predator is close enough to capture this prey
                for (auto& predator : predators) {
                    // Skip if predator is stunned
                    if (predator.isStunned) continue;
                    
                    // Check capture distance
                    int dx = prey.position.x - predator.position.x;
                    int dy = prey.position.y - predator.position.y;
                    
                    // If within capture distance
                    if (std::abs(dx) <= 1 && std::abs(dy) <= 1) {
                        // Calculate dynamic evasion chance based on fear
                        float dynamicEvasionChance = prey.evasionChance + (prey.currentFear / prey.maxFear) * 0.15f; // Max 15% bonus from fear
                        dynamicEvasionChance = std::min(dynamicEvasionChance, 0.9f); // Cap evasion at 90%

                        // Check for evasion
                        float evasion_roll = std::uniform_real_distribution<float>(0.0f, 1.0f)(gen);
                        
                        if (evasion_roll <= dynamicEvasionChance) {
                            // Evasion successful! Stun the predator
                            predator.isStunned = true;
                            predator.stunDuration = 2; // Stun for 2 frames
                            predator.currentState = Sprite::AIState::STUNNED;
                            
                            // Calculate escape position - move prey in opposite direction
                            int escape_dx = (dx == 0) ? (std::uniform_int_distribution<int>(0, 1)(gen) * 2 - 1) : ((dx > 0) ? 1 : -1);
                            int escape_dy = (dy == 0) ? (std::uniform_int_distribution<int>(0, 1)(gen) * 2 - 1) : ((dy > 0) ? 1 : -1);
                            
                            // Move prey 2-3 spaces away to escape
                            int escape_distance = std::uniform_int_distribution<int>(2, 3)(gen);
                            Vec2D escape_pos = {
                                prey.position.x + escape_dx * escape_distance,
                                prey.position.y + escape_dy * escape_distance
                            };
                            
                            // Ensure escape position is within bounds and walkable
                            escape_pos.x = std::max(0, std::min(escape_pos.x, world.width - 1));
                            escape_pos.y = std::max(0, std::min(escape_pos.y, world.height - 1));
                            
                            if (world.is_walkable(escape_pos)) {
                                // Update prey position to escape position
                                prey_sprites[i].position = escape_pos;
                            }
                            
                            // Store message about evasion for display
                            evasion_messages.push_back({prey.position, "Prey escaped from Predator " + std::to_string(
                                std::distance(predators.begin(), std::find_if(predators.begin(), predators.end(), 
                                    [&predator](const Sprite& p) { return &p == &predator; })) + 1)});
                        } else {
                            // No evasion - prey is captured
                            indices_to_remove.push_back(i);
                            break; // This prey is captured, no need to check other predators
                        }
                    }
                }
            }
            
            // Remove captured prey in reverse order to maintain valid indices
            std::sort(indices_to_remove.begin(), indices_to_remove.end(), std::greater<size_t>());
            for (size_t index : indices_to_remove) {
                if (index < prey_sprites.size()) { // Safety check
                    prey_sprites.erase(prey_sprites.begin() + index);
                }
            }
            
            // Check if any prey were captured or evaded
            size_t captures = initial_prey_count - prey_sprites.size();
            if (captures > 0 || !evasion_messages.empty()) {
                // Render to show the capture/evasion
                Renderer::render_to_console(predators, prey_sprites, world, current_step, MAX_STEPS, previous_display_rows, first_frame);
                
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
                    break; // Exit early if all prey captured
                }
            }
        }
        
        // Exit early if all prey captured
        if (prey_sprites.empty()) {
            break;
        }
        
        // Update prey after predators have moved
        for (auto& prey_sprite : prey_sprites) {
            AIController::update_sprite_ai(prey_sprite, predators, prey_sprites, world);
        }
        
        // Check for final captures after prey have moved
        bool final_capture_occurred = false;
        std::vector<std::pair<Vec2D, std::string>> final_evasion_messages;
        
        for (auto& predator : predators) {
            // Skip if predator is stunned
            if (predator.isStunned) continue;
            
            std::vector<size_t> indices_to_remove;
            
            for (size_t i = 0; i < prey_sprites.size(); ++i) {
                const auto& prey = prey_sprites[i];
                
                // Check capture distance
                int dx = prey.position.x - predator.position.x;
                int dy = prey.position.y - predator.position.y;
                
                if (std::abs(dx) <= 1 && std::abs(dy) <= 1) {
                    // Calculate dynamic evasion chance based on fear
                    float dynamicEvasionChance = prey.evasionChance + (prey.currentFear / prey.maxFear) * 0.15f; // Max 15% bonus from fear
                    dynamicEvasionChance = std::min(dynamicEvasionChance, 0.9f); // Cap evasion at 90%

                    // Check for evasion
                    float evasion_roll = std::uniform_real_distribution<float>(0.0f, 1.0f)(gen);
                    
                    if (evasion_roll <= dynamicEvasionChance) {
                        // Evasion successful! Stun the predator
                        predator.isStunned = true;
                        predator.stunDuration = 2; // Stun for 2 frames
                        predator.currentState = Sprite::AIState::STUNNED;
                        
                        // Calculate escape position - move prey in opposite direction
                        int escape_dx = (dx == 0) ? (std::uniform_int_distribution<int>(0, 1)(gen) * 2 - 1) : ((dx > 0) ? 1 : -1);
                        int escape_dy = (dy == 0) ? (std::uniform_int_distribution<int>(0, 1)(gen) * 2 - 1) : ((dy > 0) ? 1 : -1);
                        
                        // Move prey 2-3 spaces away to escape
                        int escape_distance = std::uniform_int_distribution<int>(2, 3)(gen);
                        Vec2D escape_pos = {
                            prey.position.x + escape_dx * escape_distance,
                            prey.position.y + escape_dy * escape_distance
                        };
                        
                        // Ensure escape position is within bounds and walkable
                        escape_pos.x = std::max(0, std::min(escape_pos.x, world.width - 1));
                        escape_pos.y = std::max(0, std::min(escape_pos.y, world.height - 1));
                        
                        if (world.is_walkable(escape_pos)) {
                            // Update prey position to escape position
                            prey_sprites[i].position = escape_pos;
                        }
                        
                        // Store message about evasion for display
                        final_evasion_messages.push_back({prey.position, "Prey escaped from Predator " + std::to_string(
                            std::distance(predators.begin(), std::find_if(predators.begin(), predators.end(), 
                                [&predator](const Sprite& p) { return &p == &predator; })) + 1)});
                    } else {
                        // No evasion - prey is captured
                        indices_to_remove.push_back(i);
                        final_capture_occurred = true;
                    }
                }
            }
            
            // Remove captured prey in reverse order to maintain valid indices
            std::sort(indices_to_remove.begin(), indices_to_remove.end(), std::greater<size_t>());
            for (size_t index : indices_to_remove) {
                if (index < prey_sprites.size()) { // Safety check
                    prey_sprites.erase(prey_sprites.begin() + index);
                }
            }
        }
        
        if (final_capture_occurred || !final_evasion_messages.empty()) {
            // Render to show the capture/evasion
            Renderer::render_to_console(predators, prey_sprites, world, current_step, MAX_STEPS, previous_display_rows, first_frame);
            std::cout << "\033[" << world.height + 3 << ";1H"; // Position cursor below status line
            
            if (final_capture_occurred) {
                std::cout << "Prey captured after prey movement!\033[K" << std::endl;
            }
            
            if (!final_evasion_messages.empty()) {
                for (const auto& msg : final_evasion_messages) {
                    std::cout << msg.second << " at position (" << msg.first.x << "," << msg.first.y << ")\033[K" << std::endl;
                }
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(FRAME_DELAY_MS)); // Brief pause to show capture
        }
        
        if (prey_sprites.empty()) {
            // Render one last time to show capture, then break
            Renderer::render_to_console(predators, prey_sprites, world, current_step, MAX_STEPS, previous_display_rows, first_frame);
            std::cout << ANSI_MOVE_CURSOR_TO_START << ANSI_CLEAR_SCREEN_BELOW;
            std::cout << "All prey captured!" << std::endl;
            break;
        }

        // Render the current state
        Renderer::render_to_console(predators, prey_sprites, world, current_step, MAX_STEPS, previous_display_rows, first_frame);

        // Delay for visualization
        std::this_thread::sleep_for(std::chrono::milliseconds(FRAME_DELAY_MS));
        current_step++;
    }

    // Show cursor again and clean up
    std::cout << "\033[?25h" << std::flush;
    
    if (prey_sprites.empty()) {
        std::cout << "Simulation ended: All prey captured after " << current_step << " steps." << std::endl;
    } else {
        std::cout << "Simulation ended: MAX_STEPS reached after " << current_step << " steps. " << prey_sprites.size() << " prey remaining." << std::endl;
    }
    return 0;
} 