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
set SOURCE_FILES=src\main.cpp src\Pathfinding.cpp src\World.cpp src\AIController.cpp src\Renderer.cpp
set OUTPUT_EXE=%PROJECT_NAME%.exe

REM Include the 'src' directory for header files (Sprite.h, Vec2D.h, Pathfinding.h, World.h)
set INCLUDE_PATHS=/I"src"

echo Compiling %PROJECT_NAME% from %SOURCE_FILES%...
cl.exe /EHsc /std:c++17 /W4 /Zi %INCLUDE_PATHS% %SOURCE_FILES% /link /OUT:%OUTPUT_EXE%

echo.
if %ERRORLEVEL% EQU 0 (
    echo Compilation successful! Running program with 100 steps limit:
    echo.
    REM Run with environment variable to limit to 100 steps
    set MAX_STEPS=100
    %OUTPUT_EXE%
    echo.
    echo Program completed 100 steps. Press any key to exit...
    pause > nul
) else (
    echo Compilation failed with error code %ERRORLEVEL%
)

echo.
echo Script finished. 