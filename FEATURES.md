# Features - TinyRenderer Console Simulator

This document lists the core features implemented in the simulation.

## Core Simulation

*   **Console Rendering:** Uses direct console manipulation via `std::cout` with ANSI escape codes for cursor movement, clearing lines, and setting text colors to render the simulation grid, sprites, and status information.
*   **Differential Rendering:** Only updates console rows/lines that have changed content since the last frame. This is done to minimize console flicker, reduce the amount of data sent to the terminal, and improve overall visual stability and perceived performance.
*   **Game Loop:** Runs for a fixed number of steps (configurable via `MAX_STEPS` in `compile.bat`) with a configurable delay between steps.
*   **World Generation & Structure:** 
    *   Fixed-size grid defined by `World::width` and `World::height`.
    *   Obstacle Placement: Includes border walls, randomly placed blocks, and specific cleared areas (e.g., corners, center of the map). The `initialize_obstacles` function also performs a cleanup pass to remove isolated obstacles and attempt to break up obvious dead-ends.
    *   Safe Zone Placement: A few safe zones are currently hardcoded in `initialize_obstacles` (e.g., `{10, 10}`, `{width - 10, height - 10}`).
*   **Sprites:** Represents predator ('P') and prey ('Y') with position, color, and basic properties.

## AI - General

*   **Finite State Machine (FSM):** Both predator and prey use states (`WANDERING`, `SEEKING`, `FLEEING`, `SEARCHING_LKP`) to manage behavior.
*   **Biased Random Walk:** Sprites tend to continue in their current direction while wandering, preventing overly erratic movement.

## AI - Predator

*   **Seeking Behavior:** Actively pursues the prey when within vision radius.
*   **A* Pathfinding:** Uses the A* algorithm to find the shortest path to the prey, navigating around obstacles.
*   **Path Caching/Replanning:** Follows a calculated path for a few steps (`REPLAN_PATH_INTERVAL`) before recalculating to improve efficiency.
*   **Last Known Position (LKP):** If the prey moves out of sight, the predator will move towards the prey's last known position (`SEARCHING_LKP` state) before reverting to wandering.
*   **Smarter Wandering ("Patrolling"):** Avoids immediately revisiting the last few cells (`WANDER_TRAIL_LENGTH`) it occupied while wandering, encouraging broader exploration.
*   **Resting State:** Enters a `RESTING` state when stamina is low to regenerate it more quickly. Does not move while resting.
*   **Stamina System:** Predators have a stamina pool that depletes when performing fast moves (sprinting during `SEEKING` or `WANDERING`). Stamina regenerates slowly over time, or faster when in the `RESTING` state.
*   **Stuck Detection & Unsticking:** Implements a multi-layered system to detect if a predator is stuck (stationary or oscillating between positions). If stuck for a threshold number of turns, it attempts to unstick by: 
    1.  Switching to `WANDERING` and clearing its path.
    2.  Trying random adjacent moves.
    3.  If still stuck, trying random larger jumps.
    4.  As a last resort, searching a wider area for any walkable tile.

## AI - Prey

*   **Fleeing Behavior:** Actively moves away from the predator when it's within awareness radius and has line of sight.
*   **Line of Sight (LoS):** Uses Bresenham's line algorithm to determine if the predator is directly visible; only flees if the predator is close *and* visible.
*   **Prioritized Direct Escape:** When fleeing, first attempts to move directly away (cardinal or diagonal) from the predator if the path is clear and increases distance.
*   **Broad Escape Search:** If direct escape fails, searches all adjacent cells for the best move to maximize distance from the predator.
*   **Cornered Behavior:** If no escape route improves its situation, the prey currently stops moving (implicitly "cornered").
*   **Safe Zones:** Can identify and pathfind to designated "safe zones" on the map when fleeing. These zones offer a way to reduce fear more rapidly.
*   **Fear Mechanics:** Accumulates fear when a predator is close and has line of sight. Fear decreases over time, and this decay is accelerated when the prey is inside a safe zone. High fear can influence behavior (e.g., decision to seek a safe zone).

## Build System

*   **Direct MSVC Compilation:** Uses a simple Windows batch script (`compile.bat`) to invoke `cl.exe` directly, setting necessary include paths and compiler flags.
*   **No External Dependencies (beyond C++17/Standard Library):** Relies only on standard C++ libraries and console capabilities. 

## Debugging

*   **Toggleable Path Display:** Pressing 'p' during the simulation toggles the visualization of the `currentPath` for all sprites. Paths are rendered as cyan '.' characters on empty floor or safe zone tiles. 