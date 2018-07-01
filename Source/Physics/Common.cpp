#include "Common.h"

// TODO: move this to Common module
#define FORCE_SEMICOLON (void)0
#ifdef NDEBUG
#	define DEBUG_ONLY(x)
#else
#	define DEBUG_ONLY(x) x FORCE_SEMICOLON
#endif // NDEBUG

namespace Utils
{
	glm::vec3 ConvertToGLM(btVector3 aVec)
	{
		glm::vec3 val;
		memcpy(glm::value_ptr(val), aVec.m_floats, sizeof(glm::vec3));
		return val;
	}

	glm::vec4 ConvertToGLM(btVector4 aVec)
	{
		glm::vec4 val;
		memcpy(glm::value_ptr(val), aVec.m_floats, sizeof(glm::vec4));
		return val;
	}

	glm::mat4 ConvertToGLM(const btTransform& aTransf)
	{
		glm::mat4 val;
		aTransf.getOpenGLMatrix(glm::value_ptr(val));
		return val;
	}

	btVector3 ConvertToBullet(glm::vec3 aVec)
	{
		btVector3 val;
		memcpy(val.m_floats, glm::value_ptr(aVec), sizeof(glm::vec3));
		return val;
	}

	btVector4 ConvertToBullet(glm::vec4 aVec)
	{
		btVector4 val;
		memcpy(val.m_floats, glm::value_ptr(aVec), sizeof(glm::vec4));
		return val;
	}

	btTransform ConvertToBullet(const glm::mat4& aMat)
	{
		DEBUG_ONLY( // be warned, if aMat has scaling, it'll screw everything with Bullet - tried and tested
			for (int y = 0; y < 3; y++)
			{
				for (int x = 0; x < 3; x++)
				{
					assert(aMat[y][x] >= -1.f && aMat[y][x] <= 1.f);
				}
			}
		);

		btTransform val;
		val.setFromOpenGLMatrix(glm::value_ptr(aMat));
		return val;
	}

	bool IsNan(glm::vec3 aVec)
	{
		return glm::isnan(aVec) == glm::vec3::bool_type(true);
	}

	bool IsNan(glm::vec4 aVec)
	{
		return glm::isnan(aVec) == glm::vec4::bool_type(true);
	}
}