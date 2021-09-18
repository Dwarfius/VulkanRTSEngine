#pragma once

class Game;

class Editor
{
public:
	Editor(Game& aGame);
	void Run();

private:
	void Draw();

	Game& myGame;
	glm::vec2 myTexSize;
	glm::vec2 myMousePos;
	int myPaintMode = 0;
	float myBrushRadius = 0.001f;
};