#include "Precomp.h"
#include "Editor.h"

#include "PaintingRenderPass.h"

#include <Engine/Game.h>
#include <Engine/Input.h>
#include <Engine/Systems/ImGUI/ImGUISystem.h>

#include <Graphics/Resources/Texture.h>
#include <Graphics/Resources/GPUTexture.h>

#include <Core/Resources/AssetTracker.h>
#include <Core/File.h>
#include <filesystem>

Editor::Editor(Game& aGame)
	: myGame(aGame)
	, myPaintMode(PaintMode::None)
{
	const Graphics* graphics = myGame.GetGraphics();
	myTexSize = PaintingFrameBuffer::kDescriptor.mySize;

	myCamera = Camera(
		graphics->GetWidth(),
		graphics->GetHeight(),
		true
	);
	Transform& transf = myCamera.GetTransform();
	transf.SetPos(glm::vec3(0, 0, 1));
	transf.LookAt(glm::vec3(0, 0, 0));

	myMousePos = Input::GetMousePos();
	myPrevMousePos = myMousePos;
}

void Editor::Run()
{
	Profiler::ScopedMark mark("Editor::Run");
	ProcessInput();
	Draw();

	Graphics& graphics = *myGame.GetGraphics();
	const float width = graphics.GetWidth();
	const float height = graphics.GetHeight();
	const float scaledWidth = myScale * width;
	const float scaledHeight = myScale * height;
	myCamera.Recalculate(scaledWidth, scaledHeight);

	const glm::mat4 invProj = glm::inverse(myCamera.Get());
	auto ProjectMouse = [&](glm::vec2 aMousePos)
	{
		const glm::vec2 mousePosScreenCoords = glm::vec2(
			aMousePos.x,
			height - (aMousePos.y)
		);
		const glm::vec2 mouseRelPos = mousePosScreenCoords / glm::vec2(width, height);
		const glm::vec2 normMouseRelPos = mouseRelPos * 2.f - glm::vec2(1.f);
		const glm::vec2 adjustedMousePos = invProj * glm::vec4(normMouseRelPos.x, normMouseRelPos.y, 0, 1);
		return adjustedMousePos;
	};
	
	PaintParams params{
		myPaintTexture,
		myCamera,
		myColor,
		myTexSize,
		ProjectMouse(myPrevMousePos),
		ProjectMouse(myMousePos),
		myGridDims,
		myPaintMode,
		myBrushRadius,
		myInverseScale
	};
	graphics.GetRenderPass<PaintingRenderPass>()->SetParams(params);
	graphics.GetRenderPass<DisplayRenderPass>()->SetParams(params);
}

void Editor::ProcessInput()
{
	myPrevMousePos = myMousePos;
	myMousePos = Input::GetMousePos();

	myPaintMode = PaintMode::None;
	if (!ImGui::GetIO().WantCaptureMouse && !ImGui::GetIO().WantCaptureKeyboard)
	{
		if (Input::GetMouseBtn(0))
		{
			myPaintMode = myPaintingColor ? 
				PaintMode::PaintColor : PaintMode::PaintTexture;
		}
		else if (Input::GetMouseBtn(1))
		{
			myPaintMode = PaintMode::ErasePtr;
		}
		else if (Input::GetKeyPressed(Input::Keys::Space))
		{
			myPaintMode = PaintMode::EraseAll;
		}

		if (Input::GetKey(Input::Keys::Control))
		{
			myScale -= Input::GetMouseWheelDelta() / 100.f;
		}
		else
		{
			myBrushRadius += static_cast<int>(Input::GetMouseWheelDelta());
			myBrushRadius = glm::clamp(myBrushRadius, 1, 100);
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
	{
		std::lock_guard lock(myGame.GetImGUISystem().GetMutex());
		if (ImGui::Begin("Editor"))
		{
			DrawGeneralSettings();
			ImGui::Separator();
			DrawPaintSettings();
		}
		ImGui::End();

		if (myTexturesWindowOpen)
		{
			DrawTextures();
		}
	}
}

void Editor::DrawGeneralSettings()
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

	ImGui::SliderInt("Brush size(MWheel)", &myBrushRadius, 1, 100);

	ImGui::InputInt2("Grid Dims", glm::value_ptr(myGridDims));
	myGridDims.x = std::max(myGridDims.x, 1);
	myGridDims.y = std::max(myGridDims.y, 1);

	ImGui::SliderFloat("Canvas Scale(Ctrl + MWheel)", &myScale, 0.01f, 4.f);
}

void Editor::DrawPaintSettings()
{
	ImGui::Checkbox("Paint With Color?", &myPaintingColor);
	if (myPaintingColor)
	{
		ImGui::ColorPicker3("Paint Color", glm::value_ptr(myColor));
		if (myPaintTexture.IsValid())
		{
			myPaintTexture = {};
			myTexturesPath = "";
			myTexturesWindowOpen = false;
			myTextures.clear();
		}
	}
	else
	{
		bool changed = ImGui::InputText("Paint Texture", myTexturesPath.data(), myTexturesPath.capacity() + 1, ImGuiInputTextFlags_CallbackResize,
			[](ImGuiInputTextCallbackData* aData)
		{
			std::string* valueStr = static_cast<std::string*>(aData->UserData);
			if (aData->EventFlag == ImGuiInputTextFlags_CallbackResize)
			{
				valueStr->resize(aData->BufTextLen);
				aData->Buf = valueStr->data();
			}
			return 0;
		}, & myTexturesPath);

		if (changed && !myTexturesPath.empty())
		{
			std::string_view path = myTexturesPath;
			if (path.starts_with('"') && path.ends_with('"'))
			{
				path = path.substr(1, path.size() - 2);
			}

			LoadTextures(path);
			myTexturesWindowOpen = true;
		}
		
		if (!myTexturesPath.empty() && !myTexturesWindowOpen)
		{
			if (ImGui::Button("Open Textures Window"))
			{
				myTexturesWindowOpen = true;
			}
		}

		ImGui::DragFloat("Inverse Scale", &myInverseScale, 0.1f, 0.5f, 4.f);
		ImGui::Text("Current Texture");
		if (myPaintTexture.IsValid())
		{
			const glm::vec2 size = glm::min(glm::vec2(256), myPaintTexture->GetSize());
			myGame.GetImGUISystem().Image(myPaintTexture, size);
		}
		else
		{
			ImGui::Text("None");
		}
	}
}

void Editor::LoadTextures(std::string_view aPath)
{
	// acts as a cancellation token
	uint64_t initLoadVal = myLoadReqCounter.fetch_add(1) + 1;

	std::filesystem::path path = aPath;
	const uint64_t totalWorkItems = [&] {
		std::error_code errorCode;
		std::filesystem::directory_iterator dirIter(path, errorCode);
		ASSERT(!errorCode);

		return std::count_if(dirIter, {}, [](const std::filesystem::directory_entry& aEntry) {
			return aEntry.is_regular_file();
		});
	}();
	myTotalTextures = totalWorkItems;
	myWorkItemCounter = 0;

	{
		std::lock_guard lock(myTexturesMutex);
		myTextures.clear();
	}

	std::error_code errorCode;
	std::filesystem::directory_iterator dirIter(path, errorCode);
	ASSERT(!errorCode);

	for (const auto& entry : dirIter)
	{
		if (!entry.is_regular_file())
		{
			continue;
		}

		std::string utf8Path = entry.path().string();
		std::string fileName = entry.path().filename().string();
		myTaskArena.enqueue([=, this] {
			Profiler::ScopedMark mark("Editor::LoadTextureTask");
			Handle<Texture> texture = Texture::LoadFromDisk(utf8Path);
			if (texture->GetState() == Resource::State::Error)
			{
				myTotalTextures--;
				return;
			}
			Handle<GPUTexture> gpuTexture = myGame.GetGraphics()->GetOrCreate(texture).Get<GPUTexture>();
			if (myLoadReqCounter == initLoadVal)
			{
				std::lock_guard lock(myTexturesMutex);
				myTextures.emplace(fileName, gpuTexture);

				myWorkItemCounter++;
			}
		});
	}
}

void Editor::DrawTextures()
{
	constexpr glm::vec2 kSize(64);
	if (ImGui::Begin("Textures", &myTexturesWindowOpen))
	{
		ImGui::LabelText("Path", myTexturesPath.c_str());
		ImGui::LabelText("Loaded", "%llu/%llu", 
			myWorkItemCounter.load(),
			myTotalTextures.load());

		const float offset = ImGui::GetStyle().ItemSpacing.x;
		const float width = ImGui::GetContentRegionAvail().x;
		const int rowElemCount = glm::max(static_cast<int>(width / (kSize.x + offset)), 1);
		
		std::lock_guard texturesMutex(myTexturesMutex);
		int i = 0;
		for (const auto& [filename, texture] : myTextures)
		{
			if((i++) % rowElemCount != 0)
			{
				ImGui::SameLine();
			}
			myGame.GetImGUISystem().Image(texture, kSize);
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip(filename.c_str());
				if (ImGui::IsItemClicked())
				{
					myPaintTexture = texture;
				}
			}
		}
	}
	ImGui::End();
}