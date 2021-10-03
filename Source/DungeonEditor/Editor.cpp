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
	ProcessInput();
	Draw();

	Graphics& graphics = *myGame.GetGraphics();
	const float width = graphics.GetWidth();
	const float height = graphics.GetHeight();
	const float scaledWidth = myScale * width;
	const float scaledHeight = myScale * height;
	myCamera.Recalculate(scaledWidth, scaledHeight);

	const glm::mat4 invProj = glm::inverse(myCamera.GetProj());
	const glm::vec2 mousePosScreenCoords = glm::vec2(
		myMousePos.x,
		height - (myMousePos.y)
	);
	const glm::vec2 mouseRelPos = mousePosScreenCoords / glm::vec2(width, height);
	const glm::vec2 normMouseRelPos = mouseRelPos * 2.f - glm::vec2(1.f);
	const glm::vec2 adjustedMousePos = invProj * glm::vec4(normMouseRelPos.x, normMouseRelPos.y, 0, 1);
	const glm::vec2 offset = myCamera.GetTransform().GetPos();
	PaintParams params{
		myCamera,
		myColor,
		myTexSize,
		adjustedMousePos + glm::vec2(offset.x, offset.y),
		myGridDims,
		myPaintMode,
		myBrushRadius
	};
	graphics.GetRenderPass<PaintingRenderPass>()->SetParams(params);
	graphics.GetRenderPass<DisplayRenderPass>()->SetParams(params);
}

void Editor::ProcessInput()
{
	myMousePos = Input::GetMousePos();

	myPaintMode = 0; // nothing
	if (!ImGui::GetIO().WantCaptureMouse && !ImGui::GetIO().WantCaptureKeyboard)
	{
		if (Input::GetMouseBtn(0))
		{
			myPaintMode = 1; // paint
		}
		else if (Input::GetMouseBtn(1))
		{
			myPaintMode = 2; // erase under mouse
		}
		else if (Input::GetKeyPressed(Input::Keys::Space))
		{
			myPaintMode = -1; // erase all
		}

		if (Input::GetKey(Input::Keys::Control))
		{
			myScale += Input::GetMouseWheelDelta() / 100.f;
		}
		else
		{
			myBrushRadius += Input::GetMouseWheelDelta() / 300.f;
			myBrushRadius = glm::clamp(myBrushRadius, 0.00001f, 0.2f);
		}

		if (Input::GetMouseBtnPressed(2))
		{
			myDragStart = myMousePos;
		}

		if (Input::GetMouseBtn(2))
		{
			const glm::vec2 deltaDrag = myMousePos - myDragStart;
			myDragStart = myMousePos;
			Transform& transf = myCamera.GetTransform();
			transf.SetPos(transf.GetPos() + glm::vec3(-deltaDrag.x, deltaDrag.y, 0));
		}
	}
}

void Editor::Draw()
{
	std::lock_guard lock(myGame.GetImGUISystem().GetMutex());
	if (ImGui::Begin("Editor"))
	{
		ImGui::TextWrapped("Paint with left mouse, erase with right mouse, "
			"clear all with Space and change brush size with mouse wheel/bellow. "
			"You can drag the canvas with middle mouse button, and zoom canvas with "
			"Ctrl + MWheel."
		);

		int size[2]{
			static_cast<int>(myTexSize.x),
			static_cast<int>(myTexSize.y)
		};
		ImGui::InputInt2("Tex Size", size);
		myTexSize = glm::vec2(size[0], size[1]);

		ImGui::SliderFloat("Brush size(MWheel)", &myBrushRadius, 0.00001f, 0.2f);

		ImGui::InputInt2("Grid Dims", glm::value_ptr(myGridDims));
		myGridDims.x = std::max(myGridDims.x, 1);
		myGridDims.y = std::max(myGridDims.y, 1);

		ImGui::SliderFloat("Canvas Scale(Ctrl + MWheel)", &myScale, 0.01f, 4.f);
		ImGui::ColorPicker3("Paint Color", glm::value_ptr(myColor));
	}
	ImGui::End();
}