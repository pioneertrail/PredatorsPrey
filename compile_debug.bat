@echo on
cl /EHsc /std:c++17 /W4 /Zi /I"src" src\main.cpp src\Pathfinding.cpp src\World.cpp src\AIController.cpp src\Renderer.cpp /link /OUT:TinyRenderer.exe
pause 