#pragma once

#include <Graphics/Camera.h>
#include <Core/RefCounted.h>
#include <Core/Utils.h>

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

	void LoadTextures(std::string_view aPath);
	void DrawTextures();

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
	float myInverseScale = 1.f;

	std::unordered_map<std::string, Handle<GPUTexture>> myTextures;
	std::string myTexturesPath;
	bool myTexturesWindowOpen = false;
	std::atomic<uint8_t> myLoadReqCounter;
	std::atomic<uint64_t> myWorkItemCounter;
	std::atomic<uint64_t> myTotalTextures;
	std::mutex myTexturesMutex;
	tbb::task_arena myTaskArena{
		glm::max(std::thread::hardware_concurrency() / 4, 1u),
		1, 
		tbb::task_arena::priority::low
	};
	Utils::AffinitySetter myAffinitySetter{ myTaskArena, Utils::AffinitySetter::Priority::Low };
};