#ifndef _COMMON_H
#define _COMMON_H

#if defined(GRAPHICS_VK)
	// Forcing this define for 32bit typesafe conversions, as in
	// being able to construct c++ classes based of vulkan c handles
	// theoretically this is unsafe - check vulkan.hpp for more info
	#define VULKAN_HPP_TYPESAFE_CONVERSION
	#include <vulkan/vulkan.hpp>
	#define GLFW_INCLUDE_VULKAN
#elif defined(GRAPHICS_GL)
	#include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>

using namespace std;
using namespace glm;

#define SCREEN_W (uint32_t)800
#define SCREEN_H (uint32_t)600

#endif // !_COMMON_H