#pragma once

struct GLFWwindow;

class Input
{
public:
	static void SetWindow(GLFWwindow *w);
	static void Update();
	static void PostUpdate();

	// is the key down
	static bool GetKey(char asciiCode);
	// was it just pressed this frame
	static bool GetKeyPressed(char asciiCode);

	static vec2 GetMousePos();
	static vec2 GetMouseDelta();

	// is the btn down
	static bool GetMouseBtn(char btn);
	// was the btn just pressed this frame
	static bool GetMouseBtnPressed(char btn);

private:
	static GLFWwindow *window;

	static const int keyCount = 400;
	static char kbState[keyCount];
	static const int mButtonCount = 8;
	static char mState[mButtonCount];
	static vec2 oldPos, pos;

	static void KeyCallback(GLFWwindow *window, int key, int scanCode, int action, int mods);
	static void MouseCallback(GLFWwindow *window, int button, int action, int mods);
	static int RemapKey(int key);
};