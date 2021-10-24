#pragma once

#include <Graphics/Camera.h>
#include <Core/RefCounted.h>

class Game;
enum class PaintMode : int;
class GPUTexture;

class Editor
{
public:
	Editor(Game& aGame);
	void Run();

private:
	void ProcessInput();
	void Draw();
	void DrawGeneralSettings();
	void DrawPaintSettings();

	Game& myGame;
	Camera myCamera;
	glm::vec3 myColor{ 1 };
	glm::vec2 myTexSize;
	glm::vec2 myPrevMousePos;
	glm::vec2 myMousePos;
	glm::ivec2 myGridDims{ 20, 10 };
	glm::vec2 myDragStart{ 0, 0 };
	PaintMode myPaintMode;
	int myBrushRadius = 1;
	float myScale = 1.f;
	bool myPaintingColor = true;

	Handle<GPUTexture> myPaintTexture;
	std::string myTexturePath;
	float myInverseScale = 1.f;
};