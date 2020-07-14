#pragma once

struct GLFWwindow;

// TODO: will need to reimplement with or add a command pattern on top
class Input
{
public:
	enum class Keys : uint8_t;

	static void SetWindow(GLFWwindow* aWindow);
	static void Update();
	static void PostUpdate();

	// is the key down
	static bool GetKey(Keys aKey);
	// was it just pressed this frame
	static bool GetKeyPressed(Keys aKey);

	static glm::vec2 GetMousePos();
	static glm::vec2 GetMouseDelta();
	static float GetMouseWheelDelta();

	// returns a pointer to a buffer of input characters, 
	// value of 0 means end of valid range
	static const uint32_t* GetInputChars() { return ourInputChars; }

	// is the btn down
	static bool GetMouseBtn(char aBtn);
	// was the btn just pressed this frame
	static bool GetMouseBtnPressed(char aBtn);

private:
	static GLFWwindow* ourWindow;

	struct InputMsg
	{
		uint8_t myButton;
		uint8_t myNewState : 2;
		bool myIsKeyboard : 1;
	};
	static tbb::concurrent_queue<InputMsg> ourPendingMsgs;

	constexpr static int kMButtonCount = 8;
	static char ourMState[kMButtonCount];
	static glm::vec2 ourOldPos, ourPos;
	static float ourMWheel;

	constexpr static uint8_t kMaxInputChars = 8;
	// where we recieve from GLFW
	static uint32_t ourInputCharsBuffer[kMaxInputChars]; 
	// where frame-stable input chars are, 0 marking end of valid chars
	static uint32_t ourInputChars[kMaxInputChars]; 
	// current length of the input buffer
	static uint8_t ourInputCharsLength; 
	static tbb::spin_mutex ourInputCharsMutex;

	static void KeyCallback(GLFWwindow* aWindow, int aKey, int aScanCode, int anAction, int aMods);
	static void MouseCallback(GLFWwindow* aWindow, int aButton, int anAction, int aMods);
	static void ScrollCallback(GLFWwindow* aWindow, double aXDelta, double aYDelta);
	static void InputKeyCallback(GLFWwindow* aWindow, uint32_t aChar);
	static Keys RemapKey(int aKey);

public:
	// Tries to follow the ASCII table for printable keys,
	// while packing functional keys in free spaces
	enum class Keys : uint8_t
	{
		Escape,
		Enter,
		Tab,
		Backspace,
		Insert,
		Delete,
		Right,
		Left,
		Down,
		Up,
		PageUp,
		PageDown,
		Home,
		End,
		CapsLock,
		ScrollLock,
		NumLock,
		PrintScreen,
		Pause,
		Shift,
		Control,
		Alt,
		Super,
		Menu,
		Space = 32,
		Apostrophe = 39,  /* ' */
		Comma = 44,  /* , */
		Minus,   /* - */
		Preiod,  /* . */
		ForwardSlash,  /* / */
		Key0,
		Key1,
		Key2,
		Key3,
		Key4,
		Key5,
		Key6,
		Key7,
		Key8,
		Key9,
		Semicolon = 59,  /* ; */
		Equal = 61,  /* = */
		A = 65,
		B,
		C,
		D,
		E,
		F,
		G,
		H,
		I,
		J,
		K,
		L,
		M,
		N,
		O,
		P,
		Q,
		R,
		S,
		T,
		U,
		V,
		W,
		X,
		Y,
		Z,
		LeftBracker, /* [ */
		Backslash,  /* \ */
		RightBracket,  /* ] */
		Backtick = 96,  /* ` */
		F1,
		F2,
		F3,
		F4,
		F5,
		F6,
		F7,
		F8,
		F9,
		F10,
		F11,
		F12,
		F13,
		F14,
		F15,
		F16,
		F17,
		F18,
		F19,
		F20,
		F21,
		F22,
		F23,
		F24,
		F25,
		KeyPad0,
		KeyPad1,
		KeyPad2,
		KeyPad3,
		KeyPad4,
		KeyPad5,
		KeyPad6,
		KeyPad7,
		KeyPad8,
		KeyPad9,
		KeyPadDecimal,
		KeyPadDivide,
		KeyPadMultiply,
		KeyPadSubstract,
		KeyPadAdd,
		KeyPadEnter,
		KeyPadEqual,
		Count
	};

private:
	constexpr static uint8_t kKeyCount = static_cast<uint8_t>(Keys::Count);
	static char ourKbState[kKeyCount];
};