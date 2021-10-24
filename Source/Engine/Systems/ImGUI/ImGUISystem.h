#pragma once

#include "ImGUIGLFWImpl.h"
#include <Core/RefCounted.h>

class Game;
class ImGUIRenderPass;
class GPUTexture;

// This class handles all the necessary logic for handling input, resources, etc
class ImGUISystem
{
public:
	ImGUISystem(Game& aGame);

	void Init();
	void Shutdown();
	void NewFrame(float aDeltaTime);

	std::mutex& GetMutex() { return myMutex; }
	// Helper for safely scheduling a drawing of texture
	// Ensures it lives through till the end of a frame
	void Image(Handle<GPUTexture> aTexture, glm::vec2 aSize, 
		glm::vec2 aUV0 = glm::vec2(0), glm::vec2 aUV1 = glm::vec2(1), 
		glm::vec4 aTintColor = glm::vec4(1), 
		glm::vec4 aBorderColor = glm::vec4(0)
	);

private:
	void Render();

	Game& myGame;
	std::mutex myMutex;
	ImGUIGLFWImpl myGLFWImpl;
	ImGUIRenderPass* myRenderPass;
	std::vector<Handle<GPUTexture>> myKeepAliveTextures;
};