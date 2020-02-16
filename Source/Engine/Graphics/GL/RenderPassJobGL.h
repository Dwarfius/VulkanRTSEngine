#pragma once

#include <Graphics/RenderPassJob.h>

class PipelineGL;
class ModelGL;
class TextureGL;

class RenderPassJobGL : public RenderPassJob
{
public:
	void Add(const RenderJob& aJob) override final;
	void AddRange(std::vector<RenderJob>&& aJobs) override final;
	void Clear() override final { myJobs.clear(); };
	operator std::vector<RenderJob>() && override final { return myJobs; }

private:
	// returns whether there's any work in this job
	bool HasWork() const override final;
	// called immediatelly after creating a job
	void OnInitialize(const RenderContext& aContext) override final;
	// called just before executing the jobs
	void SetupContext(const RenderContext& aContext) override final;
	// called last to submit render jobs
	void RunJobs() override final;

	static uint32_t ConvertBlendMode(RenderContext::Blending blendMode);

	std::vector<RenderJob> myJobs;
	PipelineGL* myCurrentPipeline;
	ModelGL* myCurrentModel;

	uint32_t myTexturesToUse;
	TextureGL* myCurrentTextures[RenderContext::kMaxTextureActivations];
	int myTextureSlotsToUse[RenderContext::kMaxTextureActivations];
};