#pragma once

class Game;
class GameObject;

class EntitiesWidget
{
public:
	// Draws as a widget within the current window
	// Precondition: ImGui lock acquired
	void Draw(Game& aGame);

	// Draws as a separate window
	// Precondition: ImGui lock free
	void DrawDialog(Game& aGame);

private:
	void DrawGameObject(GameObject& aGo);
	void DrawChildren(GameObject& aGo);
};