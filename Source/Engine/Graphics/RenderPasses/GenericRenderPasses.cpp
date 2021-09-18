#include "Precomp.h"
#include "GenericRenderPasses.h"

#include <Graphics/Graphics.h>
#include <Graphics/GPUResource.h>
#include <Graphics/Resources/GPUModel.h>

#include "Graphics/NamedFrameBuffers.h"
#include "Terrain.h"

DefaultRenderPass::DefaultRenderPass()
{
	myDependencies.push_back(TerrainRenderPass::kId);
}

bool DefaultRenderPass::HasResources(const RenderJob& aJob) const
{
	constexpr auto CheckResource = [](const Handle<GPUResource>& aRes) {
		return aRes.IsValid() && aRes->GetState() == GPUResource::State::Valid;
	};
	const RenderJob::TextureSet& textures = aJob.GetTextures();
	return CheckResource(aJob.GetModel())
		&& CheckResource(aJob.GetPipeline())
		&& std::all_of(textures.begin(), textures.end(), CheckResource);
}

void DefaultRenderPass::PrepareContext(RenderContext& aContext, Graphics& aGraphics) const
{
	aContext.myFrameBuffer = DefaultFrameBuffer::kName;

	aContext.myViewportSize[0] = static_cast<int>(aGraphics.GetWidth());
	aContext.myViewportSize[1] = static_cast<int>(aGraphics.GetHeight());

	aContext.myTexturesToActivate[0] = 0;

	aContext.myEnableCulling = true;
	aContext.myEnableDepthTest = true;
}

void DefaultRenderPass::Process(RenderJob& aJob, const IParams& aParams) const
{
	RenderJob::IndexedDrawParams drawParams;
	drawParams.myOffset = aParams.myOffset;
	const bool hasValidCount = aParams.myCount != uint32_t(-1);
	drawParams.myCount = hasValidCount ? aParams.myCount 
										: aJob.GetModel().Get<GPUModel>()->GetPrimitiveCount();
	aJob.SetDrawParams(drawParams);
}

bool TerrainRenderPass::HasResources(const RenderJob& aJob) const
{
	constexpr auto CheckResource = [](const Handle<GPUResource>& aRes) {
		return aRes.IsValid() && aRes->GetState() == GPUResource::State::Valid;
	};
	const RenderJob::TextureSet& textures = aJob.GetTextures();
	return CheckResource(aJob.GetPipeline())
		&& std::all_of(textures.begin(), textures.end(), CheckResource);
}

void TerrainRenderPass::PrepareContext(RenderContext& aContext, Graphics& aGraphics) const
{
	aContext.myFrameBuffer = DefaultFrameBuffer::kName;

	aContext.myViewportSize[0] = static_cast<int>(aGraphics.GetWidth());
	aContext.myViewportSize[1] = static_cast<int>(aGraphics.GetHeight());

	aContext.myShouldClearColor = true;
	aContext.myShouldClearDepth = true;

	aContext.myTexturesToActivate[0] = 0;

	aContext.myPolygonMode = aGraphics.IsWireframeActive() ? 
		RenderContext::PolygonMode::Line : RenderContext::PolygonMode::Fill;

	aContext.myEnableCulling = true;
	aContext.myEnableDepthTest = true;
}

void TerrainRenderPass::Process(RenderJob& aJob, const IParams& aParams) const
{
	const TerrainRenderParams& params = static_cast<const TerrainRenderParams&>(aParams);
	RenderJob::TesselationDrawParams drawParams;
	drawParams.myInstanceCount = params.myTileCount;
	aJob.SetDrawParams(drawParams);
}