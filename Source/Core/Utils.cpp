#include "Precomp.h"
#include "Utils.h"

namespace Utils
{
	bool IsNan(glm::vec3 aVec)
	{
		return glm::isnan(aVec) == glm::vec3::bool_type(true);
	}

	bool IsNan(glm::vec4 aVec)
	{
		return glm::isnan(aVec) == glm::vec4::bool_type(true);
	}
}