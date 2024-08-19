#include "Precomp.h"
#include "Utils.h"

namespace Utils
{
	glm::vec2 WorldToScreen(
		glm::vec3 aWorldPos,
		glm::vec2 aScreenSize,
		const glm::mat4& aViewProjMat
	)
	{
		const glm::vec4 projected = aViewProjMat * glm::vec4(aWorldPos, 1);
		const glm::vec2 denormalized = glm::vec2(projected.x, projected.y)
			/ projected.w;
		const glm::vec2 normalized = denormalized / 2.f + 0.5f;
		const glm::vec2 normalizedYFlipped{ normalized.x, 1.f - normalized.y };
		const glm::vec2 screenSpace = normalizedYFlipped * aScreenSize;
		return screenSpace;
	}

	glm::vec3 ScreenToWorld(
		glm::vec2 aScreenPos,
		float aNormDepth,
		glm::vec2 aScreenSize,
		const glm::mat4& aViewProjMat
	)
	{
		const glm::vec2 normalized = aScreenPos / aScreenSize;
		const glm::vec2 normalizedYFlipped{ normalized.x, 1.f - normalized.y };
		const glm::vec2 negNormalized = normalizedYFlipped * 2.f - 1.f;
		const glm::mat4 inverseVP = glm::inverse(aViewProjMat);
		const glm::vec4 wsProjNorm = inverseVP * glm::vec4{
			negNormalized.x, negNormalized.y, aNormDepth, 1
		};
		const glm::vec4 wsProj = wsProjNorm * (1.f / wsProjNorm.w);
		return wsProj;
	}
}