#ifndef _INPUT_H
#define _INPUT_H

#include "Common.h"

struct GLFWwindow;

class Input
{
public:
	static void SetWindow(GLFWwindow *w);
	static void Update();

	// is the key down
	static bool GetKey(char asciiCode);
	// was it just pressed this frame
	static bool GetKeyPressed(char asciiCode);
	// was it just released this frame
	static bool GetKeyReleased(char asciiCode);

	static vec2 GetMousePos();
	static vec2 GetMouseDelta();

	// is the btn down
	static bool GetMouseBtn(char btn);
	// was the btn just pressed this frame
	static bool GetMouseBtnPressed(char btn);
	// was the btn just released this frame
	static bool GetMouseBtnReleased(char btn);

private:
	static GLFWwindow *window;

	static bool kbState[400], kbOldState[400];
	static bool mState[8], mOldState[8];
	static vec2 oldPos, pos;

	static void KeyCallback(GLFWwindow *window, int key, int scanCode, int action, int mods);
	static void MouseCallback(GLFWwindow *window, int button, int action, int mods);
	static int RemapKey(int key);
};

#endif