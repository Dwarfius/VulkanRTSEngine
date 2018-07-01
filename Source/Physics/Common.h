#include <cstring>
#include <vector>

#include <tbb/tbb.h>
#undef max
#undef min

#include <btBulletDynamicsCommon.h>
#include <BulletCollision\CollisionShapes\btHeightfieldTerrainShape.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;

namespace Utils
{
	// TODO: Bullet is a right-hand coord system, while VEngine is left-handed
	// TODO: Check if OpenGL is right handed
	glm::vec3 ConvertToGLM(btVector3 aVec);
	glm::vec4 ConvertToGLM(btVector4 aVec);
	glm::mat4 ConvertToGLM(const btTransform& aTransf);
	btVector3 ConvertToBullet(glm::vec3 aVec);
	btVector4 ConvertToBullet(glm::vec4 aVec);
	btTransform ConvertToBullet(const glm::mat4& aMat);

	bool IsNan(glm::vec3 aVec);
	bool IsNan(glm::vec4 aVec);
}