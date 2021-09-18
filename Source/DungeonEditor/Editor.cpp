#include "Precomp.h"
#include "Editor.h"

#include <Engine/Game.h>
#include <Engine/Input.h>
#include <Engine/Systems/ImGUI/ImGUISystem.h>

#include <Core/Resources/AssetTracker.h>

Editor::Editor(Game& aGame)
	: myGame(aGame)
{
	myTexSize = glm::vec2(
		myGame.GetGraphics()->GetWidth(), 
		myGame.GetGraphics()->GetHeight()
	);

	myMousePos = Input::GetMousePos();
}

void Editor::Run()
{
	Draw();
}

void Editor::Draw()
{
	std::lock_guard lock(myGame.GetImGUISystem().GetMutex());

	if (ImGui::Begin("Settings"))
	{

	}
	ImGui::End();
}