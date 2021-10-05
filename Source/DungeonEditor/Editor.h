#pragma once

#include <Graphics/Camera.h>

class Game;

class Editor
{
public:
	Editor(Game& aGame);
	void Run();

private:
	void ProcessInput();
	void Draw();

	Game& myGame;
	Camera myCamera;
	glm::vec3 myColor{ 1 };
	glm::vec2 myTexSize;
	glm::vec2 myPrevMousePos;
	glm::vec2 myMousePos;
	glm::ivec2 myGridDims{ 15, 10 };
	glm::vec2 myDragStart{ 0, 0 };
	int myPaintMode = 0;
	float myBrushRadius = 0.003f;
	float myScale = 1.f;
};