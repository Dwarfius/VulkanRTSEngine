#include "Common.h"

int main()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow *window = glfwCreateWindow(800, 600, "Vulkan RTS Engine", nullptr, nullptr);

	//enumerating available extensions
	uint32_t extensionCount = 0;
	vk::enumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	printf("Found %d Vulkan extensions\n", extensionCount);
	vk::ExtensionProperties *props = new vk::ExtensionProperties[extensionCount];
	vk::enumerateInstanceExtensionProperties(nullptr, &extensionCount, props);
	for (int i = 0; i < extensionCount; i++)
		printf("  %d: %s(%d)\n", i, props[i].extensionName, props[i].specVersion);
	delete[] props;

	while (!glfwWindowShouldClose(window)) 
	{
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}