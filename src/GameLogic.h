#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include <vector>
#include <string>
#include "Sprite.h"
#include "World.h"

namespace GameLogic {
    // Core game loop for predator-prey simulation
    // Returns number of steps completed
    int run_simulation(std::vector<Sprite>& predators, 
                       std::vector<Sprite>& prey_sprites,
                       World& world, 
                       int max_steps);
    
    // Process a single simulation step
    // Returns true if simulation should continue, false if it should end
    bool process_simulation_step(std::vector<Sprite>& predators,
                                std::vector<Sprite>& prey_sprites,
                                World& world,
                                int current_step,
                                int max_steps);
    
    // Handle user input during simulation
    // Returns true if any settings were changed
    bool handle_user_input(bool& show_paths);
}

#endif // GAME_LOGIC_H 