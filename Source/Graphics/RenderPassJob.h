#pragma once

#include "RenderContext.h"

#include <Core/RefCounted.h>
#include <Graphics/UniformBlock.h>

class GPUResource;

struct RenderJob
{
	using TextureSet = std::vector<Handle<GPUResource>>;
	using UniformSet = std::vector<std::shared_ptr<UniformBlock>>;

	enum class DrawMode : char
	{
		Indexed,
		Tesselated // currently implemented as instanced tesselation
	};

	struct TesselationDrawParams
	{
		int myInstanceCount;
	};

	union DrawParams
	{
		TesselationDrawParams myTessParams;
	};

public:
	Handle<GPUResource> myPipeline;
	Handle<GPUResource> myModel;

	TextureSet myTextures;
	UniformSet myUniforms;

	RenderJob() = default;
	RenderJob(Handle<GPUResource> aPipeline,
		Handle<GPUResource> aModel,
		TextureSet aTextures,
		UniformSet aUniforms)
		: myPipeline(aPipeline)
		, myModel(aModel)
		, myTextures(aTextures)
		, myUniforms(aUniforms)
		, myPriority(0)
		, myDrawMode(DrawMode::Indexed)
		, myDrawParams()
	{
	}

	bool HasLastHandles() const
	{
		bool lastTexture = false;
		for (const Handle<GPUResource>& texture : myTextures)
		{
			lastTexture |= texture.IsLastHandle();
		}
		return myPipeline.IsLastHandle()
			|| myModel.IsLastHandle()
			|| lastTexture;
	}

	void SetPriority(uint32_t aPriority) { myPriority = aPriority; }
	uint32_t GetPriority() const { return myPriority; }
	void SetDrawMode(DrawMode aDrawMode) { myDrawMode = aDrawMode; }
	DrawMode GetDrawMode() const { return myDrawMode; }

	void SetTesselationInstanceCount(int aCount) { myDrawParams.myTessParams.myInstanceCount = aCount; }
	const DrawParams& GetDrawParams() const { return myDrawParams; }

private:
	// higher value means it'll be rendered sooner
	uint32_t myPriority;
	DrawMode myDrawMode;
	DrawParams myDrawParams;
};

// A basic class encapsulating a set of render commands
// with a specific context. Needs to be specialized for
// different rendering backends!
class RenderPassJob
{
public:
	// add a single job
	virtual void Add(const RenderJob& aJob) = 0;
	// move a collection of jobs
	virtual void AddRange(std::vector<RenderJob>&& aJobs) = 0;
	// clear the accumulated jobs
	virtual void Clear() = 0;
	// reclaim memory, if possible
	virtual operator std::vector<RenderJob>() && = 0;

	void Execute();

	void Initialize(const RenderContext& aContext);

private:
	// returns whether there's any work in this job
	virtual bool HasWork() const = 0;
	// called immediatelly after creating a job
	virtual void OnInitialize(const RenderContext& aContext) = 0;
	// called if context requests clearing
	virtual void Clear(const RenderContext& aContext) = 0;
	// called just before executing the jobs
	virtual void SetupContext(const RenderContext& aContext) = 0;
	// called last to submit render jobs
	virtual void RunJobs() = 0;

private:
	RenderContext myContext;
};