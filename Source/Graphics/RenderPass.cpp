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
		PrepareContext(myRenderContext);
		myHasValidContext = true;
	}
}

// ===============================================================

RenderPass::RenderPass()
	: myCurrentJob(nullptr)
{
}

void RenderPass::AddRenderable(RenderJob& aJob, const IParams& aParams)
{
	Process(aJob, aParams);
	myCurrentJob->Add(aJob);
}

void RenderPass::BeginPass(Graphics& anInterface)
{
	IRenderPass::BeginPass(anInterface);

	myCurrentJob = &anInterface.GetRenderPassJob(Id(), myRenderContext);
	myCurrentJob->Clear();
}

void RenderPass::SubmitJobs(Graphics& anInterface)
{

}

// ===============================================================

SortingRenderPass::SortingRenderPass()
	: myCurrentJob(nullptr)
	, myOldJob(nullptr)
{
}

void SortingRenderPass::AddRenderable(RenderJob& aJob, const IParams& aParams)
{
	aJob.SetPriority(GetPriority(aParams));
	Process(aJob, aParams);

	myJobs.PushBack(std::move(aJob));
}

void SortingRenderPass::BeginPass(Graphics& anInterface)
{
	IRenderPass::BeginPass(anInterface);

	// we use 2 frame buffering in order to utilize the lazy vector better
	if (myOldJob)
	{
		ASSERT(myJobs.Size() == 0);
		// reclaim old memory
		myJobs = std::move(*myOldJob);

		// at this point it might be smaller than this frame,
		// so grow if needed
		if (myJobs.NeedsToGrow())
		{
			myJobs.Grow();
		}
	}

	myOldJob = myCurrentJob;
	myCurrentJob = &anInterface.GetRenderPassJob(Id(), myRenderContext);
}

void SortingRenderPass::SubmitJobs(Graphics& anInterface)
{
	// sort the jobs first
	std::sort(myJobs.begin(), myJobs.end(), [](const RenderJob& a1, const RenderJob& a2) { 
		return a1.GetPriority() > a2.GetPriority();
	});

	// transfer all the accumulated jobs
	myCurrentJob->AddRange(std::move(myJobs));
}