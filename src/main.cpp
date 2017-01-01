#include "Common.h"
#include "Game.h"
#include "Graphics.h"

int main()
{
	glfwInit();

	if (glfwVulkanSupported() == GLFW_FALSE)
	{
		printf("[Error] Vulkan loader unavailable");
		return -1;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow *window = glfwCreateWindow(SCREEN_W, SCREEN_H, "Vulkan RTS Engine", nullptr, nullptr);

	Graphics::Init(window);

	glfwSetTime(0);
	float oldTime = 0;

	Game *game = new Game();
	game->Init();
	while (!glfwWindowShouldClose(window) && game->IsRunning()) 
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

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}