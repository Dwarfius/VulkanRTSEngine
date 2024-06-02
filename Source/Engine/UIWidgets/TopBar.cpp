#include "Precomp.h"
#include "TopBar.h"

#include "../Game.h"
#include "../Input.h"
#include "../Systems/ImGUI/ImGUISystem.h"

TopBar::TopBar()
{
	// we always have an empty root node
	myMenuNodes.emplace_back();
}

void TopBar::Register(std::string_view aPath, DrawCallback aCallback)
{
	myMenuItems.push_back({ GetMenuName(aPath), aCallback, false });
	InsertMenuNode(0, aPath);
}

void TopBar::Draw()
{
	if (Input::GetKeyPressed(Input::Keys::F1))
	{
		myIsVisible = !myIsVisible;
	}

	if (!myIsVisible || myMenuNodes.empty())
	{
		return;
	}

	std::lock_guard lock(Game::GetInstance()->GetImGUISystem().GetMutex());
	if (ImGui::BeginMainMenuBar())
	{
		for (uint16_t childInd : myMenuNodes[0].myChildren)
		{
			DrawMenuNode(myMenuNodes[childInd]);
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

void TopBar::InsertMenuNode(uint16_t aRootInd, std::string_view aPath)
{
	constexpr char kSeparator = '/';
	const size_t pos = aPath.find(kSeparator);
	if (pos != std::string_view::npos)
	{
		std::string_view pathNode = aPath.substr(0, pos);
		bool wasInserted = false;
		for (uint16_t childInd : myMenuNodes[aRootInd].myChildren)
		{
			if (myMenuNodes[childInd].myName == pathNode)
			{
				InsertMenuNode(childInd, aPath.substr(pos + 1)); // skip '/'
				wasInserted = true;
				break;
			}
		}
		if (!wasInserted)
		{
			const uint16_t newNodeInd = static_cast<uint16_t>(myMenuNodes.size());
			myMenuNodes.push_back({ std::string(pathNode.data(), pathNode.size()) });
			myMenuNodes[aRootInd].myChildren.push_back(newNodeInd);
			InsertMenuNode(newNodeInd, aPath.substr(pos + 1)); // skip '/'
		}
	}
	else
	{
		myMenuNodes[aRootInd].myItems.push_back(static_cast<uint16_t>(myMenuItems.size() - 1));
	}
}

void TopBar::DrawMenuNode(const MenuNode& aNode)
{
	if (ImGui::BeginMenu(aNode.myName.c_str()))
	{
		for (uint16_t childNode : aNode.myChildren)
		{
			DrawMenuNode(myMenuNodes[childNode]);
		}

		for (uint16_t menuItem : aNode.myItems)
		{
			if (ImGui::MenuItem(myMenuItems[menuItem].myName.data()))
			{
				myMenuItems[menuItem].myIsVisible = true;
			}
		}
		ImGui::EndMenu();
	}
}