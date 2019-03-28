#include <cstring>
#include <vector>
#include <unordered_map>
#include <memory>
#include <any>
#include <atomic>

#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <tbb/tbb.h>

#include "Debug/Assert.h"

using namespace std;

namespace Utils
{
	glm::vec3 ConvertToGLM(btVector3 aVec);
	glm::vec4 ConvertToGLM(btVector4 aVec);
	glm::mat4 ConvertToGLM(const btTransform& aTransf);
	btVector3 ConvertToBullet(glm::vec3 aVec);
	btVector4 ConvertToBullet(glm::vec4 aVec);
	btTransform ConvertToBullet(const glm::mat4& aMat);
}