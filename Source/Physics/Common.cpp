#include "Common.h"

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

	// be warned, if aMat has scaling, it'll screw everything with Bullet - tried and tested
	btTransform ConvertToBullet(const glm::mat4& aMat)
	{
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