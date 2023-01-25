#pragma once

#include "RenderPassJob.h"

#include <Core/CRC32.h>

class RenderContext;
class Graphics;
class Camera;
class UniformBuffer;
template<class T>
class Handle;
struct AdapterSourceData;

// The goal behind the render passes is to be able to 
// setup a a render environment, sort the objects, and 
// submit them as required. It is implemented to make use
// of parallelism, both of the render-pass use(both can be 
// used for building concurrently) as well as job creation
// (objects get submitted concurrently to generate jobs from)
class RenderPass
{
public:
	using Id = uint32_t;

	virtual ~RenderPass() = default;

	UniformBuffer* AllocateUBO(Graphics& aGraphics, size_t aSize);

	// Helper for filling UBOs for the render job with game state
	// Handles both normal and global UBOs.
	// Returns false if ran out of allocated UBOs this frame, otherwise true
	bool FillUBOs(RenderJob::UniformSet& aSet, Graphics& aGraphics, const AdapterSourceData& aSource, const GPUPipeline& aPipeline);

	virtual void Execute(Graphics& aGraphics);

	virtual Id GetId() const = 0;
	void AddDependency(Id aOtherPassId) { myDependencies.push_back(aOtherPassId); }
	const std::vector<Id>& GetDependencies() const { return myDependencies; }

protected:
	void PreallocateUBOs(size_t aSize);

private:
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
	std::vector<Id> myDependencies;
};