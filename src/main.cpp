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

		float newTime = glfwGetTime();
		float deltaTime = newTime - oldTime;
		game->Update(deltaTime);
		game->Render();
	}
	game->CleanUp();
	delete game;

	Graphics::CleanUp();
	glfwTerminate();

	return 0;
}