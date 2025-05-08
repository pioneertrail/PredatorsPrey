# TinyRenderer Console Simulator

A C++ application that simulates predator-prey interaction in a console window.
Features AI sprites with pathfinding (A*), line-of-sight checks, finite state machines (Wandering, Seeking, Fleeing, Searching Last Known Position), and basic obstacle avoidance.

## Features

*   Console-based rendering using ANSI escape codes.
*   Predator sprite (P) that seeks the Prey sprite (Y).
*   Prey sprite (Y) that flees the Predator.
*   Basic obstacle map (walls and random blocks).
*   Predator uses A* pathfinding to navigate around obstacles.
*   Prey uses line-of-sight checks and prioritized escape logic.
*   Sprites use finite state machines for different behaviors.
*   Biased random walk for more natural wandering.

(See `FEATURES.md` for more details).

## Prerequisites

*   C++17 Compiler (Tested with MSVC++ from Visual Studio 2022).
*   (For Windows) A way to run batch scripts (`.bat` files) and a Visual Studio C++ toolchain (e.g., Build Tools for Visual Studio or the full IDE).
    *   Note: The `compile.bat` script has a hardcoded path to `vcvars64.bat`. This may need to be adjusted based on your Visual Studio installation directory (e.g., `C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat`).

## Building & Running (Windows using `compile.bat`)

The provided `compile.bat` script is configured to use the Visual Studio C++ compiler (`cl.exe`).

1.  **Ensure Visual Studio Build Tools are installed:** You need the C++ workload installed. The script attempts to automatically find and run `vcvars64.bat` to set up the environment.
2.  **Open a command prompt:** Use Developer Command Prompt for VS, `cmd.exe`, or PowerShell.
3.  **Navigate to the project directory:** `cd path\to\engine`
4.  **Run the compile script:**
    ```bash
    .\compile.bat 
    ```
    (Note: Use `.\` prefix in PowerShell if needed).
5.  If successful, the script will compile the source files (`main.cpp`, `World.cpp`, `AIController.cpp`, `Renderer.cpp`, `Pathfinding.cpp`) and create `TinyRenderer.exe`. It will then automatically run the executable.
    *   The `compile.bat` script sets an environment variable `MAX_STEPS=100` to limit the simulation duration. You can modify this in the script if needed.
6.  The simulation will run in the console window for a fixed number of steps.

## Project Structure

*   `src/`:
    *   `main.cpp`: Main application entry point, game loop management.
    *   `Vec2D.h`: Simple 2D vector struct.
    *   `Sprite.h`: Sprite struct definition (data for predator/prey).
    *   `World.h`, `World.cpp`: World data (dimensions, obstacles) and related functions (`is_walkable`).
    *   `AIController.h`, `AIController.cpp`: AI logic (state machines, movement logic).
    *   `Pathfinding.h`, `Pathfinding.cpp`: A* pathfinding, line-of-sight, and distance utilities.
    *   `Renderer.h`, `Renderer.cpp`: Console rendering logic.
*   `compile.bat`: Windows batch script for compilation using MSVC.
*   `.gitignore`: Specifies files/directories for Git to ignore.
*   `README.md`: This file.
*   `FEATURES.md`: Detailed list of implemented features.
*   `CHANGELOG.md`: History of changes.
