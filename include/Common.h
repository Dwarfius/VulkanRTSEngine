#ifndef _COMMON_H
#define _COMMON_H

#include <vulkan/vulkan.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>

using namespace std;
using namespace glm;

#define SCREEN_W 800
#define SCREEN_H 600

#endif // !_COMMON_H