#include "Precomp.h"
#include "EntitiesWidget.h"

#include "Game.h"
#include "Systems/ImGUI/ImGUISystem.h"
#include "Resources/ImGUISerializer.h"

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

void EntitiesWidget::DrawDialog(Game& aGame)
{
	tbb::mutex::scoped_lock imguiLock(aGame.GetImGUISystem().GetMutex());
	ImGui::Begin("Entities");
	Draw(aGame);
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