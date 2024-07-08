#include "Precomp.h"
#include "Utils.h"

#include <Core/Transform.h>

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

	AABB AABB::Transform(const ::Transform& aTransf) const
	{
		const glm::vec3 e = myMax - myMin;
		const glm::vec4 vertices[]{
			{ myMin, 1.f },
			{ myMin + glm::vec3{ e.x, 0, 0 }, 1.f },
			{ myMin + glm::vec3{ 0, 0, e.z }, 1.f },
			{ myMin + glm::vec3{ e.x, 0, e.z }, 1.f },

			{ myMin + glm::vec3{ 0, e.y, 0 }, 1.f },
			{ myMin + glm::vec3{ e.x, e.y, 0 }, 1.f },
			{ myMin + glm::vec3{ 0, e.y, e.z }, 1.f },
			{ myMax, 1.f }
		};

		const glm::mat4& matrix = aTransf.GetMatrix();
		glm::vec4 transformed = matrix * vertices[0];
		glm::vec4 min = transformed;
		glm::vec4 max = min;
		for(uint8_t i=1; i<8; i++)
		{
			transformed = matrix * vertices[i];
			min = glm::min(min, transformed);
			max = glm::max(max, transformed);
		}
		return { min, max };
	}

	bool Intersects(const AABB& aLeft, const AABB& aRight)
	{
		for (uint8_t axis = 0; axis < 3; axis++)
		{
			const bool overlaps = aRight.myMin[axis] <= aLeft.myMax[axis]
				&& aLeft.myMin[axis] <= aRight.myMax[axis];
			if (!overlaps)
			{
				return false;
			}
		}
		return true;
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

	bool Intersects(glm::vec3 aSpherePos, float aRadius, const AABB& aBox)
	{
		// clamp sphere to the box
		const glm::vec3 pos = glm::clamp(aSpherePos, aBox.myMin, aBox.myMax);
		// point-in-sphere check
		const float sqrDist = glm::distance2(pos, aSpherePos);
		return sqrDist <= aRadius * aRadius;
	}

	// TODO: implement "Fast 3D Triangle-Box Overlap Testing" version and test it
	// https://fileadmin.cs.lth.se/cs/Personal/Tomas_Akenine-Moller/code/tribox_tam.pdf
	bool Intersects(glm::vec3 aV1, glm::vec3 aV2, glm::vec3 aV3, const AABB& aBox)
	{
		auto ProjectTriangle = [=](glm::vec3 anAxis) {
			const float proj[]{
				glm::dot(anAxis, aV1),
				glm::dot(anAxis, aV2),
				glm::dot(anAxis, aV3)
			};

			glm::vec2 result; // min, max
			result.x = proj[0];
			result.y = result.x;

			for (uint8_t i = 1; i < 3; i++)
			{
				result.x = glm::min(result.x, proj[i]);
				result.y = glm::max(result.y, proj[i]);
			}
			return result;
		};

		auto ProjectAABB = [=](glm::vec3 anAxis) {
			const glm::vec3 size = aBox.myMax - aBox.myMin;
			const glm::vec3 vertices[]{
				// bottom
				aBox.myMin,
				aBox.myMin + glm::vec3{size.x, 0, 0},
				aBox.myMin + glm::vec3{0, 0, size.z},
				aBox.myMin + glm::vec3{size.x, 0, size.z},

				// top
				aBox.myMin + glm::vec3{0, size.y, 0},
				aBox.myMin + glm::vec3{size.x, size.y, 0},
				aBox.myMin + glm::vec3{0, size.y, size.z},
				aBox.myMax
			};

			const float proj[]{
				glm::dot(anAxis, vertices[0]),
				glm::dot(anAxis, vertices[1]),
				glm::dot(anAxis, vertices[2]),
				glm::dot(anAxis, vertices[3]),
				glm::dot(anAxis, vertices[4]),
				glm::dot(anAxis, vertices[5]),
				glm::dot(anAxis, vertices[6]),
				glm::dot(anAxis, vertices[7])
			};

			glm::vec2 result; // min, max
			result.x = proj[0];
			result.y = result.x;

			for (uint8_t i = 1; i < 8; i++)
			{
				result.x = glm::min(result.x, proj[i]);
				result.y = glm::max(result.y, proj[i]);
			}
			return result;
		};

		const glm::vec3 right{ 1, 0, 0 };
		const glm::vec3 up{ 0, 1, 0 };
		const glm::vec3 forward{ 0, 0, 1 };

		const glm::vec3 f1 = aV2 - aV1;
		const glm::vec3 f2 = aV3 - aV2;
		const glm::vec3 f3 = aV1 - aV3;

		const glm::vec3 axes[]{
			{1, 0, 0},
			{0, 1, 0},
			{0, 0, 1},
			glm::cross(f1, f2),
			glm::cross(right, f1),
			glm::cross(right, f2),
			glm::cross(right, f3),
			glm::cross(up, f1),
			glm::cross(up, f2),
			glm::cross(up, f3),
			glm::cross(forward, f1),
			glm::cross(forward, f2),
			glm::cross(forward, f3)
		};

		for (glm::vec3 axis : axes)
		{
			const glm::vec2 aabbProj = ProjectAABB(axis);
			const glm::vec2 triangleProj = ProjectTriangle(axis);

			const bool overlaps = triangleProj.x <= aabbProj.y
				&& aabbProj.x <= triangleProj.y;
			if (!overlaps)
			{
				return false;
			}
		}
		return true;
	}
}