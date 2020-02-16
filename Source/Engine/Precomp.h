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
#include <memory>

#ifdef USE_AUDIO
#include <AL/alut.h>
#endif

#define GLEW_STATIC
#include <GL/glew.h>

#ifdef USE_VULKAN
// Forcing this define for 32bit typesafe conversions, as in
// being able to construct c++ classes based of vulkan C handles
// theoretically this is unsafe - check vulkan.hpp for more info
#define VULKAN_HPP_TYPESAFE_CONVERSION
#include <vulkan/vulkan.hpp>
#endif

#include <tbb/tbb.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/norm.hpp>

#include <GLFW/glfw3.h>

#include <Core/Debug/Assert.h>

using namespace std;