#include <iostream>
#include <vector>
#include <string>
#include <chrono> // For basic timing if needed for delays
#include <thread> // For std::this_thread::sleep_for
#include <cmath> // For std::abs for distance checking
#include <random> // For prey's random movement
#include <algorithm> // For std::shuffle, std::min, std::max
#include <iomanip> // For std::setw, std::left if needed for formatting status
#include <unordered_set> // For efficient obstacle lookup
#include <queue> // Added for std::priority_queue (used by Pathfinding.h indirectly via AStarNode comparison)

#include "Vec2D.h"       // Added
#include "Sprite.h"
#include "Pathfinding.h" // Added

const int WORLD_WIDTH = 80;
const int WORLD_HEIGHT = 20;
const int SPRITE_WIDTH = 1; // Character based width
const int SPRITE_HEIGHT = 1; // Character based height

// AI Parameters
const int PREDATOR_VISION_RADIUS = 40; // How far predator can "see" prey to enter SEEKING state
const int PREY_AWARENESS_RADIUS = 7;  // How close predator needs to be for prey to enter FLEEING state
const int MAX_STEPS_IN_DIRECTION = 4; // New constant for biased random walk
const int REPLAN_PATH_INTERVAL = 3; // Predator replans path every X turns if still seeking
const int WANDER_TRAIL_LENGTH = 5; // Max length of the recent wander trail for predator

// For random number generation
static std::random_device rd;
static std::mt19937 gen(rd());

// Previous frame state for differential rendering (grid part only)
static std::vector<std::string> previous_display_rows;
static bool first_frame = true;

// Obstacle Data
static std::unordered_set<Vec2D> obstacles;
const char OBSTACLE_CHAR = '#';
const std::string OBSTACLE_COLOR = Color::WHITE; // Or Color::BLUE, etc.

// Check if a position is within bounds AND not an obstacle
bool is_walkable(int r, int c) {
    if (r < 0 || r >= WORLD_HEIGHT || c < 0 || c >= WORLD_WIDTH) {
        return false; // Out of bounds
    }
    if (obstacles.count({c, r})) { // Check if position (x,y) is in the obstacle set
        return false; // Obstacle present
    }
    return true;
}

void move_randomly(Sprite& s) {
    Vec2D potential_pos = s.position;
    bool chose_new_direction = false;

    // Try to continue in the last direction (biased random walk part)
    if (s.stepsInCurrentDirection < MAX_STEPS_IN_DIRECTION && (s.lastMoveDirection.x != 0 || s.lastMoveDirection.y != 0)) {
        Vec2D continued_pos = {s.position.x + s.lastMoveDirection.x * s.speed,
                               s.position.y + s.lastMoveDirection.y * s.speed};
        if (is_walkable(continued_pos.y, continued_pos.x)) {
            potential_pos = continued_pos;
            s.stepsInCurrentDirection++;
        } else {
            chose_new_direction = true;
        }
    } else {
        chose_new_direction = true;
    }

    if (chose_new_direction) {
        std::vector<Vec2D> move_options_base = {{0,0}, {0,1}, {0,-1}, {1,0}, {-1,0}};
        std::vector<Vec2D> valid_move_choices;

        // Consider all base options first
        for (const auto& offset : move_options_base) {
            Vec2D test_pos = {s.position.x + offset.x * s.speed, s.position.y + offset.y * s.speed};
            if (is_walkable(test_pos.y, test_pos.x)) {
                // Predator-specific: try to avoid recent trail if other options exist
                if (s.type == Sprite::Type::PREDATOR) {
                    bool on_trail = false;
                    for (const auto& trail_pos : s.recentWanderTrail) {
                        if (trail_pos == test_pos) {
                            on_trail = true;
                            break;
                        }
                    }
                    if (!on_trail) {
                        valid_move_choices.push_back(offset);
                    }
                } else { // Prey uses simpler random logic
                    valid_move_choices.push_back(offset);
                }
            }
        }

        // If predator filtering left no options (e.g., all walkable are on trail), fallback to using all walkable options
        if (s.type == Sprite::Type::PREDATOR && valid_move_choices.empty()) {
             for (const auto& offset : move_options_base) {
                Vec2D test_pos = {s.position.x + offset.x * s.speed, s.position.y + offset.y * s.speed};
                if (is_walkable(test_pos.y, test_pos.x)) {
                    valid_move_choices.push_back(offset);
                }
            }
        }
        
        // Prevent immediate 180 turn if multiple choices and last move direction exists
        if (valid_move_choices.size() > 1 && (s.lastMoveDirection.x != 0 || s.lastMoveDirection.y != 0)){
            Vec2D opposite_dir = {-s.lastMoveDirection.x, -s.lastMoveDirection.y};
            valid_move_choices.erase(
                std::remove_if(valid_move_choices.begin(), valid_move_choices.end(),
                               [&](const Vec2D& choice){ return choice == opposite_dir; }),
                valid_move_choices.end());
        }

        if (valid_move_choices.empty()) { // If no valid moves at all (e.g. walled in)
            valid_move_choices.push_back({0,0}); // Default to staying still
        }

        Vec2D move_offset = valid_move_choices[std::uniform_int_distribution<>(0, (int)valid_move_choices.size() - 1)(gen)];
        
        Vec2D new_potential_pos = {s.position.x + move_offset.x * s.speed, 
                                   s.position.y + move_offset.y * s.speed};
        // is_walkable already checked for options pushed to valid_move_choices, but new_potential_pos must be based on chosen offset.
        // The selected move_offset should result in a walkable new_potential_pos.

        potential_pos = new_potential_pos; // Already verified as walkable when added to valid_move_choices
        s.lastMoveDirection = move_offset;
        s.stepsInCurrentDirection = (move_offset.x == 0 && move_offset.y == 0) ? MAX_STEPS_IN_DIRECTION : 1;
        
        // Update predator's wander trail
        if (s.type == Sprite::Type::PREDATOR && (move_offset.x != 0 || move_offset.y != 0)) { // Only add if moved
            // Add to front, remove from back if over size, avoid duplicates immediately
            if (s.recentWanderTrail.empty() || s.recentWanderTrail.front() != potential_pos) {
                s.recentWanderTrail.insert(s.recentWanderTrail.begin(), potential_pos);
                if (s.recentWanderTrail.size() > WANDER_TRAIL_LENGTH) {
                    s.recentWanderTrail.pop_back();
                }
            }
        }

    } // else (continued in same direction, potential_pos is already set)

    s.position = potential_pos;
}

void update_ai(Sprite& predator, Sprite& prey) {
    // --- Predator Logic ---
    int dist_to_prey_for_pred_logic = manhattan_distance(predator.position, prey.position);
    bool prey_in_pred_sight = (dist_to_prey_for_pred_logic <= PREDATOR_VISION_RADIUS);

    // Predator State Transitions
    if (predator.currentState == Sprite::AIState::WANDERING) {
        if (prey_in_pred_sight) {
            predator.currentState = Sprite::AIState::SEEKING;
            predator.lastKnownPreyPosition = prey.position;
            predator.currentPath.clear(); // Clear any old path when starting to seek
            predator.turnsSincePathReplan = REPLAN_PATH_INTERVAL; // Force immediate plan
        }
        // When predator *enters* wandering or is wandering, its trail management happens in move_randomly.
        // If it *was not* wandering and now *is*, we might want to clear trail.
        // Let's add clearing trail when transitioning TO wandering.
    } else if (predator.currentState == Sprite::AIState::SEEKING) {
        if (prey_in_pred_sight) {
            predator.lastKnownPreyPosition = prey.position;
            predator.turnsSincePathReplan++;
        } else {
            predator.currentState = Sprite::AIState::SEARCHING_LKP;
            predator.currentPath.clear(); // Clear path when transitioning to searching LKP
            predator.turnsSincePathReplan = REPLAN_PATH_INTERVAL; // Force immediate plan to LKP
        }
    } else if (predator.currentState == Sprite::AIState::SEARCHING_LKP) {
        if (prey_in_pred_sight) {
            predator.currentState = Sprite::AIState::SEEKING;
            predator.lastKnownPreyPosition = prey.position;
            predator.currentPath.clear();
            predator.turnsSincePathReplan = REPLAN_PATH_INTERVAL;
        } else if (predator.position == predator.lastKnownPreyPosition || dist_to_prey_for_pred_logic <= 1) {
            predator.currentState = Sprite::AIState::WANDERING;
            predator.currentPath.clear();
            predator.recentWanderTrail.clear(); // Clear trail when starting to wander from failed LKP search
        }
        // If searching LKP, we might also increment turnsSincePathReplan or have a similar timer for it.
        // For now, SEARCHING_LKP will always try to path to the LKP if not already there.
    }

    // Predator Action
    Vec2D predator_target_pos = predator.position;
    if (predator.currentState == Sprite::AIState::SEEKING) {
        if (predator.currentPath.empty() || predator.turnsSincePathReplan >= REPLAN_PATH_INTERVAL) {
            if (prey_in_pred_sight) { // Only plan if prey is actually in sight
                predator.currentPath = find_path(predator.position, prey.position, obstacles, WORLD_WIDTH, WORLD_HEIGHT);
                predator.pathFollowStep = 0;
                predator.turnsSincePathReplan = 0;
            } else {
                 // Prey not in sight but still in SEEKING somehow (should be SEARCHING_LKP or WANDERING)
                 // This case might indicate a need to transition state more aggressively if sight is lost
                 predator.currentPath.clear(); // Ensure no old path is followed
                 predator.currentState = Sprite::AIState::SEARCHING_LKP; // Re-evaluate state
            }
        }

        if (!predator.currentPath.empty() && predator.pathFollowStep < predator.currentPath.size()) {
            Vec2D next_step = predator.currentPath[predator.pathFollowStep];
            if (is_walkable(next_step.y, next_step.x)) { // Check if the next step in path is still walkable
                predator_target_pos = next_step;
                predator.pathFollowStep++;
                 if (predator.pathFollowStep >= predator.currentPath.size()) { // Reached end of current path segment
                    predator.currentPath.clear(); // Clear to force replan or state change
                    predator.turnsSincePathReplan = REPLAN_PATH_INTERVAL; // Ensure replan if still seeking
                }
            } else { // Path blocked
                predator.currentPath.clear(); // Clear path, force replan next turn
                predator.turnsSincePathReplan = REPLAN_PATH_INTERVAL;
            }
        } else if (!predator.currentPath.empty() && predator.pathFollowStep >= predator.currentPath.size()){
             predator.currentPath.clear(); // Path exhausted, force replan
             predator.turnsSincePathReplan = REPLAN_PATH_INTERVAL;
        } else if (predator.currentPath.empty() && prey_in_pred_sight) {
            // No path found, but prey is in sight. Try to move directly or wait for replan.
            // This can happen if prey is 1 step away and A* returns size 1 or 0.
            // Or if A* fails for some reason. For now, predator might pause.
            // A direct move attempt could be added here if path is empty but target is adjacent.
        }

    } else if (predator.currentState == Sprite::AIState::SEARCHING_LKP) {
        // Path to LKP is recalculated each step in SEARCHING_LKP if no valid path part exists
        // or if forced by turnsSincePathReplan for LKP (if we add that feature)
        if (predator.currentPath.empty() || predator.turnsSincePathReplan >= REPLAN_PATH_INTERVAL) { // Reuse replan logic for LKP
            predator.currentPath = find_path(predator.position, predator.lastKnownPreyPosition, obstacles, WORLD_WIDTH, WORLD_HEIGHT);
            predator.pathFollowStep = 0;
            predator.turnsSincePathReplan = 0; // Reset LKP path timer
        }

        if (!predator.currentPath.empty() && predator.pathFollowStep < predator.currentPath.size()) {
            Vec2D next_step = predator.currentPath[predator.pathFollowStep];
            if (is_walkable(next_step.y, next_step.x)){
                predator_target_pos = next_step;
                predator.pathFollowStep++;
                 if (predator.pathFollowStep >= predator.currentPath.size()) { 
                    predator.currentPath.clear();
                     // Reached LKP, state transition logic above should handle it.
                }
            } else {
                predator.currentPath.clear();
                predator.turnsSincePathReplan = REPLAN_PATH_INTERVAL; // Force replan to LKP if path blocked
            }
        } else if (predator.position != predator.lastKnownPreyPosition) {
             // No path to LKP, or path exhausted and not at LKP. Transition to WANDERING.
             predator.currentState = Sprite::AIState::WANDERING;
             predator.currentPath.clear();
        } // If at LKP, state transition handles it.	

    } else { // WANDERING (Predator)
        move_randomly(predator);
        predator_target_pos = predator.position;
        predator.currentPath.clear(); // Ensure no path is stored while wandering
        // If it just transitioned to WANDERING this turn, clear trail.
        // This needs to be handled in the state transition logic more cleanly.
    }
    predator.position = predator_target_pos;
    predator.position.x = std::max(0, std::min(predator.position.x, WORLD_WIDTH - 1));
    predator.position.y = std::max(0, std::min(predator.position.y, WORLD_HEIGHT - 1));

    if (predator.position == prey.position) return;

    // --- Prey Logic (remains the same) ---
    int dist_to_predator_for_prey_logic = manhattan_distance(predator.position, prey.position); // Use updated predator position
    bool predator_has_los_to_prey = has_line_of_sight(prey.position, predator.position, obstacles, WORLD_WIDTH, WORLD_HEIGHT);
    bool predator_in_awareness_radius = (dist_to_predator_for_prey_logic <= PREY_AWARENESS_RADIUS);

    if (prey.currentState == Sprite::AIState::WANDERING) {
        if (predator_in_awareness_radius && predator_has_los_to_prey) {
            prey.currentState = Sprite::AIState::FLEEING;
        }
    } else if (prey.currentState == Sprite::AIState::FLEEING) {
        // Stop fleeing if predator is no longer a direct threat (too far OR no LoS)
        if (!predator_in_awareness_radius || !predator_has_los_to_prey) {
            prey.currentState = Sprite::AIState::WANDERING;
        }
    }
    
    Vec2D prey_target_pos = prey.position;
    if (prey.currentState == Sprite::AIState::FLEEING) {
        bool direct_escape_taken = false;
        int dx = prey.position.x - predator.position.x;
        int dy = prey.position.y - predator.position.y;
        Vec2D primary_escape_dir = {0,0};
        if (dx > 0) primary_escape_dir.x = 1; else if (dx < 0) primary_escape_dir.x = -1;
        if (dy > 0) primary_escape_dir.y = 1; else if (dy < 0) primary_escape_dir.y = -1;
        Vec2D potential_direct_pos = {prey.position.x + primary_escape_dir.x * prey.speed, 
                                      prey.position.y + primary_escape_dir.y * prey.speed};
        if (primary_escape_dir.x != 0 || primary_escape_dir.y != 0) {
            if (is_walkable(potential_direct_pos.y, potential_direct_pos.x)) {
                int dist_after_direct_move = manhattan_distance(predator.position, potential_direct_pos);
                if (dist_after_direct_move > dist_to_predator_for_prey_logic) {
                    prey_target_pos = potential_direct_pos;
                    direct_escape_taken = true;
                }
            }
        }
        if (!direct_escape_taken && (primary_escape_dir.x != 0 || primary_escape_dir.y != 0) ) {
            Vec2D cardinal_options[2] = {{0,0}, {0,0}};
            int num_cardinal_options = 0;
            if (primary_escape_dir.x != 0) cardinal_options[num_cardinal_options++] = {primary_escape_dir.x, 0};
            if (primary_escape_dir.y != 0) cardinal_options[num_cardinal_options++] = {0, primary_escape_dir.y};
            std::shuffle(std::begin(cardinal_options), std::begin(cardinal_options) + num_cardinal_options, gen);
            for (int i = 0; i < num_cardinal_options; ++i) {
                Vec2D card_escape_dir = cardinal_options[i];
                potential_direct_pos = {prey.position.x + card_escape_dir.x * prey.speed,
                                        prey.position.y + card_escape_dir.y * prey.speed};
                if (is_walkable(potential_direct_pos.y, potential_direct_pos.x)) {
                    int dist_after_card_move = manhattan_distance(predator.position, potential_direct_pos);
                    if (dist_after_card_move > dist_to_predator_for_prey_logic) {
                        prey_target_pos = potential_direct_pos;
                        direct_escape_taken = true;
                        break;
                    }
                }
            }
        }
        if (!direct_escape_taken) {
            Vec2D best_evade_move_offset = {0,0};
            int initial_dist_to_predator = dist_to_predator_for_prey_logic; // Store initial distance for cornered check
            int max_dist_found = initial_dist_to_predator; 
            bool found_valid_evade = false;
            bool made_progress_in_evade = false; // Track if any move actually increased distance

            std::vector<Vec2D> evade_options = {{0,1}, {0,-1}, {1,0}, {-1,0}, {1,1}, {1,-1}, {-1,1}, {-1,-1}, {0,0}};
            std::shuffle(evade_options.begin(), evade_options.end(), gen);

            for(const auto& move_offset : evade_options) {
                Vec2D potential_pos = {prey.position.x + move_offset.x * prey.speed, 
                                       prey.position.y + move_offset.y * prey.speed};
                
                if (is_walkable(potential_pos.y, potential_pos.x)) {
                    int new_dist = manhattan_distance(predator.position, potential_pos);
                    if (!found_valid_evade || new_dist > max_dist_found) {
                        max_dist_found = new_dist;
                        best_evade_move_offset = move_offset;
                        found_valid_evade = true;
                        if (new_dist > initial_dist_to_predator) made_progress_in_evade = true;
                    } else if (new_dist == max_dist_found && (best_evade_move_offset.x == 0 && best_evade_move_offset.y == 0) && (move_offset.x !=0 || move_offset.y !=0)) {
                        best_evade_move_offset = move_offset;
                        found_valid_evade = true;
                        // made_progress_in_evade remains true if it was already true, or false if this move doesn't improve distance
                    }
                }
            }

            if (found_valid_evade) {
                 prey_target_pos = {prey.position.x + best_evade_move_offset.x * prey.speed,
                                   prey.position.y + best_evade_move_offset.y * prey.speed};
            } else {
                // No valid walkable move found at all by broader search (very unlikely unless map is tiny/full)
                prey_target_pos = prey.position; // Stay still
            }
            
            // Cornered Check: If no direct escape AND broader search didn't improve distance or find a move
            if (!direct_escape_taken && !made_progress_in_evade && found_valid_evade && best_evade_move_offset.x == 0 && best_evade_move_offset.y == 0) {
                // Prey is considered cornered if the best it could do was stay still and that didn't improve things.
                // For now, cornered behavior is just to stay still, which is already handled if prey_target_pos isn't changed.
                // We could add a special flag or different character for rendering if truly cornered.
                // Example: prey.displayChar = 'X'; // If we want to show it as cornered
            } else if (!direct_escape_taken && !found_valid_evade){
                // Extremely cornered: No direct escape AND no valid move found in broader search (not even staying still)
                // This implies current position became unwalkable or some other error. Default to current (which might be invalid).
                 prey_target_pos = prey.position;
            }
        }

        // Final walkability check for the chosen prey_target_pos
        if (!is_walkable(prey_target_pos.y, prey_target_pos.x)) {
            prey_target_pos = prey.position; 
        }

    } else { // WANDERING (Prey)
        move_randomly(prey);
        prey_target_pos = prey.position;
    }
    prey.position = prey_target_pos;
    prey.position.x = std::max(0, std::min(prey.position.x, WORLD_WIDTH - 1));
    prey.position.y = std::max(0, std::min(prey.position.y, WORLD_HEIGHT - 1));
}

void render_to_console(const Sprite& predator, const Sprite& prey) {
    std::vector<std::string> current_display_rows(WORLD_HEIGHT);
    std::string obstacle_display = OBSTACLE_COLOR + OBSTACLE_CHAR + Color::RESET;

    for (int r = 0; r < WORLD_HEIGHT; ++r) {
        std::string row_string;
        row_string.reserve(WORLD_WIDTH * 10);
        for (int c = 0; c < WORLD_WIDTH; ++c) {
            Vec2D current_cell_pos = {c, r};
            if (r == predator.position.y && c == predator.position.x) {
                row_string += predator.getDisplayString();
            } else if (r == prey.position.y && c == prey.position.x) {
                row_string += prey.getDisplayString();
            } else if (obstacles.count(current_cell_pos)) {
                row_string += obstacle_display;
            } else {
                row_string += ' ';
            }
        }
        current_display_rows[r] = row_string;
    }

    if (first_frame) {
        std::cout << "\033[2J\033[H" << std::flush;
        for (int r = 0; r < WORLD_HEIGHT; ++r) {
            std::cout << current_display_rows[r] << std::endl;
        }
        first_frame = false;
    } else {
        for (int r = 0; r < WORLD_HEIGHT; ++r) {
            if (previous_display_rows.empty() || r >= previous_display_rows.size() || current_display_rows[r] != previous_display_rows[r]) {
                std::cout << "\033[" << r + 1 << ";1H";
                std::cout << current_display_rows[r] << "\033[K";
            }
        }
         std::cout << std::flush;
    }
    previous_display_rows = current_display_rows;
    std::cout << "\033[" << WORLD_HEIGHT + 1 << ";1H";
    for (int i = 0; i < WORLD_WIDTH; ++i) std::cout << "-";
    std::cout << "\033[K" << std::endl;

    std::string pred_state_str;
    switch (predator.currentState) {
        case Sprite::AIState::SEEKING:
            pred_state_str = "SEEKING"; break;
        case Sprite::AIState::SEARCHING_LKP:
            pred_state_str = "SEARCH_LKP"; break;
        case Sprite::AIState::WANDERING:
        default:
            pred_state_str = "WANDERING"; break;
    }
    std::string prey_state_str = (prey.currentState == Sprite::AIState::FLEEING) ? "FLEEING" : "WANDERING";

    std::string status_line_text = 
        predator.colorCode + "P(" + std::to_string(predator.position.x) + "," + std::to_string(predator.position.y) + ") [" + pred_state_str + "]" + Color::RESET +
        "   " + 
        prey.colorCode + "Y(" + std::to_string(prey.position.x) + "," + std::to_string(prey.position.y) + ") [" + prey_state_str + "]" + Color::RESET;

    std::cout << status_line_text << "\033[K" << std::endl;
}

void initialize_obstacles() {
    obstacles.clear();
    // Add a simple horizontal wall
    for (int x = 10; x < WORLD_WIDTH - 10; ++x) {
        obstacles.insert({x, WORLD_HEIGHT / 2});
    }
    // Add a simple vertical wall
    for (int y = 5; y < WORLD_HEIGHT - 5; ++y) {
        obstacles.insert({WORLD_WIDTH / 2, y});
    }
    // Add some random blocks
    std::uniform_int_distribution<> x_dist(0, WORLD_WIDTH - 1);
    std::uniform_int_distribution<> y_dist(0, WORLD_HEIGHT - 1);
    for (int i = 0; i < 20; ++i) {
        obstacles.insert({x_dist(gen), y_dist(gen)});
    }
}

int main(int argc, char* argv[]) {
    if (first_frame) { // Initialize previous_display_rows if it's the very first run
        previous_display_rows.assign(WORLD_HEIGHT, std::string(WORLD_WIDTH, ' '));
    }
    first_frame = true; // Reset for the rendering logic at the start of this main call

    initialize_obstacles();

    Sprite predator;
    // Ensure predator doesn't spawn inside an obstacle
    do { 
      predator.position = {WORLD_WIDTH / 4, WORLD_HEIGHT / 3};
    } while (obstacles.count(predator.position));
    predator.size = {SPRITE_WIDTH, SPRITE_HEIGHT};
    predator.displayChar = 'P';
    predator.colorCode = Color::RED;
    predator.speed = 1;
    predator.type = Sprite::Type::PREDATOR;
    predator.currentState = Sprite::AIState::WANDERING;

    Sprite prey;
    // Ensure prey doesn't spawn inside an obstacle
    do {
        prey.position = {3 * WORLD_WIDTH / 4, 2 * WORLD_HEIGHT / 3};
    } while (obstacles.count(prey.position) || prey.position == predator.position);
    prey.size = {SPRITE_WIDTH, SPRITE_HEIGHT};
    prey.displayChar = 'Y';
    prey.colorCode = Color::GREEN;
    prey.speed = 1;
    prey.type = Sprite::Type::PREY;
    prey.currentState = Sprite::AIState::WANDERING;

    const int game_duration_steps = 400; // Increased duration
    const int step_delay_ms = 100;

    // Initial clear of the whole console screen before starting the loop
    std::cout << "\033[2J\033[H" << std::flush;

    for (int i = 0; i < game_duration_steps; ++i) {
        update_ai(predator, prey);
        render_to_console(predator, prey);

        if (predator.position == prey.position) {
            std::cout << "\033[" << WORLD_HEIGHT + 3 << ";1H"; // Position for final message
            std::cout << Color::YELLOW << "Predator caught the Prey!" << Color::RESET << "\033[K" << std::endl;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(step_delay_ms));
    }
    
    std::cout << "\033[" << WORLD_HEIGHT + 3 << ";1H"; // Position for final message
    std::cout << "Simulation finished." << "\033[K" << std::endl;

    return 0;
} 