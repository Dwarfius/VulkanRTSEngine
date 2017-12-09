#include "Common.h"
#include "Game.h"
#include "Graphics.h"
#include <GLFW/glfw3.h>

void glfwErrorReporter(int code, const char* desc)
{
	printf("GLFW error(%d): %s", code, desc);
}

int main()
{
	srand(static_cast<uint32_t>(time(0)));
	glfwSetErrorCallback(&glfwErrorReporter);
	glfwInit();

	glfwSetTime(0);

	Game *game = new Game();
	game->Init();
	GLFWwindow *window = glfwGetCurrentContext();
	while (!glfwWindowShouldClose(window) && game->IsRunning())
	{
		glfwPollEvents();

		// TODO: need to investigate the performance dip
		// TODO: need to refactor this
		game->RunMainThread();
	}
	game->CleanUp();
	delete game;
	
	glfwTerminate();

	return 0;
}