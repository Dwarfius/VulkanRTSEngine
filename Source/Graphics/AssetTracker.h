#pragma once

#include "Resource.h"

// Class for handling different resource types using the same interface. 
// Threadsafe
class AssetTracker
{
private:
	using AssetIter = std::unordered_map<Resource::Id, Resource*>::const_iterator;
	using AssetPair = std::pair<const Resource::Id, Resource*>;
	using RegisterIter = std::unordered_map<std::string, Resource::Id>::const_iterator;

	class LoadTask : public tbb::task
	{
	public:
		LoadTask(AssetTracker& anAssetTracker, Handle<Resource> aRes)
			: myAssetTracker(anAssetTracker)
			, myRes(aRes)
		{
		}

		tbb::task* execute() override final;

	private:
		AssetTracker& myAssetTracker;
		Handle<Resource> myRes;
	};

public:
	AssetTracker();

	// Saves an asset to the disk. Tracks it for future use
	// Threadsafe
	template<class TAsset>
	void SaveAndTrack(
		const std::string& aPath,
		Handle<TAsset> aHandle
	);

	// Creates an asset and schedules for load, otherwise just returns an existing asset.
	// Threadsafe
	template<class TAsset>
	Handle<TAsset> GetOrCreate(std::string aPath);

private:
	// Utility method to clean-up the resource from registry and asset collections
	// Doesn't delete the actual resource - it's Handle's responsibility
	// Threadsafe
	void RemoveResource(const Resource* aRes);

	tbb::spin_mutex myRegisterMutex;
	tbb::spin_mutex myAssetMutex;
	std::atomic<Resource::Id> myCounter;
	// since all resources come from disk, we can track them by their path
	std::unordered_map<std::string, Resource::Id> myRegister;
	// Resources have unique(among their type) Id, and it's the main way to find it
	// Yes, it's stored as raw, but the memory is managed by Handles
	std::unordered_map<Resource::Id, Resource*> myAssets;
};

template<class TAsset>
void AssetTracker::SaveAndTrack(
	const std::string& aPath,
	Handle<TAsset> aHandle
)
{
	ASSERT_STR(false, "NYI");
}

template<class TAsset>
Handle<TAsset> AssetTracker::GetOrCreate(std::string aPath)
{
	static_assert(std::is_base_of_v<Resource, TAsset>, "Asset tracker cannot track this type!");

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
		TAsset* asset = new TAsset(resourceId, aPath);

		// safely add it to the tracked assets
		{
			tbb::spin_mutex::scoped_lock lock(myAssetMutex);
			myAssets[resourceId] = asset;
		}

		// set up the onDestroy callback, so that we can clean up 
		// the registry and assets containters when it gets removed
		asset->AddOnDestroyCB([=](const Resource* aRes) { RemoveResource(aRes); });

		// adding it to the queue of loading, since we know that it'll be loaded from file
		Handle<TAsset> assetHandle = Handle<TAsset>(asset);
		LoadTask* loadTask = new (tbb::task::allocate_root()) LoadTask(*this, assetHandle);
		tbb::task::enqueue(*loadTask);

		return assetHandle;
	}
	else
	{
		// now that we have an id, we can find it
		tbb::spin_mutex::scoped_lock lock(myAssetMutex);
		AssetIter pair = myAssets.find(resourceId);
		return Handle<TAsset>(pair->second);
	}
}