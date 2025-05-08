#ifndef STATUS_DISPLAY_H
#define STATUS_DISPLAY_H

#include <vector>
#include "Sprite.h"

namespace StatusDisplay {
    // Display simulation status information (step counter, sprites info)
    void display_simulation_status(
        const std::vector<Sprite>& predators,
        const std::vector<Sprite>& prey_sprites,
        int current_step,
        int max_steps
    );
    
    // Display detailed predator information
    void display_predator_status(
        const std::vector<Sprite>& predators
    );
}

#endif // STATUS_DISPLAY_H 