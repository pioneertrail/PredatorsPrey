#include <iostream>
#include <random>
#include <vector>
#include "Sprite.h"
#include "World.h"
#include "SimulationSetup.h"
#include "GameLogic.h"

// Global Random Generator (used by multiple modules)
std::random_device rd;
std::mt19937 gen(rd());

// --- Main Function --- 
int main(int /*argc*/, char* /*argv*/[]) {
    // Initialize the world
    World world;
    world.initialize_obstacles();

    // Get the maximum number of steps
    int max_steps = SimulationSetup::get_max_steps();
    
    // Initialize predators and prey
    std::vector<Sprite> predators = SimulationSetup::initialize_predators();
    std::vector<Sprite> prey_sprites = SimulationSetup::initialize_prey(world);
    
    // Run the simulation
    GameLogic::run_simulation(predators, prey_sprites, world, max_steps);
    
    return 0;
} 