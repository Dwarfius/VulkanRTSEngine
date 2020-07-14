#pragma once

#include "ImGUIGLFWImpl.h"

class Game;

// This class handles all the necessary logic for handling input, resources, etc
class ImGUISystem
{
public:
	ImGUISystem(Game& aGame);

	void Init();
	void Shutdown();
	void NewFrame(float aDeltaTime);

	tbb::mutex& GetMutex() { return myMutex; }

private:
	Game& myGame;
	tbb::mutex myMutex;
	ImGUIGLFWImpl myGLFWImpl;
};