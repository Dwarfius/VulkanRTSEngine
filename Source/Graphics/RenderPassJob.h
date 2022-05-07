#pragma once

#include "RenderContext.h"

#include <Core/RefCounted.h>
#include <Core/StaticVector.h>

class GPUPipeline;
class GPUTexture;
class GPUModel;
class UniformBuffer;
class Graphics;

struct RenderJob
{
	using TextureSet = StaticVector<Handle<GPUTexture>, 4>;
	using UniformSet = StaticVector<Handle<UniformBuffer>, 4>;

	enum class DrawMode : char
	{
		Array,
		Indexed,
		Tesselated // currently implemented as instanced tesselation
	};

	struct ArrayDrawParams
	{
		uint32_t myOffset;
		uint32_t myCount;
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
		ArrayDrawParams myArrayParams;
		TesselationDrawParams myTessParams;
		IndexedDrawParams myIndexedParams;
	};

public:
	RenderJob() = default;
	RenderJob(Handle<GPUPipeline> aPipeline, Handle<GPUModel> aModel,
		const TextureSet& aTextures);

	// Copies a single uniform block for a frame
	// Note: It's on the caller to ensure the order matches the order of descriptors/uniform
	// buffers for a pipeline
	void AddUniformBlock(const Handle<UniformBuffer>& aBuffer);

	// Check if this job has any resources which are last handles
	bool HasLastHandles() const;

	void SetDrawParams(const IndexedDrawParams& aParams);
	void SetDrawParams(const TesselationDrawParams& aParams);
	void SetDrawParams(const ArrayDrawParams& aParams);
	const DrawParams& GetDrawParams() const { return myDrawParams; }
	DrawMode GetDrawMode() const { return myDrawMode; }

	Handle<GPUPipeline>& GetPipeline() { return myPipeline; }
	const Handle<GPUPipeline>& GetPipeline() const { return myPipeline; }
	Handle<GPUModel>& GetModel() { return myModel; }
	const Handle<GPUModel>& GetModel() const { return myModel; }
	TextureSet& GetTextures() { return myTextures; }
	const TextureSet& GetTextures() const { return myTextures; }
	UniformSet& GetUniformSet() { return myUniforms; }
	const UniformSet& GetUniformSet() const { return myUniforms; }

	glm::ivec4 GetScissorRect() const { return  { myScissorRect[0], myScissorRect[1], myScissorRect[2], myScissorRect[3] }; }
	void SetScissorRect(int aIndex, int aValue) { myScissorRect[aIndex] = aValue; }

private:
	Handle<GPUPipeline> myPipeline;
	Handle<GPUModel> myModel;

	TextureSet myTextures;
	UniformSet myUniforms;

	int myScissorRect[4];
	DrawMode myDrawMode;
	DrawParams myDrawParams;
};

// A basic class encapsulating a set of render commands
// with a specific context. Needs to be specialized for
// different rendering backends!
class RenderPassJob
{
public:
	virtual ~RenderPassJob() = default;

	// add a single job
	virtual void Add(const RenderJob& aJob) = 0;
	// move a collection of jobs
	virtual void AddRange(std::vector<RenderJob>&& aJobs) = 0;
	// clear the accumulated jobs
	virtual void Clear() = 0;
	// reclaim memory, if possible
	virtual operator std::vector<RenderJob>() && = 0;

	void Execute(Graphics& aGraphics);

	void Initialize(const RenderContext& aContext);

protected:
	const RenderContext& GetRenderContext() const { return myContext; }

private:
	// returns whether there's any work in this job
	virtual bool HasWork() const = 0;
	// called immediatelly after creating a job
	virtual void OnInitialize(const RenderContext& aContext) = 0;
	// called first before running jobs
	virtual void BindFrameBuffer(Graphics& aGraphics, const RenderContext& aContext) = 0;
	// always called before running jobs
	virtual void Clear(const RenderContext& aContext) = 0;
	// called just before executing the jobs
	virtual void SetupContext(Graphics& aGraphics, const RenderContext& aContext) = 0;
	// called last to submit render jobs
	virtual void RunJobs() = 0;

	RenderContext myContext;
};