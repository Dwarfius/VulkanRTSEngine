#include "Precomp.h"
#include "Input.h"

GLFWwindow* Input::ourWindow = nullptr;
tbb::concurrent_queue<Input::InputMsg> Input::ourPendingMsgs;
char Input::ourKbState[kKeyCount];
char Input::ourMState[kMButtonCount];
glm::vec2 Input::ourOldPos, Input::ourPos;
float Input::ourMWheel = 0.f;

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
	glfwSetScrollCallback(ourWindow, Input::ScrollCallback);

	std::memset(ourKbState, 0, sizeof(ourKbState));
	std::memset(ourMState, 0, sizeof(ourMState));

	// Update them to the same value to avoid issues
	ourPos = ourOldPos = GetMousePos();
}

void Input::Update()
{
	ourOldPos = ourPos;
	ourPos = GetMousePos();

	InputMsg msg;
	while (ourPendingMsgs.try_pop(msg))
	{
		if (msg.myIsKeyboard)
		{
			// avoid resetting repeat state
			if (ourKbState[msg.myButton] != GLFW_REPEAT || msg.myNewState != GLFW_PRESS)
			{
				ourKbState[msg.myButton] = msg.myNewState;
			}
		}
		else
		{
			// avoid resetting repeat state
			if (ourMState[msg.myButton] != GLFW_REPEAT || msg.myNewState != GLFW_PRESS)
			{
				ourMState[msg.myButton] = msg.myNewState;
			}
		}
	}
}

// This needs to be called at the end of the update loop - it gets around the slow repeat rate of OS
// by automatically marking the button as repeated
void Input::PostUpdate()
{
	for (int i = 0; i < kKeyCount; i++)
	{
		if (ourKbState[i] == GLFW_PRESS)
		{
			ourKbState[i] = GLFW_REPEAT;
		}
	}
	for (int i = 0; i < kMButtonCount; i++)
	{
		if (ourMState[i] == GLFW_PRESS)
		{
			ourMState[i] = GLFW_REPEAT;
		}
	}
	ourMWheel = 0;
}

bool Input::GetKey(Keys aKey)
{
	uint8_t key = static_cast<uint8_t>(aKey);
	return ourKbState[key] == GLFW_PRESS || ourKbState[key] == GLFW_REPEAT;
}

bool Input::GetKeyPressed(Keys aKey)
{
	uint8_t key = static_cast<uint8_t>(aKey);
	return ourKbState[key] == GLFW_PRESS;
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

float Input::GetMouseWheelDelta()
{
	return ourMWheel;
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
	if (aKey == GLFW_KEY_WORLD_1 
		|| aKey == GLFW_KEY_WORLD_2 
		|| aKey == GLFW_KEY_UNKNOWN)
	{
		return;
	}

	uint8_t key = static_cast<uint8_t>(RemapKey(aKey));
	ourPendingMsgs.push(InputMsg{ key, static_cast<uint8_t>(anAction), true });
}

void Input::MouseCallback(GLFWwindow* aWindow, int aButton, int anAction, int aMods)
{
	ASSERT_STR(aButton >= 0 && aButton < kMButtonCount, "Unsupported button: %d", aButton);
	uint8_t button = static_cast<uint8_t>(aButton);
	ourPendingMsgs.push(InputMsg{ button, static_cast<uint8_t>(anAction), false });
}

void Input::ScrollCallback(GLFWwindow* aWidnow, double aXDelta, double aYDelta)
{
	static_cast<void>(aXDelta);

	ourMWheel = aYDelta;
}

Input::Keys Input::RemapKey(int aKey)
{
	ASSERT_STR(aKey != GLFW_KEY_WORLD_1
		&& aKey != GLFW_KEY_WORLD_2
		&& aKey != GLFW_KEY_UNKNOWN, "Can't handle this key!");
	ASSERT_STR(aKey <= GLFW_KEY_LAST, "Invalid key!");

	// ASCII setup
	uint8_t offset = 0;
	uint8_t start = static_cast<uint8_t>(aKey);
	if (aKey >= GLFW_KEY_RIGHT_SHIFT)
	{
		offset = aKey - GLFW_KEY_RIGHT_SHIFT;
		start = static_cast<uint8_t>(Keys::Shift);
	}
	else if (aKey >= GLFW_KEY_LEFT_SHIFT)
	{
		offset = aKey - GLFW_KEY_LEFT_SHIFT;
		start = static_cast<uint8_t>(Keys::Shift);
	}
	else if (aKey >= GLFW_KEY_KP_0)
	{
		offset = aKey - GLFW_KEY_KP_0;
		start = static_cast<uint8_t>(Keys::KeyPad0);
	}
	else if (aKey >= GLFW_KEY_F1)
	{
		offset = aKey - GLFW_KEY_F1;
		start = static_cast<uint8_t>(Keys::F1);
	}
	else if (aKey >= GLFW_KEY_CAPS_LOCK)
	{
		offset = aKey - GLFW_KEY_CAPS_LOCK;
		start = static_cast<uint8_t>(Keys::CapsLock);
	}
	else if (aKey >= GLFW_KEY_ESCAPE)
	{
		offset = aKey - GLFW_KEY_ESCAPE;
		start = static_cast<uint8_t>(Keys::Escape);
	}
	
	return static_cast<Keys>(start + offset);
}