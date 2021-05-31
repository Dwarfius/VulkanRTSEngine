#pragma once

class TopBar
{
public:
	using Callback = std::function<void()>;

	void Register(std::string_view aPath, Callback aCallback);
	void Unregister(std::string_view aPath);

	void Draw();

private:
	struct MenuItem
	{
		// Until ImGui resolves https://github.com/ocornut/imgui/issues/494
		// we have to use double heap for the menu instead of single view
		std::vector<std::string> myMenuStack;
		std::string_view myPath;
		Callback myCallback;
	};

	void DrawMenu(const MenuItem& anItem, char* aCurrentMenu, char& aMenuCount);

	static std::vector<std::string> GetMenuStack(std::string_view aPath);
	static std::string_view GetMenuName(std::string_view aPath);

	std::vector<MenuItem> myMenuItems;
	bool myIsVisible;
};