#include "Precomp.h"
#include "EntitiesWidget.h"

#include "Game.h"
#include "Systems/ImGUI/ImGUISystem.h"
#include "Resources/ImGUISerializer.h"

#include <Core/Utils.h>
#include <Core/Resources/AssetTracker.h>

void EntitiesWidget::Draw(Game& aGame)
{
	World& world = aGame.GetWorld();
	world.Access([&](std::vector<Handle<GameObject>>& aGameObjects) {
		ImGui::LabelText("Entities Count", "%llu", aGameObjects.size());
		if (!aGameObjects.empty())
		{
			ImGui::Text("Entities:");
		}
		for (Handle<GameObject>& gameObjectHandle : aGameObjects)
		{
			GameObject& gameObject = *gameObjectHandle.Get();
			if (gameObject.GetParent().IsValid())
			{
				// skip children objects - they'll be displayed as part of hierarchy
				return;
			}

			char uidBuffer[33];
			gameObject.GetUID().GetString(uidBuffer);
			
			char nodeBuffer[128];
			if (!gameObject.GetPath().empty())
			{
				char nameBuffer[64]{};
				const std::string_view name = gameObject.GetName();
				ASSERT_STR(name.size() < std::extent_v<decltype(nameBuffer)>, "Name too long!");
				std::copy(name.begin(), name.end(), std::begin(nameBuffer));
				Utils::StringFormat(nodeBuffer, "%s - (%s)", nameBuffer, uidBuffer);
			}
			else
			{
				std::copy(std::begin(uidBuffer), std::end(uidBuffer), std::begin(nodeBuffer));
			}

			if (ImGui::TreeNode(nodeBuffer))
			{
				DrawGameObject(gameObject);
				DrawChildren(gameObject);

				ImGui::TreePop();
			}
		}
	});
}

void EntitiesWidget::DrawDialog(Game& aGame, bool& aIsOpen)
{
	if (ImGui::Begin("Entities", &aIsOpen))
	{
		Draw(aGame);
	}
	ImGui::End();
}

void EntitiesWidget::DrawGameObject(GameObject& aGo)
{
	ImGUISerializer serializer(Game::GetInstance()->GetAssetTracker());
	aGo.Serialize(serializer);
}

void EntitiesWidget::DrawChildren(GameObject& aGo)
{
	size_t childCount = aGo.GetChildCount();
	for (size_t childInd = 0; childInd < childCount; childInd++)
	{
		char childBuff[15];
		Utils::StringFormat(childBuff, "Child %zu", childInd);
		if (ImGui::TreeNode(childBuff))
		{
			GameObject& childGo = aGo.GetChild(childInd);
			DrawGameObject(childGo);
			ImGui::TreePop();
		}
	}
}