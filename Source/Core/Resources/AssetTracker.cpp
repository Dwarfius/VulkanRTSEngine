#include "Precomp.h"
#include "AssetTracker.h"

#include "JsonSerializer.h"
#include "File.h"
#include "../Profiler.h"
#include "BinarySerializer.h"

std::vector<Handle<Resource>> AssetTracker::DebugAccess::AccessResources(AssetTracker& aTracker)
{
	std::vector<Handle<Resource>> resources;
	tbb::spin_mutex::scoped_lock lock(aTracker.myAssetMutex);
	resources.reserve(aTracker.myAssets.size());
	for (auto& [id, res] : aTracker.myAssets)
	{
		resources.emplace_back(res);
	}
	return resources;
}

AssetTracker::AssetTracker()
	: myCounter(Resource::InvalidId)
{
}

AssetTracker::ResIdPair AssetTracker::FindRes(std::string_view aPath)
{
	Resource::Id resourceId = Resource::InvalidId;
	std::string_view foundPath = aPath;
	{
		tbb::spin_mutex::scoped_lock lock(myRegisterMutex);
		auto iter = myRegister.find(aPath);
		if (iter != myRegister.end())
		{
			resourceId = iter->second;
		}
	}
	// It could be that the resource is external, so try to find it's owner
	if(resourceId == Resource::InvalidId)
	{
		{
			tbb::spin_mutex::scoped_lock lock(myExternalsMutex);
			auto iter = myExternals.find(aPath);
			if (iter == myExternals.end())
			{
				// we didn't find it, return a dud
				return { };
			}

			resourceId = iter->second;
		}
		{
			// if we found it, then we have to look up the path of the owner
			tbb::spin_mutex::scoped_lock lock(myRegisterMutex);
			auto pathIter = myPaths.find(resourceId);
			ASSERT(pathIter != myPaths.end());
			foundPath = pathIter->second;
		}
	}
	return { foundPath, resourceId };
}

Handle<Resource> AssetTracker::ResourceChanged(ResIdPair aRes, bool aForceLoad /* = false */)
{
	Resource* changedRes = nullptr;
	GetResource(aRes.myId, changedRes);
	if(!changedRes && aForceLoad)
	{
		tbb::spin_mutex::scoped_lock lock(myAssetMutex);
		auto createIter = myCreates.find(aRes.myId);
		ASSERT(createIter != myCreates.end());
		changedRes = createIter->second(aRes.myId, aRes.myPath);
		changedRes->myOnDestroyCB = [=](const Resource* aRes) { RemoveResource(aRes); };
		myAssets[aRes.myId] = changedRes;
	}
	if (changedRes)
	{
		changedRes->myState = Resource::State::Uninitialized;
		StartLoading(changedRes);
	}
	return changedRes;
}

void AssetTracker::RegisterExternal(std::string_view aPath, Resource::Id anId)
{
	const std::string path{ aPath };

	tbb::spin_mutex::scoped_lock lock(myExternalsMutex);
	myExternals.insert({path, anId});
}

std::string AssetTracker::NormalizePath(std::string_view aPath, std::string_view anExt)
{
	static_assert(std::string_view(Resource::kAssetsFolder).back() == '/', "Asset Folder must end with a slash!");
	ASSERT_STR(anExt[0] == '.', "Extension must start with a period!");

	const bool hasAssetsRoot = aPath.starts_with(Resource::kAssetsFolder);
	const bool hasExt = aPath.ends_with(anExt);

	std::string normalizedPath;
	normalizedPath.reserve(
		(!hasAssetsRoot ? Resource::kAssetsFolder.GetLength() : 0) + 
		aPath.size() + 
		(!hasExt ? anExt.size() : 0) +
		1 // for \0
	);

	if (!hasAssetsRoot)
	{
		normalizedPath.append(Resource::kAssetsFolder);
	}
	normalizedPath.append(aPath);
	if (!hasExt)
	{
		normalizedPath.append(anExt);
	}
	return normalizedPath;
}

void AssetTracker::SaveAndTrackImpl(std::string_view aPath, Resource& aRes)
{
	// change the handle's path
	aRes.myPath.assign(aPath);

	// dump to disk
	if (aRes.PrefersBinarySerialization())
	{
		BinarySerializer binSerializer(*this, false);
		aRes.Save(*this, binSerializer);
	}
	else
	{
		JsonSerializer jsonSerializer(*this, false);
		aRes.Save(*this, jsonSerializer);
	}

	// if it's a newly generated object - give it an id for tracking
	if (aRes.GetId() == Resource::InvalidId)
	{
		aRes.myId = ++myCounter;
	}

	// register for tracking
	{
		tbb::spin_mutex::scoped_lock lock(myRegisterMutex);
		myRegister[aRes.myPath] = aRes.GetId();
		myPaths[aRes.GetId()] = aRes.myPath;
	}

	{
		tbb::spin_mutex::scoped_lock lock(myAssetMutex);
		myAssets[aRes.GetId()] = &aRes;
	}
}

void AssetTracker::RemoveResource(const Resource* aRes)
{
	tbb::spin_mutex::scoped_lock assetsLock(myAssetMutex);
	myAssets.erase(aRes->GetId());
}

void AssetTracker::AssignDynamicId(Resource& aResource)
{
	aResource.myId = ++myCounter;
	aResource.myOnDestroyCB = [=](const Resource* aRes) { RemoveResource(aRes); };

	{
		tbb::spin_mutex::scoped_lock assetsLock(myAssetMutex);
		myAssets.insert({aResource.myId, &aResource});
	}
}

Resource::Id AssetTracker::GetOrCreateResourceId(std::string_view aPath)
{
	Resource::Id resourceId = Resource::InvalidId;

	tbb::spin_mutex::scoped_lock lock(myRegisterMutex);
	auto pair = myRegister.find(aPath);
	if (pair != myRegister.end())
	{
		resourceId = pair->second;
	}
	else
	{
		// we don't have one, so register one
		resourceId = ++myCounter;
		std::string path{ aPath };
		myRegister[path] = resourceId;
		myPaths[resourceId] = std::move(path);
	}

	return resourceId;
}

void AssetTracker::StartLoading(Handle<Resource> aRes)
{
	struct LoadTask
	{
		void operator()() const
		{
			Profiler::ScopedMark profile("AssetTracker::LoadTask");

			// if we're holding the last handle, means noone needs this anymore,
			// so can skip it and destroy it implicitly
			if (!myRes.IsLastHandle())
			{
				if (myRes->PrefersBinarySerialization())
				{
					BinarySerializer binSerializer(myAssetTracker, true);
					myRes->Load(myAssetTracker, binSerializer);
				}
				else
				{
					JsonSerializer jsonSerializer(myAssetTracker, true);
					myRes->Load(myAssetTracker, jsonSerializer);
				}
				
				ASSERT_STR(myRes->GetState() == Resource::State::Ready
					|| myRes->GetState() == Resource::State::Error,
					"Found a resource that didn't change it's state after Load!");
			}
		}

		mutable Handle<Resource> myRes;
		AssetTracker& myAssetTracker;
	};

	myLoadTaskGroup.run(LoadTask{ aRes, *this });
}