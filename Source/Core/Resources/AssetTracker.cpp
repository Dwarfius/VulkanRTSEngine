#include "Precomp.h"
#include "AssetTracker.h"

#include "JsonSerializer.h"
#include "File.h"
#include "../Profiler.h"
#include "BinarySerializer.h"

AssetTracker::AssetTracker()
	: myCounter(Resource::InvalidId)
{
}

Handle<Resource> AssetTracker::FileChanged(const std::string& aPath)
{
	auto GetResId = [&](const std::string& aStr) {
		Resource::Id resourceId = Resource::InvalidId;
		tbb::spin_mutex::scoped_lock lock(myRegisterMutex);
		auto iter = myRegister.find(aStr);
		if (iter != myRegister.end())
		{
			resourceId = iter->second;
		}
		return resourceId;
	};

	Resource::Id resourceId = GetResId(aPath);

	// not an asset we had loaded/tracking
	if (resourceId == Resource::InvalidId)
	{
		tbb::spin_mutex::scoped_lock lock(myExternalsMutex);
		auto iter = myExternals.find(aPath);
		if (iter == myExternals.end())
		{
			return {};
		}

		resourceId = iter->second;
	}

	Resource* changedRes = nullptr;
	GetResource(resourceId, changedRes);
	if(!changedRes) // TODO: support unloaded cases!
	{
		return {};
	}
	changedRes->myState = Resource::State::Uninitialized;
	StartLoading(changedRes);
	return changedRes;
}

void AssetTracker::RegisterExternal(std::string_view aPath, Resource::Id anId)
{
	const std::string path{ aPath.data(), aPath.size() };

	tbb::spin_mutex::scoped_lock lock(myExternalsMutex);
	myExternals.insert({path, anId});
}

void AssetTracker::SaveAndTrackImpl(const std::string& aPath, Resource& aRes)
{
	// change the handle's path
	if (!aPath.starts_with(Resource::kAssetsFolder.CStr()))
	{
		aRes.myPath = Resource::kAssetsFolder.CStr() + aPath;
	}
	else
	{
		aRes.myPath = aPath;
	}

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
		myRegister[aPath] = aRes.GetId();
	}

	{
		tbb::spin_mutex::scoped_lock lock(myAssetMutex);
		myAssets[aRes.GetId()] = &aRes;
	}
}

void AssetTracker::RemoveResource(const Resource* aRes)
{
	const Resource::Id id = aRes->GetId();
	{
		tbb::spin_mutex::scoped_lock assetsLock(myAssetMutex);
		myAssets.erase(id);
	}
	
	{
		tbb::spin_mutex::scoped_lock externLock(myExternalsMutex);
		std::erase_if(myExternals, [id](const auto& item) {
			return item.second == id;
		});
	}
}

void AssetTracker::AssignDynamicId(Resource& aResource)
{
	aResource.myId = ++myCounter;

	{
		tbb::spin_mutex::scoped_lock assetsLock(myAssetMutex);
		myAssets.insert({aResource.myId, &aResource});
	}
}

Resource::Id AssetTracker::GetOrCreateResourceId(const std::string& aPath)
{
	Resource::Id resourceId = Resource::InvalidId;

	tbb::spin_mutex::scoped_lock lock(myRegisterMutex);
	std::unordered_map<std::string, Resource::Id>::const_iterator pair = myRegister.find(aPath);
	if (pair != myRegister.end())
	{
		resourceId = pair->second;
	}
	else
	{
		// we don't have one, so register one
		resourceId = ++myCounter;
		myRegister[aPath] = resourceId;
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