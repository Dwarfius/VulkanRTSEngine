#pragma once

#include <iostream>
#include <vector>
#include <array>
#include <string>
#include <thread>
#include <fstream>
#include <algorithm>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <atomic>
#include <random>
#include <chrono>

#define NOMINMAX
#include <tbb/tbb.h>
#undef NOMINMAX

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/hash.hpp>

#include <AL/alut.h>

#define GLEW_STATIC
#include <GL/glew.h>

// Forcing this define for 32bit typesafe conversions, as in
// being able to construct c++ classes based of vulkan C handles
// theoretically this is unsafe - check vulkan.hpp for more info
#define VULKAN_HPP_TYPESAFE_CONVERSION
#include <vulkan/vulkan.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <tiny_obj_loader.h>
#include <stb_image.h>

using namespace std;