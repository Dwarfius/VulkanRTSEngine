#include "Precomp.h"
#include "ImGUIRendering.h"

#include <Graphics/Graphics.h>
#include <Graphics/Resources/GPUPipeline.h>
#include <Graphics/Resources/GPUModel.h>
#include <Graphics/Resources/GPUTexture.h>
#include <Graphics/Resources/UniformBuffer.h>

#include <Core/Profiler.h>

#include "Graphics/NamedFrameBuffers.h"
#include "Graphics/RenderPasses/DebugRenderPass.h"

void ImGUIAdapter::FillUniformBlock(const AdapterSourceData& aData, UniformBlock& aUB)
{
	const ImGUIData& data = static_cast<const ImGUIData&>(aData);
	aUB.SetUniform(0, 0, data.myOrthoProj);
}

ImGUIRenderPass::ImGUIRenderPass(Handle<Pipeline> aPipeline, Handle<Texture> aFontAtlas, Graphics& aGraphics)
	: myDestFrameBuffer(DefaultFrameBuffer::kName)
{
	AddDependency(DebugRenderPass::kId);

	aPipeline->ExecLambdaOnLoad([&](const Resource* aRes) {
		const Pipeline* pipeline = static_cast<const Pipeline*>(aRes);
		ASSERT_STR(pipeline->GetAdapterCount() == 1,
			"Only supporting 1 adapter! Please update if changed!");
		const Descriptor& descriptor = pipeline->GetAdapter(0).GetDescriptor();
		myUniformBuffer = aGraphics.CreateUniformBuffer(descriptor.GetBlockSize());
	});
	myPipeline = aGraphics.GetOrCreate(aPipeline).Get<GPUPipeline>();
	Handle<Model> model = new Model(
		Model::PrimitiveType::Triangles, 
		std::span<ImGUIVertex>{},
		true
	);
	myModel = aGraphics.GetOrCreate(model, true, GPUResource::UsageType::Dynamic).Get<GPUModel>();
	myFontAtlas = aGraphics.GetOrCreate(aFontAtlas).Get<GPUTexture>();
}

bool ImGUIRenderPass::IsReady() const
{
	return myPipeline->GetState() == GPUResource::State::Valid
		&& myFontAtlas->GetState() == GPUResource::State::Valid
		&& (myModel->GetState() == GPUResource::State::Valid
			|| myModel->GetState() == GPUResource::State::PendingUpload)
		&& myUniformBuffer->GetState() == GPUResource::State::Valid;
}

void ImGUIRenderPass::OnPrepareContext(RenderContext& aContext, Graphics& aGraphics) const
{
	aContext.myFrameBuffer = myDestFrameBuffer;

	aContext.myEnableBlending = true;
	aContext.myBlendingEq = RenderContext::BlendingEq::Add;
	aContext.mySourceBlending = RenderContext::Blending::SourceAlpha;
	aContext.myDestinationBlending = RenderContext::Blending::OneMinusSourceAlpha;
	aContext.myScissorMode = RenderContext::ScissorMode::PerObject;
	aContext.myEnableCulling = false;
	aContext.myEnableDepthTest = false;
	aContext.myShouldClearColor = false;
	aContext.myShouldClearDepth = false;
	aContext.myViewportSize[0] = static_cast<int>(aGraphics.GetWidth());
	aContext.myViewportSize[1] = static_cast<int>(aGraphics.GetHeight());
	aContext.myTexturesToActivate[0] = 0;
}

// We're using BeginPass to generate all work and schedule updates of assets (model)
void ImGUIRenderPass::BeginPass(Graphics& aGraphics)
{
	Profiler::ScopedMark mark("ImGUIRenderPass::BeginPass");

	RenderPass::BeginPass(aGraphics);
	ImGui::Render();

	if (!IsReady())
	{
		// required GPU resources not created yet, so skip this frame
		return;
	}

	ImGUIFrame frame = PrepareFrame();
	if (frame.myParams.empty())
	{
		return;
	}

	UniformBlock block(*myUniformBuffer.Get(), myPipeline->GetAdapter(0).GetDescriptor());
	block.SetUniform(0, 0, frame.myMatrix);

	for (Params& params : frame.myParams)
	{
		GPUTexture* texture = myFontAtlas.Get();
		if (params.myTexture.IsValid())
		{
			texture = params.myTexture.Get();
		}

		RenderJob& job = AllocateJob();
		job.SetPipeline(myPipeline.Get());
		job.SetModel(myModel.Get());
		job.GetTextures().PushBack(texture);
		job.GetUniformSet().PushBack(myUniformBuffer.Get());

		for (uint8_t i = 0; i < 4; i++)
		{
			job.SetScissorRect(i, params.myScissorRect[i]);
		}

		RenderJob::IndexedDrawParams drawParams;
		drawParams.myOffset = params.myOffset;
		drawParams.myCount = params.myCount;
		job.SetDrawParams(drawParams);
	}

	if (frame.myDesc.myIndCount > 0)
	{
		myModel->GetResource().Get<Model>()->Update(frame.myDesc);
		myModel->UpdateRegion({});
	}
}

// Precondition: ImGui::Render() must've already been called
ImGUIRenderPass::ImGUIFrame ImGUIRenderPass::PrepareFrame()
{
	const ImDrawData* drawData = ImGui::GetDrawData();
	// check if we got any draw commands
	if (!drawData->CmdListsCount)
	{
		return {};
	}

	// Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
	const int fb_width = (int)(drawData->DisplaySize.x * drawData->FramebufferScale.x);
	const int fb_height = (int)(drawData->DisplaySize.y * drawData->FramebufferScale.y);
	if (fb_width <= 0 || fb_height <= 0)
	{
		return {};
	}

	using UploadDesc = Model::UploadDescriptor<ImGUIVertex>;
	UploadDesc uploadDesc;

	for (int i = 0; i < drawData->CmdListsCount; i++)
	{
		const ImDrawList* cmdList = drawData->CmdLists[i];
		uploadDesc.myVertCount += cmdList->VtxBuffer.Size;
		uploadDesc.myIndCount += cmdList->IdxBuffer.Size;
	}

	ImGUIVertex* vertBuffer = new ImGUIVertex[uploadDesc.myVertCount];
	Model::IndexType* indBuffer = new Model::IndexType[uploadDesc.myIndCount];

	std::vector<ImGUIRenderPass::Params> renderParams;
	uint32_t vertOffset = 0;
	uint32_t indOffset = 0;
	// Will project scissor/clipping rectangles into framebuffer space
	const ImVec2 clip_off = drawData->DisplayPos;         // (0,0) unless using multi-viewports
	const ImVec2 clip_scale = drawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)
	for (int cmdListInd = 0; cmdListInd < drawData->CmdListsCount; cmdListInd++)
	{
		const ImDrawList* cmdList = drawData->CmdLists[cmdListInd];

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

		for (int cmdInd = 0; cmdInd < cmdList->CmdBuffer.Size; cmdInd++)
		{
			const ImDrawCmd* cmd = &cmdList->CmdBuffer[cmdInd];

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
				ImGUIRenderPass::Params params;
				params.myCount = cmd->ElemCount;
				params.myOffset = indOffset + cmd->IdxOffset;
				params.myScissorRect[0] = (int)clip_rect.x;
				params.myScissorRect[1] = (int)(fb_height - clip_rect.w);
				params.myScissorRect[2] = (int)(clip_rect.z - clip_rect.x);
				params.myScissorRect[3] = (int)(clip_rect.w - clip_rect.y);
				if (cmd->TextureId)
				{
					params.myTexture = Handle<GPUTexture>(static_cast<GPUTexture*>(cmd->TextureId));
				}

				renderParams.push_back(params);
			}
		}

		vertOffset += cmdList->VtxBuffer.Size;
		indOffset += cmdList->IdxBuffer.Size;
	}
	uploadDesc.myVertsOwned = true;
	uploadDesc.myVertices = vertBuffer;

	uploadDesc.myIndOwned = true;
	uploadDesc.myIndices = indBuffer;

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
	return { 
		.myDesc = uploadDesc, 
		.myMatrix = orthoProj,
		.myParams = std::move(renderParams)
	};
}