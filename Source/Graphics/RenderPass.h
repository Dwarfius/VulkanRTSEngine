#pragma once

#include "RenderPassJob.h"

#include <Core/LazyVector.h>
#include <Core/CRC32.h>

class RenderContext;
class Graphics;
class Camera;

// The goal behind the render passes is to be able to 
// setup a a render environment, sort the objects, and 
// submit them as required. This abstraction is intended 
// to simplify performance-scaling by running the 
// renderpasses in parallel, as well as enable use render-pass
// concept.
// TODO: investigate static polymorphism benefits

// implementation interface
class IRenderPass
{
public:
	enum class Category
	{
		Renderables, // a pass for rendering objects
		Terrain, // a pass for rendering terrain
		PostProcessing // a pass for performing post processing
		// TODO: add more (shadows, particle, compute?)
	};

	struct IParams
	{
		float myDistance;
	};
public:
	IRenderPass();

	virtual void AddRenderable(const RenderJob& aJob, const IParams& aParams) = 0;
	virtual void BeginPass(Graphics& anInterface);
	virtual void SubmitJobs(Graphics& anInterface) = 0;
	virtual Category GetCategory() const = 0;
	virtual uint32_t Id() const = 0;

protected:
	// Controls whether every frame the context needs to be recreated
	// or can it be cached at initialization time and reused
	// By default is static render context
	virtual bool HasDynamicRenderContext() const { return false; }
	// default implementation returns full viewport context
	virtual void PrepareContext(RenderContext& aContext) const = 0;
	// updates the renderjob to do with the correct render settings
	virtual void Process(RenderJob& aJob, const IParams& aParams) const = 0;

	RenderContext myRenderContext;
	bool myHasValidContext;
};

// RenderPass doesn't accumulate the renderables internally, 
// it just immediatelly submits them to the render pass job
class RenderPass : public IRenderPass
{
public:
	RenderPass();

	void AddRenderable(const RenderJob& aJob, const IParams& aParams) override final;
	void BeginPass(Graphics& anInterface) override final;
	void SubmitJobs(Graphics& anInterface) override final;

protected:
	uint32_t Id() const override { return Utils::CRC32("RenderPass"); }

private:
	RenderPassJob* myCurrentJob;
};

// SortingRenderPass accumualtes renderables internally and discards
// them if they reached the limit of a lazy vector. It'll regrow
// on a start of a frame, to accomodate new renderables.
class SortingRenderPass : public IRenderPass
{
public:
	SortingRenderPass();

	void AddRenderable(const RenderJob& aJob, const IParams& aParams) override final;
	void BeginPass(Graphics& anInterface) override final;
	void SubmitJobs(Graphics& anInterface) override final;

protected:
	virtual uint32_t GetPriority(const IParams& aParams) const = 0;

private:
	uint32_t Id() const override { return Utils::CRC32("SortingRenderPass"); }

	LazyVector<RenderJob, 100> myJobs;
	RenderPassJob* myCurrentJob;
	RenderPassJob* myOldJob;
};