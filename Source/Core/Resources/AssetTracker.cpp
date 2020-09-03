#include "Precomp.h"
#include "AssetTracker.h"

#include "JsonSerializer.h"
#include "File.h"
#include "../Profiler.h"

tbb::task* AssetTracker::LoadTask::execute()
{
	Profiler::ScopedMark profile("AssetTracker::LoadTask");

	// if we're holding the last handle, means noone needs this anymore,
	// so can skip it and destroy it implicitly
	if (!myRes.IsLastHandle())
	{
		myRes->Load(myAssetTracker);
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

std::optional<std::reference_wrapper<Serializer>> AssetTracker::GetReadSerializer(const std::string& aPath)
{
	File file(aPath);
	if (!file.Read())
	{
		return std::nullopt;
	}

	Serializer& serializer = *myReadSerializers.local();
	serializer.ReadFrom(file);
	return std::ref(serializer);
}

void AssetTracker::RemoveResource(const Resource* aRes)
{
	const std::string& path = aRes->GetPath();

	{
		tbb::spin_mutex::scoped_lock assetsLock(myAssetMutex);
		myAssets.erase(aRes->GetId());
	}
}