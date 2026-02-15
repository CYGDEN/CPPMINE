#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#undef near
#undef far
#include <vulkan/vulkan.h>

#include <vector>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <algorithm>
#include <random>
#include <chrono>
#include <fstream>
#include <string>

#include "TYPES.cpp"
#include "SOUNDMANAGER.cpp"
#include "GRAPHICS.cpp"
#include "ALLOPTIMIZER.cpp"
#include "BLOCK_PHYSICS.cpp"
#include "BLOCK_FRACTURE.cpp"
#include "BLOCK_PARTICLES.cpp"
#include "BLOCK_DELETE.cpp"