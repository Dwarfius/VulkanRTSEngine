#pragma once

#include <unordered_map>
#include <algorithm>
#include <vector>
#include <string>
#include <functional>
#include <atomic>
#include <queue>
#include <fstream>
#include <memory>
#include <limits>
#include <mutex>
#include <span>
#include <array>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/norm.hpp>

#include <tbb/concurrent_queue.h>
#include <tbb/concurrent_unordered_set.h>
#include <tbb/spin_mutex.h>
#include <tbb/task_scheduler_observer.h>
#include <tbb/queuing_mutex.h>
#include <tbb/enumerable_thread_specific.h>
#include <tbb/task_group.h>
#include <tbb/parallel_for.h>

#include <Core/Debug/Assert.h>