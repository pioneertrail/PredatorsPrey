# Tiny AI Sprite Console Simulator

A simple C++ application that simulates two AI-controlled sprites (a predator and a prey)
and displays their state in the console.

## Prerequisites

- C++17 Compiler
- (For Windows) A way to run batch scripts (`.bat` files) and a Visual Studio C++ toolchain.

## Building & Running (Windows using `compile.bat`)

The provided `compile.bat` script is configured to use the Visual Studio C++ compiler.

1.  Ensure you have the Visual Studio Build Tools or Visual Studio IDE installed with C++ support.
    The script attempts to use `vcvars64.bat` which should be part of this installation.
2.  Open a command prompt (like `cmd.exe` or PowerShell).
3.  Navigate to the project directory.
4.  Run the compile script:
    ```bash
    compile.bat
    ```
5.  If successful, this will compile `src/main.cpp` and create `TinyRenderer.exe` in the project root.
    It will then automatically run `TinyRenderer.exe`.

The simulation will run for a fixed number of steps, printing the state of the predator (P) and prey (Y) to the console at each step.

## Building (Using CMake - Optional)

While `compile.bat` is provided for direct compilation on Windows, CMake files are also present for cross-platform building or IDE integration.

1.  Ensure you have a C++17 compiler and CMake installed.
2.  Create a build directory:
    ```bash
    mkdir build
    cd build
    ```
3.  Configure CMake and build:
    ```bash
    cmake ..
    cmake --build . # Or use your specific build system like make, ninja, etc.
    ```
4.  Run the executable from the build directory (e.g., `build/TinyRenderer` or `build/Debug/TinyRenderer.exe`).

## How it Works

-   **Sprites**: The predator (`P`) and prey (`Y`) are represented by characters.
-   **AI**: 
    -   The predator moves directly towards the prey.
    -   The prey moves directly away from the predator.
-   **Simulation**: Positions are updated in discrete steps.
-   **Display**: The "world" is drawn as a grid of characters in the console. Sprite positions are updated each step.
