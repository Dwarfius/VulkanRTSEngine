#pragma once

class GameObject;
class Transform;
class Game;

namespace Utils
{
	struct Ray;
};

class Gizmos
{
	enum class Axis : uint8_t
	{
		Right,
		Up,
		Forward,
		None
	};

	enum class Mode : uint8_t
	{
		Translation,
		Rotation,
		Scale
	};

	enum class Space : uint8_t
	{
		World,
		Local
	};
	constexpr static float kGizmoRange = 1.f;
	constexpr static glm::vec3 kHighlightColor{ 1, 1, 0 };

public:
	// Returns true if currently handling input
	bool Draw(GameObject& aGameObj, Game& aGame);
	// Returns true if currently handling input
	bool Draw(Transform& aTransf, Game& aGame);

private:
	bool DrawTranslation(Transform& aTransf, Game& aGame);
	bool DrawRotation(Transform& aTransf, Game& aGame);
	bool DrawScale(Transform& aTransf, Game& aGame);

	static void GenerateArrow(glm::vec3 aStart, glm::vec3 anEnd, glm::vec3(&aVerts)[3]);
	static void DrawArrow(DebugDrawer& aDrawer, glm::vec3 aStart, glm::vec3 anEnd, glm::vec3(&aVerts)[3], glm::vec3 aColor);
	static bool CheckCircle(glm::vec3 aCenter, glm::vec3 aNormal, float aRadius,
		const Utils::Ray& aRay, glm::vec3& anIntersectPoint);
	static Utils::Ray GetCameraRay(const Game& aGame);

	glm::vec3 myOldMousePosOrDir;
	glm::vec3 myRotationDirStart;
	Axis myPickedAxis = Axis::None;
	Mode myMode = Mode::Translation;
	Space mySpace = Space::World;
};