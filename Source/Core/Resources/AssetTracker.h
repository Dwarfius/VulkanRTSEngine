#pragma once

#include "Resource.h"

// TODO: add std::string_view overrides
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

	// Saves an asset to the disk. Tracks it for future use
	// Automatically appends Asset extension if it's missing
	// Threadsafe
	template<class TAsset>
	void SaveAndTrack(
		const std::string& aPath,
		Handle<TAsset> aHandle
	);

	// Saves an asset to the disk. Tracks it for future use
	// Automatically appends Asset extension if it's missing
	// Threadsafe
	template<class TAsset>
	void SaveAndTrack(
		const std::string& aPath,
		TAsset* aRes
	);

	// Allocates a dynamic resource an Id - allows users to manually
	// tie/associate resources with GPUResources or anything else
	void AssignDynamicId(Resource& aResource);

	// Creates an asset and schedules for load, otherwise just returns an existing asset.
	// Threadsafe
	template<class TAsset>
	Handle<TAsset> GetOrCreate(const std::string& aPath);

	template<class TAsset>
	Handle<TAsset> Get(Resource::Id anId);

	Handle<Resource> FileChanged(const std::string& aPath);
	void RegisterExternal(std::string_view aPath, Resource::Id anId);

private:
	void SaveAndTrackImpl(
		const std::string& aPath,
		Resource& aHandle
	);

	// Utility method to clean-up the resource from registry and asset collections
	// Doesn't delete the actual resource - it's Handle's responsibility
	// Threadsafe
	void RemoveResource(const Resource* aRes);

	Resource::Id GetOrCreateResourceId(const std::string& aPath);

	// returns true if a Resource was found, false otherwise
	template<class TAsset>
	bool GetOrCreateResource(Resource::Id anId, const std::string& aPath, TAsset*& aRes);
	// returns true if a Resource was found, false otherwise
	template<class TAsset>
	bool GetResource(Resource::Id anId, TAsset*& aRes);

	void StartLoading(Handle<Resource> aRes);

	// TODO: replace with mutexes, as unordered_map 
	// operations are too heavy
	tbb::spin_mutex myRegisterMutex;
	tbb::spin_mutex myAssetMutex;
	tbb::spin_mutex myExternalsMutex;
	std::atomic<Resource::Id> myCounter;
	// since all resources come from disk, we can track them by their path
	std::unordered_map<std::string, Resource::Id> myRegister;
	// Resources have unique(among their type) Id, and it's the main way to find it
	// Yes, it's stored as raw, but the memory is managed by Handles
	std::unordered_map<Resource::Id, Resource*> myAssets;
	// Map of external assets to owning resource - hotreload support
	std::unordered_map<std::string, Resource::Id> myExternals;
	oneapi::tbb::task_group myLoadTaskGroup;
};

template<class TAsset>
void AssetTracker::SaveAndTrack(const std::string& aPath, Handle<TAsset> aHandle)
{
	static_assert(std::is_base_of_v<Resource, TAsset>, "Asset tracker cannot track this type!");
	static_assert(!std::is_same_v<TAsset, Resource>, "Invalid type, can't save a generic resource!");

	ASSERT_STR(aHandle.IsValid(), "Trying to save invalid handle!");
	if (aPath.rfind(TAsset::kExtension.CStr()) == std::string::npos)
	{
		SaveAndTrackImpl(aPath + TAsset::kExtension.CStr(), *aHandle.Get());
	}
	else
	{
		SaveAndTrackImpl(aPath, *aHandle.Get());
	}
}

template<class TAsset>
void AssetTracker::SaveAndTrack(const std::string& aPath, TAsset* aRes)
{
	static_assert(std::is_base_of_v<Resource, TAsset>, "Asset tracker cannot track this type!");
	static_assert(!std::is_same_v<TAsset, Resource>, "Invalid type, can't save a generic resource!");

	ASSERT_STR(aRes, "Trying to save invalid resource!");
	if (aPath.rfind(TAsset::kExtension.CStr()) == std::string::npos)
	{
		SaveAndTrackImpl(aPath + TAsset::kExtension.CStr(), *aRes);
	}
	else
	{
		SaveAndTrackImpl(aPath, *aRes);
	}
}

template<class TAsset>
Handle<TAsset> AssetTracker::GetOrCreate(const std::string& aPath)
{
	static_assert(std::is_base_of_v<Resource, TAsset>, "Asset tracker cannot track this type!");
	static_assert(!std::is_same_v<TAsset, Resource>, "Invalid type, can't load a generic resource!");

	// first gotta check if we have it in the registry
	std::string path = Resource::kAssetsFolder.CStr() + aPath;
	Resource::Id resourceId = GetOrCreateResourceId(path);

	// now that we have an id, we can find it
	TAsset* asset = nullptr;
	if (GetOrCreateResource(resourceId, path, asset))
	{
		return Handle<TAsset>(asset);
	}

	// We didn't find it, which means it's newly created and needs loading
	// set up the onDestroy callback, so that we can clean up 
	// the registry and assets containters when it gets removed
	asset->myOnDestroyCB = [=](const Resource* aRes) { RemoveResource(aRes); };

	// adding it to the queue of loading, since we know that it'll be loaded from file
	Handle<TAsset> assetHandle{ asset };
	StartLoading(assetHandle);

	return assetHandle;
}

template<class TAsset>
Handle<TAsset> AssetTracker::Get(Resource::Id anId)
{
	static_assert(std::is_base_of_v<Resource, TAsset>, "Asset tracker cannot track this type!");
	static_assert(!std::is_same_v<TAsset, Resource>, "Invalid type, can't load a generic resource!");

	TAsset* asset = nullptr;
	if (GetResource(anId, asset))
	{
		return Handle<TAsset>(asset);
	}
	return Handle<TAsset>();
}

template<class TAsset>
bool AssetTracker::GetOrCreateResource(Resource::Id anId, const std::string& aPath, TAsset*& aRes)
{
	tbb::spin_mutex::scoped_lock lock(myAssetMutex);
	AssetIter pair = myAssets.find(anId);
	if (pair != myAssets.end())
	{
		aRes = static_cast<TAsset*>(pair->second);
		return true;
	}

	aRes = new TAsset(anId, aPath);
	myAssets[anId] = aRes;
	return false;
}

template<class TAsset>
bool AssetTracker::GetResource(Resource::Id anId, TAsset*& aRes)
{
	tbb::spin_mutex::scoped_lock lock(myAssetMutex);
	AssetIter pair = myAssets.find(anId);
	if (pair != myAssets.end())
	{
		aRes = static_cast<TAsset*>(pair->second);
		return true;
	}
	return false;
}