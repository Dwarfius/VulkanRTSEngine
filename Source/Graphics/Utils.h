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
}