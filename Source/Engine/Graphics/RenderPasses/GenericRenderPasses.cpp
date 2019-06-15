#include "Precomp.h"
#include "GenericRenderPasses.h"

#include <Graphics/Graphics.h>
#include "../../Terrain.h"

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

void TerrainRenderPass::PrepareContext(RenderContext& aContext) const
{
	aContext.myViewportSize[0] = static_cast<int>(Graphics::GetWidth());
	aContext.myViewportSize[1] = static_cast<int>(Graphics::GetHeight());

	aContext.myTexturesToActivate[0] = 0;

	// TESTING TERRAIN TESSELATION
	aContext.myShouldClearColor = true;
	aContext.myShouldClearDepth = true;
	// ===================

	aContext.myPolygonMode = RenderContext::PolygonMode::Line;

	aContext.myEnableCulling = true;
	aContext.myEnableDepthTest = true;
}

void TerrainRenderPass::Process(RenderJob& aJob, const IParams& aParams) const
{
	aJob.SetDrawMode(RenderJob::DrawMode::Tesselated);

	const TerrainRenderParams& params = static_cast<const TerrainRenderParams&>(aParams);
	const glm::vec3 tiles = params.mySize / static_cast<float>(Terrain::TileSize);
	const int tileCount = static_cast<int>(glm::max(tiles.x, 1.f) * glm::max(tiles.z, 1.f));
	aJob.SetTesselationInstanceCount(tileCount);
}