#include "Common.h"
#include <Game.h>

void glfwErrorReporter(int code, const char* desc)
{
	printf("GLFW error(%d): %s", code, desc);
}

int main()
{
	srand(static_cast<uint32_t>(time(0)));

	Game* game = new Game(&glfwErrorReporter);
	game->Init();
	while (game->IsRunning())
	{
		game->RunMainThread();
	}
	game->CleanUp();
	delete game;

	return 0;
}