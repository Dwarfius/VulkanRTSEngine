#pragma once

namespace Utils
{
	glm::vec2 WorldToScreen(
		glm::vec3 aWorldPos, 
		glm::vec2 aScreenSize, 
		const glm::mat4& aViewProjMat
	);

	glm::vec3 ScreenToWorld(
		glm::vec2 aScreenPos,
		float aNormDepth,
		glm::vec2 aScreenSize,
		const glm::mat4& aViewProjMat
	);

	bool IsInTriangle(
		glm::vec2 aPos,
		glm::vec2 aA,
		glm::vec2 aB,
		glm::vec2 aC
	);

	struct Ray
	{
		glm::vec3 myOrigin;
		glm::vec3 myDir;
	};
	// Find parametric factors Ta and Tb for 2 points closest
	// between rays A and B
	void GetClosestTBetweenRays(const Ray& aA, const Ray& aB, float& aAT, float& aBT);

	bool Intersects(const Ray& aRay, glm::vec3 aA, glm::vec3 aB, glm::vec3 aC, float& aRayT);

	struct Plane
	{
		glm::vec3 myPoint;
		glm::vec3 myNormal;
	};
	bool Intersects(const Ray& aRay, const Plane& aPlane, float& aRayT);

	struct AABB
	{
		glm::vec3 myMin;
		glm::vec3 myMax;
	};
	bool Intersects(const Ray& aRay, const AABB& aBox, float& aRayT);

	bool Intersects(glm::vec3 aV1, glm::vec3 aV2, glm::vec3 aV3, const AABB& aBox);
}