#include "Precomp.h"
#include "EntitiesWidget.h"

#include "Game.h"
#include "Systems/ImGUI/ImGUISystem.h"
#include "Resources/ImGUISerializer.h"

#include <Core/Utils.h>

void EntitiesWidget::Draw(Game& aGame)
{
	aGame.ForEach([&](GameObject& aGo) {
		if (aGo.GetParent().IsValid())
		{
			// skip children objects - they'll be displayed as part of hierarchy
			return;
		}

		char uidBuffer[33];
		aGo.GetUID().GetString(uidBuffer);
		if (ImGui::TreeNode(uidBuffer))
		{
			DrawGameObject(aGo);
			DrawChildren(aGo);

			ImGui::TreePop();
		}
	});
}

void EntitiesWidget::DrawDialog(Game& aGame, bool& aIsOpen)
{
	std::lock_guard imguiLock(aGame.GetImGUISystem().GetMutex());
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