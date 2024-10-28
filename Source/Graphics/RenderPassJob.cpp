#include "Precomp.h"
#include "RenderPassJob.h"

#include <Graphics/Resources/GPUBuffer.h>

void RenderPassJob::Execute(Graphics& aGraphics)
{
	BindFrameBuffer(aGraphics, myContext);
	Clear(myContext);

	if(!myCmdBuffer.IsEmpty())
	{
		SetupContext(aGraphics, myContext);
		RunCommands(myCmdBuffer);
	}

	if (myContext.myDownloadTexture)
	{
		DownloadFrameBuffer(aGraphics, *myContext.myDownloadTexture);
	}
}

void RenderPassJob::Initialize(const RenderContext& aContext)
{
	myContext = aContext;
	OnInitialize(myContext);
}