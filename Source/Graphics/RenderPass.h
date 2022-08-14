#pragma once

#include "RenderPassJob.h"

#include <Core/CRC32.h>

class RenderContext;
class Graphics;
class Camera;
class UniformBuffer;
template<class T>
class Handle;

// The goal behind the render passes is to be able to 
// setup a a render environment, sort the objects, and 
// submit them as required. It is implemented to make use
// of parallelism, both of the render-pass use(both can be 
// used for building concurrently) as well as job creation
// (objects get submitted concurrently to generate jobs from)

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
	virtual void PrepareContext(RenderContext& aContext, Graphics& aGraphics) const = 0;

	RenderContext myRenderContext;
	bool myHasValidContext = false;
	std::vector<Id> myDependencies;
};

// RenderPass doesn't accumulate the renderables internally, 
// it just immediatelly submits them to the render pass job
class RenderPass : public IRenderPass
{
public:
	RenderJob& AllocateJob();
	UniformBuffer* AllocateUBO(Graphics& aGraphics, size_t aSize);

	void BeginPass(Graphics& anInterface) override;
	void SubmitJobs(Graphics& anInterface) override {}

	// updates the renderjob with the correct render settings
	virtual void Process(RenderJob& aJob, const IParams& aParams) const = 0;

private:
	RenderPassJob* myCurrentJob = nullptr;
	
	struct UBOBucket
	{
		// Note: not using LazyVector as it's not thread safe
		std::vector<Handle<UniformBuffer>> myUBOs;
		std::atomic<uint32_t> myUBOCounter;
		size_t mySize;

		UBOBucket(size_t aSize);

		UBOBucket(UBOBucket&& aOther) noexcept
		{
			*this = std::move(aOther);
		}

		UBOBucket& operator=(UBOBucket&& aOther) noexcept
		{
			myUBOs = std::move(aOther.myUBOs);
			mySize = std::move(aOther.mySize);
			myUBOCounter = aOther.myUBOCounter.load();
			return *this;
		}

		int operator<(const UBOBucket& aOther) const
		{
			return mySize < aOther.mySize;
		}

		UniformBuffer* AllocateUBO(Graphics& aGraphics, size_t aSize);
		void PrepForPass(Graphics& aGraphics);
	};
	std::vector<UBOBucket> myUBOBuckets;
	tbb::concurrent_unordered_set<size_t> myNewBuckets;
};