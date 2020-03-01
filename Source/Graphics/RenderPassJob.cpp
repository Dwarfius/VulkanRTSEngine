#include "Precomp.h"
#include "RenderPassJob.h"

void RenderPassJob::Execute()
{
	if (myContext.myShouldClearColor || myContext.myShouldClearDepth)
	{
		Clear(myContext);
	}

	if (HasWork())
	{
		SetupContext(myContext);
		RunJobs();
	}
}

void RenderPassJob::Initialize(const RenderContext& aContext)
{
	myContext = aContext;
	OnInitialize(myContext);
}