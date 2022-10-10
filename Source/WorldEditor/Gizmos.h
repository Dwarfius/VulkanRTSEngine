#pragma once

class GameObject;
class Transform;
class Game;
class DebugDrawer;

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
		Rotation
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

	glm::vec3 myOldMousePosOrDir;
	glm::vec3 myRotationDirStart;
	Axis myPickedAxis = Axis::None;
	Mode myMode = Mode::Translation;
};