#pragma once

#include "RenderContext.h"

#include <Core/RefCounted.h>

class GPUResource;
class UniformBlock;

struct RenderJob
{
	using TextureSet = std::vector<Handle<GPUResource>>;
	// TODO: replace with std::vector<std::span<char>> that's backed by a pool or per frame allocator
	using UniformSet = std::vector<std::vector<char>>;

	enum class DrawMode : char
	{
		Indexed,
		Tesselated // currently implemented as instanced tesselation
	};

	struct IndexedDrawParams
	{
		uint32_t myOffset;
		uint32_t myCount;
	};

	struct TesselationDrawParams
	{
		int myInstanceCount;
	};

	union DrawParams
	{
		TesselationDrawParams myTessParams;
		IndexedDrawParams myIndexedParams;
	};

public:
	RenderJob() = default;
	RenderJob(Handle<GPUResource> aPipeline, Handle<GPUResource> aModel,
		const TextureSet& aTextures);

	// Copies a set of uniform blocks for a frame
	void SetUniformSet(const std::vector<std::shared_ptr<UniformBlock>>& aUniformSet);

	// Copies a single uniform block for a frame
	// Note: It's on the caller to ensure the order matches the order of descriptors/uniform
	// buffers for a pipeline
	void AddUniformBlock(const UniformBlock& aBlock);

	// Check if this job has any resources which are last handles
	bool HasLastHandles() const;

	void SetPriority(uint32_t aPriority) { myPriority = aPriority; }
	uint32_t GetPriority() const { return myPriority; }
	void SetDrawMode(DrawMode aDrawMode) { myDrawMode = aDrawMode; }
	DrawMode GetDrawMode() const { return myDrawMode; }

	void SetDrawParams(const IndexedDrawParams& aParams) { myDrawParams.myIndexedParams = aParams; }
	void SetDrawParams(const TesselationDrawParams& aParams) { myDrawParams.myTessParams = aParams; }
	const DrawParams& GetDrawParams() const { return myDrawParams; }

	Handle<GPUResource>& GetPipeline() { return myPipeline; }
	const Handle<GPUResource>& GetPipeline() const { return myPipeline; }
	Handle<GPUResource>& GetModel() { return myModel; }
	const Handle<GPUResource>& GetModel() const { return myModel; }
	TextureSet& GetTextures() { return myTextures; }
	const TextureSet& GetTextures() const { return myTextures; }
	UniformSet& GetUniformSet() { return myUniforms; }
	const UniformSet& GetUniformSet() const { return myUniforms; }

	glm::ivec4 GetScissorRect() const { return  { myScissorRect[0], myScissorRect[1], myScissorRect[2], myScissorRect[3] }; }
	void SetScissorRect(int aIndex, int aValue) { myScissorRect[aIndex] = aValue; }

private:
	Handle<GPUResource> myPipeline;
	Handle<GPUResource> myModel;

	TextureSet myTextures;
	UniformSet myUniforms;

	int myScissorRect[4];
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

protected:
	const RenderContext& GetRenderContext() const { return myContext; }

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

	RenderContext myContext;
};