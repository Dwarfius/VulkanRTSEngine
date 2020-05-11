#include "Precomp.h"
#include "GenericRenderPasses.h"

#include <Graphics/Graphics.h>
#include <Graphics/GPUResource.h>

#include "../../Terrain.h"

bool DefaultRenderPass::HasResources(const RenderJob& aJob) const
{
	constexpr auto CheckResource = [](const Handle<GPUResource>& aRes) {
		return aRes.IsValid() && aRes->GetState() == GPUResource::State::Valid;
	};
	return CheckResource(aJob.myModel)
		&& CheckResource(aJob.myPipeline)
		&& std::all_of(aJob.myTextures.begin(), aJob.myTextures.end(), CheckResource);
}

void DefaultRenderPass::PrepareContext(RenderContext& aContext) const
{
	aContext.myViewportSize[0] = static_cast<int>(Graphics::GetWidth());
	aContext.myViewportSize[1] = static_cast<int>(Graphics::GetHeight());

	aContext.myShouldClearColor = true;
	aContext.myShouldClearDepth = true;

	aContext.myTexturesToActivate[0] = 0;

	aContext.myEnableCulling = true;
	aContext.myEnableDepthTest = true;
}

void DefaultRenderPass::Process(RenderJob& aJob, const IParams& /*aParams*/) const
{
	aJob.SetDrawMode(RenderJob::DrawMode::Indexed);
}

bool TerrainRenderPass::HasResources(const RenderJob& aJob) const
{
	constexpr auto CheckResource = [](const Handle<GPUResource>& aRes) {
		return aRes.IsValid() && aRes->GetState() == GPUResource::State::Valid;
	};
	return CheckResource(aJob.myPipeline)
		&& std::all_of(aJob.myTextures.begin(), aJob.myTextures.end(), CheckResource);
}

void TerrainRenderPass::PrepareContext(RenderContext& aContext) const
{
	aContext.myViewportSize[0] = static_cast<int>(Graphics::GetWidth());
	aContext.myViewportSize[1] = static_cast<int>(Graphics::GetHeight());

	//aContext.myShouldClearColor = true;
	//aContext.myShouldClearDepth = true;

	aContext.myTexturesToActivate[0] = 0;

	aContext.myPolygonMode = Graphics::ourUseWireframe ? 
		RenderContext::PolygonMode::Line : RenderContext::PolygonMode::Fill;

	aContext.myEnableCulling = true;
	aContext.myEnableDepthTest = true;
}

void TerrainRenderPass::Process(RenderJob& aJob, const IParams& aParams) const
{
	aJob.SetDrawMode(RenderJob::DrawMode::Tesselated);

	const TerrainRenderParams& params = static_cast<const TerrainRenderParams&>(aParams);
	aJob.SetTesselationInstanceCount(params.myTileCount);
}