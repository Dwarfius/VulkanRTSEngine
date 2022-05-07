#include "Precomp.h"
#include "RenderPass.h"

#include "RenderContext.h"
#include "Graphics.h"

IRenderPass::IRenderPass()
	: myHasValidContext(false)
{
}

void IRenderPass::BeginPass(Graphics& anInterface)
{
	if (!myHasValidContext || HasDynamicRenderContext())
	{
		PrepareContext(myRenderContext, anInterface);
		myHasValidContext = true;
	}
}

// ===============================================================

RenderPass::RenderPass()
	: myCurrentJob(nullptr)
{
}

RenderJob& RenderPass::AllocateJob()
{
	return myCurrentJob->AllocateJob();
}

void RenderPass::BeginPass(Graphics& anInterface)
{
	IRenderPass::BeginPass(anInterface);

	myCurrentJob = &anInterface.GetRenderPassJob(GetId(), myRenderContext);
	myCurrentJob->Clear();
}

void RenderPass::SubmitJobs(Graphics& anInterface)
{

}