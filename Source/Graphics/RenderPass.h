#pragma once

class RenderContext;
class Graphics;
class Camera;
class GPUBuffer;
template<class T> class Handle;
struct AdapterSourceData;
class CmdBuffer;
class GPUPipeline;

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

	GPUBuffer* AllocateUBO(Graphics& aGraphics, size_t aSize);

	// Helper for filling UBOs for the render job with game state
	// Handles instance UBOs only.
	// Returns false if ran out of allocated UBOs this frame, otherwise true
	// If succeeds, binds all GPUPipeline's UBOs. Doesn't grow the command buffer!
	bool BindUBOs(CmdBuffer& aCmdBuffer, Graphics& aGraphics, const AdapterSourceData& aSource, const GPUPipeline& aPipeline);

	// Helper for filling UBOs for the render job with game state
	// Handles instance UBOs only.
	// Returns false if ran out of allocated UBOs this frame, otherwise true
	// If succeeds, binds all GPUPipeline's UBOs. May grow the command buffer!
	bool BindUBOsAndGrow(CmdBuffer& aCmdBuffer, Graphics& aGraphics, const AdapterSourceData& aSource, const GPUPipeline& aPipeline);

	size_t GetUBOCount() const;
	size_t GetUBOTotalSize() const;

	virtual void Execute(Graphics& aGraphics);

	virtual Id GetId() const = 0;
	void AddDependency(Id aOtherPassId) { myDependencies.push_back(aOtherPassId); }
	const std::vector<Id>& GetDependencies() const { return myDependencies; }

	
	virtual std::string_view GetTypeName() const = 0;

protected:
	void PreallocateUBOs(size_t aSize);

private:
	struct UBOBucket
	{
		// Note: not using LazyVector as it's not thread safe
		std::vector<Handle<GPUBuffer>> myUBOs;
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

		GPUBuffer* AllocateUBO(Graphics& aGraphics, size_t aSize);
		void PrepForPass(Graphics& aGraphics);
	};
	std::vector<UBOBucket> myUBOBuckets;
	tbb::concurrent_unordered_set<size_t> myNewBuckets;
	std::vector<Id> myDependencies;
};