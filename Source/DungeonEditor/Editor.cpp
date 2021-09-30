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
	const Graphics* graphics = myGame.GetGraphics();
	myTexSize = glm::vec2(
		graphics->GetWidth(),
		graphics->GetHeight()
	);

	myCamera = Camera(
		graphics->GetWidth(),
		graphics->GetHeight(),
		true
	);
	Transform& transf = myCamera.GetTransform();
	transf.SetPos(glm::vec3(0, 0, 1));
	transf.LookAt(glm::vec3(0, 0, 0));

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
		myBrushRadius = glm::clamp(myBrushRadius, 0.00001f, 0.2f);
	}

	Draw();

	Graphics& graphics = *myGame.GetGraphics();
	myCamera.Recalculate(graphics.GetWidth(), graphics.GetHeight());

	PaintingRenderPass::Params params{
		myCamera,
		myTexSize,
		myMousePos,
		myGridDims,
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

		ImGui::InputInt2("Grid Dims", glm::value_ptr(myGridDims));
		myGridDims.x = std::max(myGridDims.x, 1);
		myGridDims.y = std::max(myGridDims.y, 1);
	}
	ImGui::End();
}