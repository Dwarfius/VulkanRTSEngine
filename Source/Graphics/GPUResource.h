#pragma once

#include <Core/RefCounted.h>

class Graphics;
class Resource;

// Base class for resources on the GPU. 
// Operations with it aren't thread safe - requires external sync
// when modifying properties of GPUResource subclasses
class GPUResource : public RefCounted
{
public:
	using ResourceId = uint32_t;

	enum class State
	{
		Invalid,
		PendingCreate,
		PendingUpload, // pending first upload
		Valid,
		PendingUnload,
		Error
	};

	enum class UsageType
	{
		Static, // upload once and use
		Dynamic // can upload multiple times
	};

	struct UploadRegion
	{
		size_t myStart;
		size_t myCount;
	};

public:
	GPUResource();
	GPUResource(const GPUResource& anOther) = delete;
	GPUResource& operator=(const GPUResource& anOther) = delete;

	void Cleanup() override;

	// You can specify that the GPU resource will retain it's source resource
	// For the entire duration of it's lifetime. Can be useful if there are planned
	// updates-from-source.
	void Create(Graphics& aGraphics, Handle<Resource> aRes, bool aShouldKeepRes = false);
	// Marks that the resource must be updated during next frame prep
	// Note: prefer UpdateRegions to avoid multiple resource upload schedules
	void UpdateRegion(UploadRegion aRegion);
	// Marks that the resource must be updated during next frame prep
	void UpdateRegions(const UploadRegion* aRegions, uint8_t aRegCount);
	// Marks the resource for unloading. Afterwards the resource cannot be
	// accessed in any way, so it can be deleted
	void Unload();

	State GetState() const { return myState; }
	virtual bool AreDependenciesValid() const;

	// Returns the underlying resource, if it's still attached
	// Returns Empty handle if it has been released
	// Note: To retain handle, when creating GPUResource specifiy to
	// keep the resource handle
	Handle<Resource> GetResource() const { return myResHandle; }

	// Adds a dependent for hot reload tracking. The dependents will
	// be reloaded if this GPUResource is reloaded. Asserts on duplicates. 
	// Caller is responsible for cleaning up dependents via RemoveDependent. 
	void AddDependent(GPUResource* aDependent);
	// Removes the dependent from the list, if it has one. 
	void RemoveDependent(GPUResource* aDependent);
	template<class TFunc>
	void ForEachDependent(const TFunc& aFunc) const
	{
#if ASSERT_MUTEX
		AssertReadLock lock(myDependentsMutex);
#endif
		for (GPUResource* res : myDependents)
		{
			aFunc(res);
		}
	}

protected:
	Handle<Resource> myResHandle;
	ResourceId myResId;
	State myState;
	Graphics* myGraphics; // non owning ptr

	// A convinience wrapper to set the error message in debug builds.
	// Sets the state to Error
	void SetErrMsg(std::string_view anErrString);

private:
	virtual void OnCreate(Graphics& aGraphics) = 0;
	virtual bool OnUpload(Graphics& aGraphics) = 0;
	virtual void OnUnload(Graphics& aGraphics) = 0;

	//================================
	// Graphics interface support
	friend class Graphics;

	void TriggerCreate();
	bool TriggerUpload();
	void TriggerUnload();
	//================================

	std::vector<GPUResource*> myDependents;
	std::vector<UploadRegion> myRegionsToUpload;
#if ASSERT_MUTEX
	mutable AssertRWMutex myDependentsMutex;
#endif
	bool myKeepResHandle;
};