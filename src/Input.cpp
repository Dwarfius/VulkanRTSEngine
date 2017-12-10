#include "Common.h"
#include "Input.h"

GLFWwindow* Input::window = nullptr;
char Input::kbState[keyCount];
char Input::mState[mButtonCount];
vec2 Input::oldPos, Input::pos;

void Input::SetWindow(GLFWwindow *w)
{
	// cleanup old window if it was set
	if (window)
	{
		glfwSetKeyCallback(window, nullptr);
		glfwSetMouseButtonCallback(window, nullptr);
	}

	window = w;
	glfwSetKeyCallback(window, Input::KeyCallback);
	glfwSetMouseButtonCallback(window, Input::MouseCallback);

	memset(kbState, 0, sizeof(kbState));
	memset(mState, 0, sizeof(mState));
}

void Input::Update()
{
	oldPos = pos;
	pos = GetMousePos();
}

// This needs to be called at the end of the update loop - it gets around the slow repeat rate of OS
// by automatically marking the button as repeated
void Input::PostUpdate()
{
	for (int i = 0; i < keyCount; i++)
	{
		if (kbState[i] == GLFW_PRESS)
			kbState[i] = GLFW_REPEAT;
	}
	for (int i = 0; i < mButtonCount; i++)
	{
		if (mState[i] == GLFW_PRESS)
			mState[i] = GLFW_REPEAT;
	}
}

bool Input::GetKey(char asciiCode)
{
	return kbState[asciiCode] == GLFW_PRESS || kbState[asciiCode] == GLFW_REPEAT;
}

bool Input::GetKeyPressed(char asciiCode)
{
	return kbState[asciiCode] == GLFW_PRESS;
}

vec2 Input::GetMousePos()
{
	double x, y;
	glfwGetCursorPos(window, &x, &y);
	return vec2(x, y);
}

vec2 Input::GetMouseDelta()
{
	return pos - oldPos;
}

bool Input::GetMouseBtn(char btn)
{
	return mState[btn] == GLFW_PRESS || mState[btn] == GLFW_REPEAT;
}

bool Input::GetMouseBtnPressed(char btn)
{
	return mState[btn] == GLFW_PRESS;
}

void Input::KeyCallback(GLFWwindow *window, int key, int scanCode, int action, int mods)
{
	// safety check
	printf("%d %d\n", key, action);
	if (key < 0 || key >= 400)
		printf("[Warning] Unrecognized key pressed(key: %d, scanCode: %d)\n", key, scanCode);
	else
		kbState[RemapKey(key)] = action;
}

void Input::MouseCallback(GLFWwindow *window, int button, int action, int mods)
{
	if (button < 0 || button >= 8)
		printf("[Warning] Unrecognized mouse btn pressed(btn: %d)\n", button);
	else
		mState[button] = action;
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