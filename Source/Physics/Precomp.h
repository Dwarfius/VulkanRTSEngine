#include <cstring>
#include <vector>
#include <unordered_map>
#include <memory>
#include <atomic>
#include <mutex>
#include <unordered_set>

#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <tbb/spin_mutex.h>
#include <tbb/task_scheduler_observer.h>
#include <tbb/queuing_mutex.h>
#include <tbb/enumerable_thread_specific.h>
#include <tbb/task_group.h>

#include <Core/Debug/Assert.h>

namespace Utils
{
	glm::vec3 ConvertToGLM(btVector3 aVec);
	glm::vec4 ConvertToGLM(btVector4 aVec);
	glm::mat4 ConvertToGLM(const btTransform& aTransf);
	btVector3 ConvertToBullet(glm::vec3 aVec);
	btVector4 ConvertToBullet(glm::vec4 aVec);
	btTransform ConvertToBullet(const glm::mat4& aMat);
}