#ifndef SIMULATION_SETUP_H
#define SIMULATION_SETUP_H

#include <vector>
#include <random>
#include "Sprite.h"
#include "World.h"

// External reference to the global random generator
extern std::mt19937 gen;

namespace SimulationSetup {
    // Constants for simulation
    const int NUM_PREDATORS = 3;
    const int NUM_PREY = 6;

    // Initialize the predators in the world
    std::vector<Sprite> initialize_predators();
    
    // Initialize the prey in the world
    std::vector<Sprite> initialize_prey(const World& world);
    
    // Get the maximum number of steps from environment variable
    int get_max_steps();
}

#endif // SIMULATION_SETUP_H 