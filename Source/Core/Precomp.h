#pragma once

#include <iostream>
#include <vector>
#include <array>
#include <string>
#include <string_view>
#include <thread>
#include <fstream>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <atomic>
#include <random>
#include <chrono>
#include <memory>
#include <variant>
#include <mutex>
#include <print>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/hash.hpp>

#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>
#include <tbb/spin_mutex.h>
#include <tbb/queuing_mutex.h>
#include <tbb/task_arena.h>
#include <tbb/task_group.h>
#include <tbb/enumerable_thread_specific.h>
#include <tbb/task_scheduler_observer.h>

#include <GLFW/glfw3.h>

#include "Debug/Assert.h"