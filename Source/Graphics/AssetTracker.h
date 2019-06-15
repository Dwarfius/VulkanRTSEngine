#pragma once

#include "IGPUAllocator.h"

// Class for handling different resource types using the same interface. 
// Threadsafe
class AssetTracker
{
private:
	using AssetIter = std::unordered_map<Resource::Id, Resource*>::const_iterator;
	using AssetPair = std::pair<const Resource::Id, Resource*>;
	using RegisterIter = std::unordered_map<std::string, Resource::Id>::const_iterator;

public:
	AssetTracker();

	// Sets the object for allocating GPU resources
	void SetGPUAllocator(IGPUAllocator* anAllocator) { myGPUAllocator = anAllocator; }

	// Creates a dynamic asset. Caller responsible for calling Load manually.
	// Once Load is done, Tracker will schedule it for upload. Threadsafe
	template<class AssetType>
	Handle<AssetType> Create();

	// Creates an asset and schedules for load, otherwise just returns an existing asset.
	// Threadsafe
	template<class AssetType>
	Handle<AssetType> GetOrCreate(std::string aPath);

	// Cleans up the assets, as well as loads and uploads the new ones. 
	// Changes OpenGL state if OpenGL backend is used.
	void ProcessQueues();

private:
	// ==========================
	// Graphics support
	friend class Graphics;
	friend class GraphicsGL;
	friend class GraphicsVk;
	// Unloads all resources from the GPU
	void UnloadAll();
	// ==========================
	
	void ProcessReleases();
	void ProcessLoads();
	void ProcessUploads();

	// Utility method to clean-up the resource from registry and asset collections
	// Doesn't delete the actual resource - it's Handle's responsibility
	// Schedules GPUResource to be removed
	// Threadsafe
	void RemoveResource(const Resource* aRes);

	IGPUAllocator* myGPUAllocator;

	tbb::spin_mutex myRegisterMutex;
	tbb::spin_mutex myAssetMutex;
	std::atomic<Resource::Id> myCounter;
	// since all resources come from disk, we can track them by their path
	std::unordered_map<std::string, Resource::Id> myRegister;
	// Resources have unique(among their type) Id, and it's the main way to find it
	// Yes, it's stored as raw, but the memory is managed by Handles
	std::unordered_map<Resource::Id, Resource*> myAssets;

	// Queues support variables
	tbb::concurrent_queue<GPUResource*> myReleaseQueue;
	tbb::concurrent_queue<Handle<Resource>> myLoadQueue;
	tbb::concurrent_queue<Handle<Resource>> myUploadQueue;
};

template<class AssetType>
Handle<AssetType> AssetTracker::Create()
{
	static_assert(is_base_of_v<Resource, AssetType>, "Asset tracker cannot track this type!");

	Resource::Id resourceId = ++myCounter;
	AssetType* asset = new AssetType(resourceId);

	// safely add it to the tracked assets
	{
		tbb::spin_mutex::scoped_lock lock(myAssetMutex);
		myAssets[resourceId] = asset;
	}

	// set up the onDestroy callback, so that we can clean up 
	// the registry and assets containters when it gets removed
	asset->AddOnDestroyCB(bind(&AssetTracker::RemoveResource, this, std::placeholders::_1));

	Handle<AssetType> handle(asset);

	// add it to the queue of uploading
	// the user will have to manually call Load on the Resource
	myUploadQueue.push(handle.template Get<Resource>());

	return handle;
}

template<class AssetType>
Handle<AssetType> AssetTracker::GetOrCreate(std::string aPath)
{
	static_assert(std::is_base_of_v<Resource, AssetType>, "Asset tracker cannot track this type!");

	// first gotta check if we have it in the registry
	bool needsCreating = false;
	Resource::Id resourceId = Resource::InvalidId;
	{
		tbb::spin_mutex::scoped_lock lock(myRegisterMutex);
		std::unordered_map<std::string, Resource::Id>::const_iterator pair = myRegister.find(aPath);
		if (pair != myRegister.end())
		{
			resourceId = pair->second;
		}
		else
		{
			// we don't have one, so register one
			needsCreating = true;
			resourceId = ++myCounter;
			myRegister[aPath] = resourceId;
		}
	}

	if (needsCreating)
	{
		AssetType* asset = new AssetType(resourceId, aPath);

		// safely add it to the tracked assets
		{
			tbb::spin_mutex::scoped_lock lock(myAssetMutex);
			myAssets[resourceId] = asset;
		}

		// set up the onDestroy callback, so that we can clean up 
		// the registry and assets containters when it gets removed
		asset->AddOnDestroyCB(bind(&AssetTracker::RemoveResource, this, std::placeholders::_1));

		// adding it to the queue of loading, since we know that it'll be loaded from file
		myLoadQueue.push(asset);

		return Handle<AssetType>(asset);
	}
	else
	{
		// now that we have an id, we can find it
		tbb::spin_mutex::scoped_lock lock(myAssetMutex);
		AssetIter pair = myAssets.find(resourceId);
		return Handle<AssetType>(pair->second);
	}
}