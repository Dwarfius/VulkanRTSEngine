#include "Common.h"
#include "Game.h"
#include "Graphics.h"

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

		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			break;

		game->Update();
		game->Render();
	}
	game->CleanUp();
	delete game;
	
	glfwTerminate();

	return 0;
}