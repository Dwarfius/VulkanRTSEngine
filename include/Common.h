#ifndef _COMMON_H
#define _COMMON_H

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <vector>
#include <array>
#include <string>

using namespace std;
using namespace glm;

#define SCREEN_W (uint32_t)800
#define SCREEN_H (uint32_t)600

#endif // !_COMMON_H