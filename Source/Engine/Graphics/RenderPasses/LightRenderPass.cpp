#include "Precomp.h"
#include "LightRenderPass.h"

#include <Graphics/Graphics.h>
#include <Graphics/RenderContext.h>
#include <Graphics/RenderPassJob.h>

#include "Light.h"

LightRenderPass::LightRenderPass(Graphics& aGraphics)
{
	myLightsUBO = aGraphics.CreateUniformBuffer(LightAdapter::ourDescriptor.GetBlockSize());
}

void LightRenderPass::Execute(Graphics& aGraphics)
{
	// We don't render anything - we just gather data to push to UBO
	if (myLightsUBO->GetState() != GPUResource::State::Valid)
	{
		return;
	}

	UniformBlock block(*myLightsUBO.Get());
	AdapterSourceData source(aGraphics);
	LightAdapter::FillUniformBlock(source, block);

	RenderPassJob& passJob = aGraphics.CreateRenderPassJob({});
	RenderPassJob::SetBufferCmd& uboCmd = passJob.GetCmdBuffer().Write<RenderPassJob::SetBufferCmd>();
	uboCmd.mySlot = LightAdapter::kBindpoint;
	uboCmd.myBuffer = myLightsUBO.Get();
}