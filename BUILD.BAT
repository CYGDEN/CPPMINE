@echo off
glslc -fshader-stage=vertex SHADERS/vertex.glsl -o vert.spv
glslc -fshader-stage=fragment SHADERS/fragment.glsl -o frag.spv
g++ -O2 -o fpsgame.exe MAIN.cpp -lvulkan-1 -lgdi32 -luser32 -lwinmm -mwindows
if %errorlevel%==0 (
    echo BUILD OK
    fpsgame.exe
) else (
    echo BUILD FAILED
    pause
)