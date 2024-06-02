#pragma once

class TopBar
{
public:
	using DrawCallback = std::function<void(bool&)>;

	TopBar();

	void Register(std::string_view aPath, DrawCallback aCallback);

	void Draw();

private:
	static std::string_view GetMenuName(std::string_view aPath);

	struct MenuItem
	{
		// Safe to use a view because all cases are string literals
		std::string_view myName;
		DrawCallback myDrawCallback;
		bool myIsVisible;
	};
	std::vector<MenuItem> myMenuItems;

	struct MenuNode
	{
		// Until ImGui resolves https://github.com/ocornut/imgui/issues/494
		// we have to use double heap for the menu instead of single view
		std::string myName;
		std::vector<uint16_t> myItems;
		std::vector<uint16_t> myChildren;
	};
	std::vector<MenuNode> myMenuNodes;

	void InsertMenuNode(uint16_t aRootInd, std::string_view aPath);
	void DrawMenuNode(const MenuNode& aNode);

	bool myIsVisible = false;
};