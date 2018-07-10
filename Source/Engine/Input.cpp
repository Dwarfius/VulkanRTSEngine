#include "Common.h"
#include "Input.h"

GLFWwindow* Input::ourWindow = nullptr;
char Input::ourKbState[ourKeyCount];
char Input::ourMState[ourMButtonCount];
glm::vec2 Input::ourOldPos, Input::ourPos;

void Input::SetWindow(GLFWwindow *aWindow)
{
	// cleanup old window if it was set
	if (ourWindow)
	{
		glfwSetKeyCallback(ourWindow, nullptr);
		glfwSetMouseButtonCallback(ourWindow, nullptr);
	}

	ourWindow = aWindow;
	glfwSetKeyCallback(ourWindow, Input::KeyCallback);
	glfwSetMouseButtonCallback(ourWindow, Input::MouseCallback);

	memset(ourKbState, 0, sizeof(ourKbState));
	memset(ourMState, 0, sizeof(ourMState));

	// Update them to the same value to avoid issues
	ourPos = ourOldPos = GetMousePos();
}

void Input::Update()
{
	ourOldPos = ourPos;
	ourPos = GetMousePos();
}

// This needs to be called at the end of the update loop - it gets around the slow repeat rate of OS
// by automatically marking the button as repeated
void Input::PostUpdate()
{
	for (int i = 0; i < ourKeyCount; i++)
	{
		if (ourKbState[i] == GLFW_PRESS)
		{
			ourKbState[i] = GLFW_REPEAT;
		}
	}
	for (int i = 0; i < ourMButtonCount; i++)
	{
		if (ourMState[i] == GLFW_PRESS)
		{
			ourMState[i] = GLFW_REPEAT;
		}
	}
}

bool Input::GetKey(char asciiCode)
{
	return ourKbState[asciiCode] == GLFW_PRESS || ourKbState[asciiCode] == GLFW_REPEAT;
}

bool Input::GetKeyPressed(char asciiCode)
{
	return ourKbState[asciiCode] == GLFW_PRESS;
}

glm::vec2 Input::GetMousePos()
{
	double x, y;
	glfwGetCursorPos(ourWindow, &x, &y);
	return glm::vec2(x, y);
}

glm::vec2 Input::GetMouseDelta()
{
	return ourPos - ourOldPos;
}

bool Input::GetMouseBtn(char btn)
{
	return ourMState[btn] == GLFW_PRESS || ourMState[btn] == GLFW_REPEAT;
}

bool Input::GetMouseBtnPressed(char btn)
{
	return ourMState[btn] == GLFW_PRESS;
}

void Input::KeyCallback(GLFWwindow* aWindow, int aKey, int aScanCode, int anAction, int aMods)
{
	ASSERT_STR(aKey >= 0 && aKey < ourKeyCount, "Unsupported key: %d", aKey);
	ourKbState[RemapKey(aKey)] = anAction;
}

void Input::MouseCallback(GLFWwindow* aWindow, int aButton, int anAction, int aMods)
{
	ASSERT_STR(aButton >= 0 && aButton < ourMButtonCount, "Unsupported button: %d", aButton);
	ourMState[aButton] = anAction;
}

int Input::RemapKey(int key)
{
	switch (key)
	{
	case GLFW_KEY_ESCAPE:
		return 27;
	case GLFW_KEY_ENTER:
		return 10;
	case GLFW_KEY_LEFT_SHIFT:
		return 11;
	case GLFW_KEY_RIGHT_SHIFT:
		return 12;
	default:
		return key;
	}
}