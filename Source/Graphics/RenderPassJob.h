#pragma once

#include "RenderContext.h"

#include <Core/StaticVector.h>
#include <Core/StableVector.h>

class GPUPipeline;
class GPUTexture;
class GPUModel;
class UniformBuffer;
class Graphics;

struct RenderJob
{
	using TextureSet = StaticVector<GPUTexture*, 4>;
	using UniformSet = StaticVector<UniformBuffer*, 4>;

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
	void SetDrawParams(const IndexedDrawParams& aParams);
	void SetDrawParams(const TesselationDrawParams& aParams);
	void SetDrawParams(const ArrayDrawParams& aParams);
	const DrawParams& GetDrawParams() const { return myDrawParams; }
	DrawMode GetDrawMode() const { return myDrawMode; }

	GPUPipeline* GetPipeline() { return myPipeline; }
	const GPUPipeline* GetPipeline() const { return myPipeline; }
	void SetPipeline(GPUPipeline* aPipeline) { myPipeline = aPipeline; }

	GPUModel* GetModel() { return myModel; }
	const GPUModel* GetModel() const { return myModel; }
	void SetModel(GPUModel* aModel) { myModel = aModel; }

	TextureSet& GetTextures() { return myTextures; }
	const TextureSet& GetTextures() const { return myTextures; }

	UniformSet& GetUniformSet() { return myUniforms; }
	const UniformSet& GetUniformSet() const { return myUniforms; }

	glm::ivec4 GetScissorRect() const { return  { myScissorRect[0], myScissorRect[1], myScissorRect[2], myScissorRect[3] }; }
	void SetScissorRect(int aIndex, int aValue) { myScissorRect[aIndex] = aValue; }

private:
	// Note - keeping as ptr to have trivial constructor of RenderJob
	GPUPipeline* myPipeline; // non-owning
	GPUModel* myModel; // non-owning

	TextureSet myTextures;
	UniformSet myUniforms;

	int myScissorRect[4]{0, 0, 0, 0};
	DrawMode myDrawMode = DrawMode::Indexed;
	DrawParams myDrawParams;
};

// A basic class encapsulating a set of render commands
// with a specific context. Needs to be specialized for
// different rendering backends!
class RenderPassJob
{
public:
	virtual ~RenderPassJob() = default;

	// Allocate a job to be filled out
	// Threadsafe
	RenderJob& AllocateJob();

	// clear the accumulated jobs
	void Clear() { myJobs.Clear(); }

	void Execute(Graphics& aGraphics);

	void Initialize(const RenderContext& aContext);

protected:
	const RenderContext& GetRenderContext() const { return myContext; }

private:
	// called immediatelly after creating a job
	virtual void OnInitialize(const RenderContext& aContext) = 0;
	// called first before running jobs
	virtual void BindFrameBuffer(Graphics& aGraphics, const RenderContext& aContext) = 0;
	// always called before running jobs
	virtual void Clear(const RenderContext& aContext) = 0;
	// called just before executing the jobs
	virtual void SetupContext(Graphics& aGraphics, const RenderContext& aContext) = 0;
	// called last to submit render jobs
	virtual void RunJobs(StableVector<RenderJob>& aJobs) = 0;

	RenderContext myContext;
	StableVector<RenderJob> myJobs;
	tbb::spin_mutex myJobsMutex;
};