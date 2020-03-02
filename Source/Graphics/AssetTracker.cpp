#include "Precomp.h"
#include "AssetTracker.h"

tbb::task* AssetTracker::LoadTask::execute()
{
	// if we're holding the last handle, means noone needs this anymore,
	// so can skip it and destroy it implicitly
	// TODO: should track how many fail the check, and try to prevent
	// them being scheduled
	if (!myRes.IsLastHandle())
	{
		myRes->Load(myAssetTracker);
		ASSERT_STR(myRes->GetState() == Resource::State::Ready
			|| myRes->GetState() == Resource::State::Error,
			"Found a resource that didn't change it's state after Load!");
	}
	// TODO: look into task-recycling
	return nullptr;
}

AssetTracker::AssetTracker()
	: myCounter(Resource::InvalidId)
{
}

void AssetTracker::RemoveResource(const Resource* aRes)
{
	const std::string& path = aRes->GetPath();

	{
		tbb::spin_mutex::scoped_lock assetsLock(myAssetMutex);
		myAssets.erase(aRes->GetId());
	}
}