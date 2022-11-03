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

	// Taken from https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/moller-trumbore-ray-triangle-intersection
	bool Intersects(const Ray& aRay, glm::vec3 aA, glm::vec3 aB, glm::vec3 aC, float& aRayT)
	{
		const glm::vec3 AB = aB - aA;
		const glm::vec3 AC = aC - aA;
		const glm::vec3 perp = glm::cross(aRay.myDir, AC);
		const float det = glm::dot(AB, perp);
		if (det < glm::epsilon<float>())
		{
			return false;
		}

		const float invDet = 1 / det;
		const glm::vec3 tVec = aRay.myOrigin - aA;
		const float u = glm::dot(tVec, perp) * invDet;
		if (u < 0 || u > 1)
		{
			return false;
		}

		const glm::vec3 qVec = glm::cross(tVec, AB);
		const float v = glm::dot(aRay.myDir, qVec) * invDet;
		if (v < 0 || u + v > 1)
		{
			return false;
		}

		aRayT = glm::dot(AC, qVec) * invDet;
		return true;
	}

	// Taken from https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-plane-and-ray-disk-intersection
	bool Intersects(const Ray& aRay, const Plane& aPlane, float& aRayT)
	{
		const float denominator = glm::dot(aPlane.myNormal, aRay.myDir);
		if (glm::abs(denominator) > glm::epsilon<float>())
		{
			const glm::vec3 planeToRayOrigin = aPlane.myPoint - aRay.myOrigin;
			aRayT = glm::dot(planeToRayOrigin, aPlane.myNormal) / denominator;
			return aRayT >= 0;
		}
		return false;
	}

	// Taken from https://tavianator.com/2022/ray_box_boundary.html
	bool Intersects(const Ray& aRay, const AABB& aBox, float& aRayT)
	{
		float tMax = std::numeric_limits<float>::max();
		aRayT = 0;

		const glm::vec3 invDir = 1.f / aRay.myDir;
		for (int32_t i = 0; i < glm::vec3::length(); i++)
		{
			const float t1 = (aBox.myMin[i] - aRay.myOrigin[i]) * invDir[i];
			const float t2 = (aBox.myMax[i] - aRay.myOrigin[i]) * invDir[i];

			aRayT = glm::min(glm::max(t1, aRayT), glm::max(t2, aRayT));
			tMax = glm::max(glm::min(t1, tMax), glm::min(t2, tMax));
		}
		return aRayT <= tMax;
	}
}