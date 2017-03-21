#include "Common.h"
#include "Game.h"
#include "Graphics.h"
#include <GLFW/glfw3.h>

int main()
{
	glfwInit();

	glfwSetTime(0);

	Game *game = new Game();
	game->Init();
	GLFWwindow *window = Game::GetGraphics()->GetWindow();
	while (!glfwWindowShouldClose(window) && game->IsRunning())
	{
		glfwPollEvents();

		game->Update();
		game->CollisionUpdate();
		if(game->IsRunning())
			game->Render();
	}
	game->CleanUp();
	delete game;
	
	glfwTerminate();

	return 0;
}