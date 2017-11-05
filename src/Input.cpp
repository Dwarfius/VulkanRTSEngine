#include "Common.h"
#include "Input.h"

GLFWwindow* Input::window = nullptr;
bool Input::kbState[400], Input::kbOldState[400];
bool Input::mState[8], Input::mOldState[8];
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
	memset(kbOldState, 0, sizeof(kbOldState));

	memset(mState, 0, sizeof(mState));
	memset(mOldState, 0, sizeof(mOldState));
}

void Input::Update()
{
	memcpy(kbOldState, kbState, sizeof(kbState));
	memcpy(mOldState, mState, sizeof(mState));

	oldPos = pos;
	pos = GetMousePos();
}

bool Input::GetKey(char asciiCode)
{
	return kbState[asciiCode];
}

bool Input::GetKeyPressed(char asciiCode)
{
	return !kbOldState[asciiCode] && kbState[asciiCode];
}

bool Input::GetKeyReleased(char asciiCode)
{
	return kbOldState[asciiCode] && !kbState[asciiCode];
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
	return mState[btn];
}

bool Input::GetMouseBtnPressed(char btn)
{
	return !mOldState[btn] && mState[btn];
}

bool Input::GetMouseBtnReleased(char btn)
{
	return mOldState[btn] && !mState[btn];
}

void Input::KeyCallback(GLFWwindow *window, int key, int scanCode, int action, int mods)
{
	// safety check
	if (key < 0 || key >= 400)
		printf("[Warning] Unrecognized key pressed(key: %d, scanCode: %d)\n", key, scanCode);
	else
		kbState[RemapKey(key)] = action == GLFW_PRESS || action == GLFW_REPEAT;
}

void Input::MouseCallback(GLFWwindow *window, int button, int action, int mods)
{
	if (button < 0 || button >= 8)
		printf("[Warning] Unrecognized mouse btn pressed(btn: %d)\n", button);
	else
		mState[button] = action == GLFW_PRESS;
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