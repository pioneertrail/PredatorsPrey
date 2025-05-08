#ifndef CAPTURE_LOGIC_H
#define CAPTURE_LOGIC_H

#include <vector>
#include <utility>
#include <string>
#include <random>
#include "Sprite.h"
#include "World.h"

// External reference to the global random generator
extern std::mt19937 gen;

namespace CaptureLogic {
    // Check for and process prey captures by predators
    // Returns a pair containing:
    // - Number of prey captured
    // - Vector of evasion messages (position, message)
    std::pair<size_t, std::vector<std::pair<Vec2D, std::string>>> 
    process_captures(std::vector<Sprite>& predators, 
                    std::vector<Sprite>& prey_sprites,
                    const World& world);
    
    // Attempt to capture a specific prey with a specific predator
    // Returns true if prey was captured, false if prey evaded
    bool attempt_capture(Sprite& predator, Sprite& prey, const World& world,
                        std::vector<std::pair<Vec2D, std::string>>& evasion_messages,
                        int predator_index);
    
    // Calculate escape position for prey that successfully evaded capture
    Vec2D calculate_escape_position(const Sprite& prey, const Sprite& predator, const World& world);
}

#endif // CAPTURE_LOGIC_H 