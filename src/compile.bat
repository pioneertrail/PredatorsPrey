@echo off
REM Set up Visual Studio Environment
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to set up Visual Studio environment.
    echo Please ensure the path to vcvars64.bat is correct.
    exit /b 1
)
echo Visual Studio environment has been set up.
echo.

set PROJECT_NAME=TinyRenderer
set OUTPUT_EXE=%PROJECT_NAME%.exe

REM Include the 'src' directory for header files
set INCLUDE_PATHS=/I"src"

REM Explicitly list all source files
set SOURCE_FILES=^
src\main.cpp ^
src\Pathfinding.cpp ^
src\World.cpp ^
src\AIController.cpp ^
src\Renderer.cpp ^
src\MovementController.cpp ^
src\PredatorAI.cpp ^
src\PreyAI.cpp ^
src\PathfindingHelpers.cpp ^
src\SimulationSetup.cpp ^
src\GameLogic.cpp ^
src\CaptureLogic.cpp ^
src\GridRenderer.cpp ^
src\StatusDisplay.cpp

echo Compiling %PROJECT_NAME%...
echo Files to compile:
echo %SOURCE_FILES%

cl.exe /EHsc /std:c++17 /W4 /Zi %INCLUDE_PATHS% %SOURCE_FILES% /link /OUT:%OUTPUT_EXE%

echo.
if %ERRORLEVEL% EQU 0 (
    echo Compilation successful! Running program with 100 steps limit:
    echo.
    REM Run with environment variable to limit to 100 steps
    set MAX_STEPS=100
    %OUTPUT_EXE%
    echo.
    echo Program completed %MAX_STEPS% steps.
) else (
    echo Compilation failed with error code %ERRORLEVEL%
)

echo.
echo Script finished. 