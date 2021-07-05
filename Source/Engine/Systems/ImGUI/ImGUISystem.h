#pragma once

#include "ImGUIGLFWImpl.h"

class Game;
class ImGUIRenderPass;

// This class handles all the necessary logic for handling input, resources, etc
class ImGUISystem
{
public:
	ImGUISystem(Game& aGame);

	void Init();
	void Shutdown();
	void NewFrame(float aDeltaTime);
	void Render();

	std::mutex& GetMutex() { return myMutex; }

private:
	Game& myGame;
	std::mutex myMutex;
	ImGUIGLFWImpl myGLFWImpl;
	ImGUIRenderPass* myRenderPass;
};