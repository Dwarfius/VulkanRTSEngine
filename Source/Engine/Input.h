#pragma once

struct GLFWwindow;

// TODO: will need to transition from ascii codes to enum vals for interface
class Input
{
public:
	static void SetWindow(GLFWwindow* aWindow);
	static void Update();
	static void PostUpdate();

	// is the key down
	static bool GetKey(char anAsciiCode);
	// was it just pressed this frame
	static bool GetKeyPressed(char anAsciiCode);

	static glm::vec2 GetMousePos();
	static glm::vec2 GetMouseDelta();

	// is the btn down
	static bool GetMouseBtn(char aBtn);
	// was the btn just pressed this frame
	static bool GetMouseBtnPressed(char aBtn);

private:
	static GLFWwindow* ourWindow;

	static const int ourKeyCount = 400;
	static char ourKbState[ourKeyCount];
	static const int ourMButtonCount = 8;
	static char ourMState[ourMButtonCount];
	static glm::vec2 ourOldPos, ourPos;

	static void KeyCallback(GLFWwindow* aWindow, int aKey, int aScanCode, int anAction, int aMods);
	static void MouseCallback(GLFWwindow* aWindow, int aButton, int anAction, int aMods);
	static int RemapKey(int aKey);
};