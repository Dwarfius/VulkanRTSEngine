#pragma once

#include <Core/RefCounted.h>
#include "Resource.h"

class Graphics;

// Base class for resources on the GPU. 
// Operations with it aren't thread safe - requires external sync
// when modifying properties of GPUResource subclasses
class GPUResource : 
	public RefCountedWithDestroy<GPUResource>
{
public:
	static void Destroy(GPUResource* aResource);
	
	enum class State
	{
		Invalid,
		PendingCreate,
		PendingUpload,
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

	// You can specify for the GPU resource to retain it's source resource
	// That means it'll retain it during the entire duration of it's runtime
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

#ifdef _DEBUG
	const std::string& GetErrorMsg() const { return myErrorMsg; }
#else
	const std::string& GetErrorMsg() const { return ""; }
#endif

protected:
	Handle<Resource> myResHandle;
	Resource::Id myResId;
	std::vector<Handle<GPUResource>> myDependencies;
	State myState;

	// A convinience wrapper to set the error message in debug builds.
	// Sets the state to Error
	void SetErrMsg(std::string&& anErrString);

private:
	virtual void OnCreate(Graphics& aGraphics) = 0;
	virtual bool OnUpload(Graphics& aGraphics) = 0;
	virtual void OnUnload(Graphics& aGraphics) = 0;

	//================================
	// Graphics interface support
	friend class Graphics;

	void TriggerCreate();
	void TriggerUpload();
	void TriggerUnload();
	//================================

	void UpdateDependencies(const Resource* aRes);

	Graphics* myGraphics; // non owning ptr
	std::vector<UploadRegion> myRegionsToUpload;

#ifdef _DEBUG
	// used for tracking what went wrong
	std::string myErrorMsg;
#endif
	bool myKeepResHandle;
};