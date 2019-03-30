#include "Precomp.h"

#include <Core/Debug/Assert.h>

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
		DEBUG_ONLY(
			for (int y = 0; y < 3; y++)
			{
				for (int x = 0; x < 3; x++)
				{
					ASSERT_STR(aMat[y][x] >= -1.f && aMat[y][x] <= 1.f, "Bullet doesn't support scaling of it's matrices!");
				}
			}
		);

		btTransform val;
		val.setFromOpenGLMatrix(glm::value_ptr(aMat));
		return val;
	}
}