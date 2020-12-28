#pragma once

#include <string_view>
#include <string>
#include <variant>
#include <utility>
#include <queue>
#include <type_traits>
#include <unordered_map>
#include <iterator>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <benchmark/benchmark.h>
#include <nlohmann/json.hpp>
#include "Extra/fifo_map.hpp"

#include <Core/Debug/Assert.h>