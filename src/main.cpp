#include "Common.h"
#include "Game.h"
#include "Graphics.h"

int main()
{
	glfwInit();
	Graphics::Init();

	glfwSetTime(0);
	float oldTime = 0;

	Game *game = new Game();
	game->Init();
	while (!glfwWindowShouldClose(Graphics::GetWindow()) && game->IsRunning()) 
	{
		glfwPollEvents();

		if (glfwGetKey(Graphics::GetWindow(), GLFW_KEY_ESCAPE) == GLFW_PRESS)
			break;

		float newTime = glfwGetTime();
		float deltaTime = newTime - oldTime;
		oldTime = newTime;
		game->Update(deltaTime);
		game->Render();
	}
	game->CleanUp();
	delete game;

	Graphics::CleanUp();
	glfwTerminate();

	return 0;
}