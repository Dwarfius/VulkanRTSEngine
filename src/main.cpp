#include "Common.h"
#include "Game.h"

int main()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow *window = glfwCreateWindow(SCREEN_W, SCREEN_H, "Vulkan RTS Engine", nullptr, nullptr);

	//enumerating available extensions
	uint32_t extensionCount = 0;
	vk::enumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	printf("Found %d Vulkan extensions\n", extensionCount);
	vk::ExtensionProperties *props = new vk::ExtensionProperties[extensionCount];
	vk::enumerateInstanceExtensionProperties(nullptr, &extensionCount, props);
	for (int i = 0; i < extensionCount; i++)
		printf("  %d: %s(%d)\n", i, props[i].extensionName, props[i].specVersion);
	delete[] props;

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

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}