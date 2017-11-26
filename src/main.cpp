#include "Common.h"
#include "Game.h"
#include "Graphics.h"
#include <GLFW/glfw3.h>

int main()
{
	srand(static_cast<uint32_t>(time(0)));
	glfwInit();

	glfwSetTime(0);

	Game *game = new Game();
	game->Init();
	GLFWwindow *window = game->GetGraphicsRaw()->GetWindow();
	while (!glfwWindowShouldClose(window) && game->IsRunning())
	{
		glfwPollEvents();

		game->RunTaskGraph();
	}
	game->CleanUp();
	delete game;
	
	glfwTerminate();

	return 0;
}