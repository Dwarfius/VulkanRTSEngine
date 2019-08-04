#include "Precomp.h"
#include "Graphics.h"

#include "Camera.h"

Graphics* Graphics::ourActiveGraphics = NULL;
bool Graphics::ourUseWireframe = false;
int Graphics::ourWidth = 800;
int Graphics::ourHeight = 600;

Graphics::Graphics(AssetTracker& anAssetTracker)
	: myAssetTracker(anAssetTracker)
	, myRenderCalls(0)
{
}

void Graphics::BeginGather()
{
	myRenderCalls = 0;

	for (IRenderPass* pass : myRenderPasses)
	{
		pass->BeginPass(*this);
	}
}

void Graphics::Render(IRenderPass::Category aCategory, const Camera& aCam, const RenderJob& aJob, const IRenderPass::IParams& aParams)
{
	// TODO: find a better way to match it
	// find the renderpass fitting the category
	IRenderPass* renderPassToUse = nullptr;
	for (IRenderPass* pass : myRenderPasses)
	{
		if (pass->GetCategory() == aCategory)
		{
			renderPassToUse = pass;
			break;
		}
	}

	renderPassToUse->AddRenderable(aJob, aParams);

	myRenderCalls++;
}

void Graphics::Display()
{
	// trigger submission of all the jobs
	for (IRenderPass* pass : myRenderPasses)
	{
		pass->SubmitJobs(*this);
	}
}

void Graphics::AddRenderPass(IRenderPass* aRenderPass)
{
	myRenderPasses.push_back(aRenderPass);
}