#include "Precomp.h"
#include "AssetTracker.h"

AssetTracker::AssetTracker()
	: myCounter(Resource::InvalidId)
{
}

void AssetTracker::ProcessQueues()
{
	ProcessLoads();
}

void AssetTracker::ProcessLoads()
{
	Handle<Resource> loadItem;
	while (myLoadQueue.try_pop(loadItem))
	{
		if (loadItem.IsLastHandle())
		{
			// if we're holding the last handle, means noone needs this anymore,
			// so can skip it (which will replace the loadItem), and destroy it
			continue;
		}
		Resource* resource = loadItem.Get();
		// TODO: good candidate for multithreading
		resource->Load(*this);
		if (resource->GetState() == Resource::State::Error)
		{
			// Empty scope for the sake of having a comment:
			// Since there was an error, so removing it from tracking
			// it'll get released once the user gets rid of the handle
		}
		else
		{
			ASSERT_STR(resource->GetState() == Resource::State::Ready, "Found a resource that didn't change it's state after Load!");
		}
	}
}

void AssetTracker::RemoveResource(const Resource* aRes)
{
	const std::string& path = aRes->GetPath();

	if(path.length())
	{
		tbb::spin_mutex::scoped_lock registerLock(myRegisterMutex);
		myRegister.erase(path);
	}

	{
		tbb::spin_mutex::scoped_lock assetsLock(myAssetMutex);
		myAssets.erase(aRes->GetId());
	}
}