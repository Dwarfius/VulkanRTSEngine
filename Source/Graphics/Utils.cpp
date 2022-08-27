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

	// Taken from https://stackoverflow.com/questions/2049582/how-to-determine-if-a-point-is-in-a-2d-triangle
	bool IsInTriangle(
		glm::vec2 aPos,
		glm::vec2 aA,
		glm::vec2 aB,
		glm::vec2 aC
	)
	{
		const float A = 1 / 2.f * (-aB.y * aC.x + aA.y * (-aB.x + aC.x) + aA.x * (aB.y - aC.y) + aB.x * aC.y);
		const float sign = A < 0 ? -1.f : 1.f;
		const float s = (aA.y * aC.x - aA.x * aC.y + (aC.y - aA.y) * aPos.x + (aA.x - aC.x) * aPos.y) * sign;
		const float t = (aA.x * aB.y - aA.y * aB.x + (aA.y - aB.y) * aPos.x + (aB.x - aA.x) * aPos.y) * sign;

		return s > 0 && t > 0 && (s + t) < 2 * A * sign;
	}

	// Taken from https://nelari.us/post/gizmos/
	void GetClosestTBetweenRays(const Ray& aA, const Ray& aB, float& aAT, float& aBT)
	{
		const glm::vec3 dir = aB.myOrigin - aA.myOrigin;
		const float v1Sqr = glm::dot(aA.myDir, aA.myDir);
		const float v2Sqr = glm::dot(aB.myDir, aB.myDir);
		const float v1v2 = glm::dot(aA.myDir, aB.myDir);

		const float det = v1v2 * v1v2 - v1Sqr * v2Sqr;

		if (glm::abs(det) > glm::epsilon<float>()) [[likely]]
		{
			const float invDet = 1.f / det;
			const float dpv1 = glm::dot(dir, aA.myDir);
			const float dpv2 = glm::dot(dir, aB.myDir);

			aAT = invDet * (v1v2 * dpv2 - v2Sqr * dpv1);
			aBT = invDet * (v1Sqr * dpv2 - v1v2 * dpv1);
		}
		else
		{
			// safe defaults
			aAT = 0;
			aBT = 0;
		}
	}
}