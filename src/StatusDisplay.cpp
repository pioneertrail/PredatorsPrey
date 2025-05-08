#include "StatusDisplay.h"
#include <iostream>
#include <iomanip>

// Predator colors for differentiation
namespace ColorExt {
    const std::string BRIGHT_RED = "\033[91m";
    const std::string BRIGHT_MAGENTA = "\033[95m";
    const std::string BRIGHT_CYAN = "\033[96m";
}

namespace StatusDisplay {

void display_simulation_status(
    const std::vector<Sprite>& predators,
    const std::vector<Sprite>& prey_sprites,
    int current_step,
    int max_steps
) {
    // Reset color and print status information
    std::cout << Color::RESET;
    
    // Calculate average fear and stamina
    float total_fear = 0.0f;
    for (const auto& prey : prey_sprites) {
        total_fear += prey.currentFear;
    }
    float avg_fear = prey_sprites.empty() ? 0.0f : total_fear / prey_sprites.size();

    int total_stamina = 0;
    for (const auto& predator : predators) {
        total_stamina += predator.currentStamina;
    }
    float avg_stamina = predators.empty() ? 0.0f : static_cast<float>(total_stamina) / predators.size();

    // Status line
    std::cout << "Step: " << std::setw(4) << current_step << "/" << max_steps
              << " | Predators: " << predators.size()
              << " (Avg Stam: " << std::fixed << std::setprecision(1) << avg_stamina << ")"
              << " | Prey: " << prey_sprites.size()
              << " (Avg Fear: " << std::fixed << std::setprecision(1) << avg_fear << ")"
              << std::endl;

    // Predator state counts
    int resting_count = 0;
    int stunned_count = 0;
    for (const auto& predator : predators) {
        if (predator.currentState == Sprite::AIState::RESTING) resting_count++;
        if (predator.currentState == Sprite::AIState::STUNNED) stunned_count++;
    }
    std::cout << "Predator States: Resting: " << resting_count 
              << ", Stunned: " << stunned_count << std::endl;
}

void display_predator_status(
    const std::vector<Sprite>& predators
) {
    // Predator colors for differentiation
    const std::string predator_colors[] = {
        Color::RED, 
        ColorExt::BRIGHT_MAGENTA,
        ColorExt::BRIGHT_CYAN
    };
    
    // Predator status line - limit to first 3 predators
    for (size_t i = 0; i < predators.size() && i < 3; ++i) {
        const auto& p = predators[i];
        std::string pred_state_str;
        std::string color = (i < 3) ? predator_colors[i] : Color::RED;
        
        // Convert state enum to readable string for HUD
        switch (p.currentState) {
            case Sprite::AIState::SEEKING: pred_state_str = "SEEKING"; break;
            case Sprite::AIState::SEARCHING_LKP: pred_state_str = "SEARCH_LKP"; break;
            case Sprite::AIState::WANDERING: pred_state_str = "WANDERING"; break;
            case Sprite::AIState::RESTING: pred_state_str = "RESTING"; break;
            case Sprite::AIState::STUNNED: pred_state_str = "STUNNED"; break;
            default: pred_state_str = "WANDERING"; break;
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

} // namespace StatusDisplay 