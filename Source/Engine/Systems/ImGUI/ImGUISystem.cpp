#include "Precomp.h"
#include "ImGUISystem.h"

#include "Game.h"
#include "Systems/ImGUI/ImGUIRendering.h"

#include <Graphics/UniformAdapterRegister.h>
#include <Graphics/Graphics.h>

#include <Core/Resources/AssetTracker.h>
#include <Core/Profiler.h>

ImGUISystem::ImGUISystem(Game& aGame)
	: myGame(aGame)
	, myGLFWImpl(aGame)
	, myRenderPass(nullptr)
{
}

void ImGUISystem::Init()
{
	Profiler::ScopedMark profile("ImGUISystem::Init");

	UniformAdapterRegister::GetInstance().Register<ImGUIAdapter>();

	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	myGLFWImpl.Init();

	Handle<Pipeline> imGUIPipeline = myGame.GetAssetTracker().GetOrCreate<Pipeline>("imgui.ppl");
	Handle<Texture> fontAtlas = new Texture();
	fontAtlas->SetMinFilter(ITexture::Filter::Linear);
	fontAtlas->SetMagFilter(ITexture::Filter::Linear);
	{
		ImGuiIO& io = ImGui::GetIO();
		unsigned char* pixels;
		int width, height;
		io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

		fontAtlas->SetWidth(width);
		fontAtlas->SetHeight(height);
		fontAtlas->SetFormat(ITexture::Format::SNorm_RGBA);
		fontAtlas->SetPixels(pixels, false);
	}

	myRenderPass = new ImGUIRenderPass(imGUIPipeline, fontAtlas, *myGame.GetGraphics());
	myGame.GetGraphics()->AddRenderPass(myRenderPass);
}

void ImGUISystem::Shutdown()
{
	myGLFWImpl.Shutdown();
	ImGui::DestroyContext();
	delete myRenderPass;
}

void ImGUISystem::NewFrame(float aDeltaTime)
{
	myGLFWImpl.NewFrame(aDeltaTime);
	ImGui::NewFrame();
}

void ImGUISystem::Render()
{
	Profiler::ScopedMark imguiProfile("ImGui::Render");
	ImGui::Render();

	if (!myRenderPass->IsReady())
	{
		// required GPU resources not created yet, so skip this frame
		return;
	}

	const ImDrawData* drawData = ImGui::GetDrawData();
	// check if we got any draw commands
	if (!drawData->CmdListsCount)
	{
		return;
	}

	// Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
	const int fb_width = (int)(drawData->DisplaySize.x * drawData->FramebufferScale.x);
	const int fb_height = (int)(drawData->DisplaySize.y * drawData->FramebufferScale.y);
	if (fb_width <= 0 || fb_height <= 0)
	{
		return;
	}

	// Will project scissor/clipping rectangles into framebuffer space
	const ImVec2 clip_off = drawData->DisplayPos;         // (0,0) unless using multi-viewports
	const ImVec2 clip_scale = drawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

	{
		const float L = drawData->DisplayPos.x;
		const float R = drawData->DisplayPos.x + drawData->DisplaySize.x;
		const float T = drawData->DisplayPos.y;
		const float B = drawData->DisplayPos.y + drawData->DisplaySize.y;
		const glm::mat4 orthoProj{
			{ 2.0f / (R - L),   0.0f,         0.0f,   0.0f },
			{ 0.0f,         2.0f / (T - B),   0.0f,   0.0f },
			{ 0.0f,         0.0f,			-1.0f,   0.0f },
			{ (R + L) / (L - R),  (T + B) / (B - T),  0.0f,   1.0f },
		};
		myRenderPass->SetProj(orthoProj);
	}

	using UploadDesc = Model::UploadDescriptor<ImGUIVertex>;
	UploadDesc uploadDesc;
	std::memset(&uploadDesc, 0, sizeof(UploadDesc));

	for (int i = 0; i < drawData->CmdListsCount; i++)
	{
		const ImDrawList* cmdList = drawData->CmdLists[i];
		uploadDesc.myVertCount += cmdList->VtxBuffer.Size;
		uploadDesc.myIndCount += cmdList->IdxBuffer.Size;
	}

	ImGUIVertex* vertBuffer = new ImGUIVertex[uploadDesc.myVertCount];
	Model::IndexType* indBuffer = new Model::IndexType[uploadDesc.myIndCount];

	uint32_t vertOffset = 0;
	uint32_t indOffset = 0;
	for (int i = 0; i < drawData->CmdListsCount; i++)
	{
		const ImDrawList* cmdList = drawData->CmdLists[i];

		// copy verts
		static_assert(sizeof(ImGUIVertex) == sizeof(ImDrawVert), "Can no longer copy verts!");
		std::memcpy(vertBuffer + vertOffset, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));

		// copy indices
		static_assert(sizeof(Model::IndexType) == sizeof(ImDrawIdx), "Can no longer copy indices!");
		//std::memcpy(indBuffer + indOffset, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
		// So this is annoying - since indices are meant to be in separate buffers, we need to
		// manually shift them since we're putting it all into 1 buffer
		for (int i = 0; i < cmdList->IdxBuffer.Size; i++)
		{
			indBuffer[indOffset + i] = vertOffset + cmdList->IdxBuffer.Data[i];
		}

		for (int cmd_i = 0; cmd_i < cmdList->CmdBuffer.Size; cmd_i++)
		{
			const ImDrawCmd* cmd = &cmdList->CmdBuffer[cmd_i];

			// Project scissor/clipping rectangles into framebuffer space
			ImVec4 clip_rect;
			clip_rect.x = (cmd->ClipRect.x - clip_off.x) * clip_scale.x;
			clip_rect.y = (cmd->ClipRect.y - clip_off.y) * clip_scale.y;
			clip_rect.z = (cmd->ClipRect.z - clip_off.x) * clip_scale.x;
			clip_rect.w = (cmd->ClipRect.w - clip_off.y) * clip_scale.y;

			if (clip_rect.x < fb_width
				&& clip_rect.y < fb_height
				&& clip_rect.z >= 0.0f
				&& clip_rect.w >= 0.0f)
			{
				ImGUIRenderParams params;
				params.myCount = cmd->ElemCount;
				params.myOffset = indOffset + cmd->IdxOffset;
				params.myScissorRect[0] = (int)clip_rect.x;
				params.myScissorRect[1] = (int)(fb_height - clip_rect.w);
				params.myScissorRect[2] = (int)(clip_rect.z - clip_rect.x);
				params.myScissorRect[3] = (int)(clip_rect.w - clip_rect.y);

				myRenderPass->AddImGuiRenderJob(params);
			}
		}

		vertOffset += cmdList->VtxBuffer.Size;
		indOffset += cmdList->IdxBuffer.Size;
	}

	uploadDesc.myVertsOwned = true;
	uploadDesc.myVertices = vertBuffer;

	uploadDesc.myIndOwned = true;
	uploadDesc.myIndices = indBuffer;
	myRenderPass->UpdateImGuiVerts(uploadDesc);
}