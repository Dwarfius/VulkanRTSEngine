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

// implementation interface
class IRenderPass
{
public:
	using Id = uint32_t;

	struct IParams
	{
		uint32_t myOffset = 0; // rendering param, offset into rendering buffer
		uint32_t myCount = -1; // rendering param, how many elements to render from a buffer, all by default
		// Distance from a camera
		float myDistance = 0;
	};
public:
	IRenderPass();
	virtual ~IRenderPass() = default;

	virtual void BeginPass(Graphics& anInterface);
	virtual void SubmitJobs(Graphics& anInterface) = 0;
	virtual Id GetId() const = 0;

	void AddDependency(Id aOtherPassId) { myDependencies.push_back(aOtherPassId); }
	const std::vector<Id>& GetDependencies() const { return myDependencies; }

protected:
	// Controls whether every frame the context needs to be recreated
	// or can it be cached at initialization time and reused
	// By default is static render context
	virtual bool HasDynamicRenderContext() const { return false; }
	// default implementation returns full viewport context
	virtual void PrepareContext(RenderContext& aContext) const = 0;

	RenderContext myRenderContext;
	bool myHasValidContext;
	std::vector<Id> myDependencies;
};

// RenderPass doesn't accumulate the renderables internally, 
// it just immediatelly submits them to the render pass job
class RenderPass : public IRenderPass
{
public:
	RenderPass();

	void AddRenderable(RenderJob& aJob, const IParams& aParams);

	void BeginPass(Graphics& anInterface) override final;
	void SubmitJobs(Graphics& anInterface) override final;

protected:
	// updates the renderjob with the correct render settings
	virtual void Process(RenderJob& aJob, const IParams& aParams) const = 0;

private:
	RenderPassJob* myCurrentJob;
};