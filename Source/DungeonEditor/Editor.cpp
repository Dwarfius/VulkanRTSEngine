#include "Precomp.h"
#include "Editor.h"

#include "PaintingRenderPass.h"

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
	myMousePos = Input::GetMousePos();

	const bool ignoreMouse = ImGui::GetIO().WantCaptureMouse;

	myPaintMode = 0; // nothing
	if (!ignoreMouse && Input::GetMouseBtn(0))
	{
		myPaintMode = 1; // paint
	}
	else if (!ignoreMouse && Input::GetMouseBtn(1))
	{
		myPaintMode = 2; // erase under mouse
	}
	else if (Input::GetKeyPressed(Input::Keys::Space))
	{
		myPaintMode = -1; // erase all
	}

	if (!ignoreMouse)
	{
		myBrushRadius += Input::GetMouseWheelDelta() / 300.f;
		myBrushRadius = glm::clamp(myBrushRadius, 0.f, 1.f);
	}

	Draw();

	Graphics& graphics = *myGame.GetGraphics();
	PaintingRenderPass::Params params{
		myTexSize,
		myMousePos,
		myPaintMode,
		myBrushRadius
	};
	graphics.GetRenderPass<PaintingRenderPass>()->SetParams(params);
}

void Editor::Draw()
{
	std::lock_guard lock(myGame.GetImGUISystem().GetMutex());
	if (ImGui::Begin("Editor"))
	{
		ImGui::TextWrapped("Paint with left mouse, erase with right mouse, "
			"clear all with Space and change brush size with mouse wheel/bellow."
		);

		int size[2]{
			static_cast<int>(myTexSize.x),
			static_cast<int>(myTexSize.y)
		};
		ImGui::InputInt2("Tex Size", size);
		myTexSize = glm::vec2(size[0], size[1]);

		ImGui::SliderFloat("Brush size", &myBrushRadius, 0.00001f, 0.2f);
	}
	ImGui::End();
}