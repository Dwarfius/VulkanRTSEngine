#include "Precomp.h"
#include "AssetTracker.h"

#include "JsonSerializer.h"
#include "File.h"
#include "../Profiler.h"
#include "BinarySerializer.h"

AssetTracker::AssetTracker()
	: myCounter(Resource::InvalidId)
	, myReadSerializers([this]() {	return new JsonSerializer(*this, true);	})
	, myWriteSerializers([this]() {	return new JsonSerializer(*this, false); })
{
}

AssetTracker::~AssetTracker()
{
	// we can't use RAII type like std::unique_ptr because it
	// requires type to be fully defined, but we only forward
	// declare it at that point
	for (Serializer* serializer : myReadSerializers)
	{
		delete serializer;
	}
	for (Serializer* serializer : myWriteSerializers)
	{
		delete serializer;
	}
}

void AssetTracker::SaveAndTrackImpl(const std::string& aPath, Resource& aRes)
{
	// change the handle's path
	aRes.myPath = Resource::kAssetsFolder.CStr() + aPath;

	// dump to disk
	Serializer* serializer = nullptr;
	BinarySerializer binSerializer(*this, false);
	if (aRes.PrefersBinarySerialization())
	{
		serializer = &binSerializer;
	}
	else
	{
		serializer = myWriteSerializers.local();
	}
	aRes.Save(*this, *serializer);

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
	const std::string& path = aRes->GetPath();

	{
		tbb::spin_mutex::scoped_lock assetsLock(myAssetMutex);
		myAssets.erase(aRes->GetId());
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
				Serializer* serializer = nullptr;
				BinarySerializer binSerializer(myAssetTracker, true);
				if (myRes->PrefersBinarySerialization())
				{
					serializer = &binSerializer;
				}
				else
				{
					serializer = myAssetTracker.myReadSerializers.local();
				}
				myRes->Load(myAssetTracker, *serializer);
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