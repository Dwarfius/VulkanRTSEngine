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

protected:
	void PreallocateUBOs(size_t aSize);

private:
	RenderPassJob* myCurrentJob = nullptr;
	
	struct UBOBucket
	{
		// Note: not using LazyVector as it's not thread safe
		std::vector<Handle<UniformBuffer>> myUBOs;
		std::atomic<uint32_t> myUBOCounter = 32;
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