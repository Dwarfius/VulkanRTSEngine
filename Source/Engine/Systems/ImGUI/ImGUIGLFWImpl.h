#pragma once

class Game;
struct GLFWcursor;
struct GLFWwindow;

class ImGUIGLFWImpl
{
public:
	ImGUIGLFWImpl(Game& aGame);

	void Init(GLFWwindow& aWindow);
	void Shutdown();
	void NewFrame(float aDeltaTime);

private:
	void UpdateInput() const;
	void UpdateMouseCursor() const;

	constexpr static int kCursorCount = 9; // ImGuiMouseCursor_COUNT
	GLFWcursor* myCursors[kCursorCount];
	Game& myGame;
};