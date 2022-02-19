#include "Precomp.h"
#include "TopBar.h"

#include "../Game.h"
#include "../Input.h"
#include "../Systems/ImGUI/ImGUISystem.h"

void TopBar::Register(std::string_view aPath, DrawCallback aCallback)
{
	myMenuItems.push_back({ GetMenuStack(aPath), aPath, aCallback, false });
	std::sort(myMenuItems.begin(), myMenuItems.end(), [](const MenuItem& aLeft, const MenuItem& aRight) {
		return aLeft.myPath.compare(aRight.myPath) < 0;
	});
}

void TopBar::Unregister(std::string_view aPath)
{
	std::string_view item = GetMenuName(aPath);
	myMenuItems.erase(std::remove_if(myMenuItems.begin(), myMenuItems.end(), [&](const MenuItem& aMenu) {
		return aMenu.myPath == aPath;
	}));
}

void TopBar::Draw()
{
	if (Input::GetKeyPressed(Input::Keys::F1))
	{
		myIsVisible = !myIsVisible;
	}

	if (!myIsVisible)
	{
		std::lock_guard lock(Game::GetInstance()->GetImGUISystem().GetMutex());
		ImGui::SetNextWindowPos({ 0,0 });
		ImGui::Begin("Info");
		ImGui::Text("Press F1 to bring the Main Bar");
		ImGui::End();
		return;
	}

	std::lock_guard lock(Game::GetInstance()->GetImGUISystem().GetMutex());
	if (ImGui::BeginMainMenuBar())
	{
		constexpr size_t kMaxPath = 250;
		char currentMenu[kMaxPath]{};
		char menuCount = 0;
		for (MenuItem& item : myMenuItems)
		{
			if (DrawMenu(item, currentMenu, menuCount))
			{
				item.myIsVisible = true;
			}
		}
		while (menuCount)
		{
			ImGui::EndMenu();
			menuCount--;
		}
		ImGui::EndMainMenuBar();
	}

	for (MenuItem& item : myMenuItems)
	{
		if (item.myIsVisible)
		{
			item.myDrawCallback(item.myIsVisible);
		}
	}
}

bool TopBar::DrawMenu(const MenuItem& anItem, char* aCurrentMenu, char& aMenuCount) const
{
	// check if we need to start a new menu
	size_t offset = 0;
	char menusOpened = 0;
	bool foundMissmatch = false;
	for(char depth = 0; depth < anItem.myMenuStack.size(); depth++)
	{
		const std::string& menu = anItem.myMenuStack[depth];
		bool isOpen = false;
		if (!foundMissmatch)
		{
			foundMissmatch = std::memcmp(menu.data(), aCurrentMenu + offset, menu.size()) != 0;
			if (foundMissmatch)
			{
				// destroy all menu stack
				for (char menuInd = depth; menuInd < aMenuCount; menuInd++)
				{
					ImGui::EndMenu();
				}

				// push current menu on top - the rest will be handled on next iters
				isOpen = ImGui::BeginMenu(menu.c_str());
				if (isOpen)
				{
					std::memcpy(aCurrentMenu + offset, menu.data(), menu.size());
				}
				else
				{
					// user doesn't have the menu open
					// so there's no point in going deeper
					break;
				}
			}
			else
			{
				// there's a match for the menu stack,
				// so it's open from a previous menu item
				isOpen = true;
			}
		}
		else
		{
			// Last iteration we found a match, meaning we need
			// continue populating the hierarchy stack
			// and open relevant submenus
			isOpen = ImGui::BeginMenu(menu.c_str());
			if (isOpen)
			{
				std::memcpy(aCurrentMenu + offset, menu.data(), menu.size());
			}
			else
			{
				break;
			}
		}
		offset += menu.size();
		if (isOpen)
		{
			menusOpened++;
		}
	}
	aMenuCount = menusOpened;

	// draw out current item
	std::string_view name = GetMenuName(anItem.myPath);
	return aMenuCount == anItem.myMenuStack.size() && ImGui::MenuItem(name.data());
}

std::vector<std::string> TopBar::GetMenuStack(std::string_view aPath)
{
	constexpr char kSeparator = '/';
	std::vector<std::string> stack;
	size_t offset = 0;
	size_t pos;
	while ((pos = aPath.find(kSeparator, offset)) != std::string_view::npos)
	{
		std::string menu(&aPath[offset], &aPath[pos]);
		stack.push_back(std::move(menu));
		offset = pos + 1;
	}
	return stack;
}

std::string_view TopBar::GetMenuName(std::string_view aPath)
{
	constexpr char kSeparator = '/';
	const size_t pos = aPath.rfind(kSeparator);
	if (pos != std::string_view::npos)
	{
		return aPath.substr(pos + 1); // skip '/'
	}
	else
	{
		return std::string_view{};
	}
}