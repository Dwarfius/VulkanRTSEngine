#pragma once

class GameObject;
class Transform;
class Game;

class Gizmos
{
	enum class Axis : uint8_t
	{
		Right,
		Up,
		Forward,
		None
	};
	constexpr static float kGizmoRange = 1.f;

public:
	// Returns true if currently handling input
	bool Draw(GameObject& aGameObj, Game& aGame);
	// Returns true if currently handling input
	bool Draw(Transform& aTransf, Game& aGame);

private:
	bool DrawTranslation(Transform& aTransf, Game& aGame);

	glm::vec3 myOldMousePosWS;
	Axis myPickedAxis = Axis::None;
};