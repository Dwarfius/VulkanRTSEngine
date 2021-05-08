#include "Precomp.h"
#include "AssetTracker.h"

#include "JsonSerializer.h"
#include "File.h"
#include "../Profiler.h"
#include "BinarySerializer.h"

tbb::task* AssetTracker::LoadTask::execute()
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
	
	// we return nullptr here because there's no continuation (dependent task)
	// from completion a file load. If this change, will need to adjust
	// to avoid an extra task allocation
	return nullptr;
}

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

void AssetTracker::SaveAndTrack(const std::string& aPath, Handle<Resource> aRes)
{
	// change the handle's path
	ASSERT_STR(aRes.IsValid(), "Invalid handle - nothing to save!");
	aRes->myPath = Resource::kAssetsFolder.CStr() + aPath;

	// dump to disk
	Serializer* serializer = nullptr;
	BinarySerializer binSerializer(*this, false);
	if (aRes->PrefersBinarySerialization())
	{
		serializer = &binSerializer;
	}
	else
	{
		serializer = myWriteSerializers.local();
	}
	aRes->Save(*this, *serializer);

	// if it's a newly generated object - give it an id for tracking
	if (aRes->GetId() == Resource::InvalidId)
	{
		aRes->myId = ++myCounter;
	}

	// register for tracking
	{
		tbb::spin_mutex::scoped_lock lock(myRegisterMutex);
		myRegister[aPath] = aRes->GetId();
	}

	{
		tbb::spin_mutex::scoped_lock lock(myAssetMutex);
		myAssets[aRes->GetId()] = aRes.Get();
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