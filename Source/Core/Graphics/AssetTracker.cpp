#include "Precomp.h"
#include "AssetTracker.h"

AssetTracker::AssetTracker()
	: myCounter(Resource::InvalidId)
	, myGPUAllocator(nullptr)
{
}

void AssetTracker::ProcessQueues()
{
	ProcessReleases();
	ProcessLoads();
	ProcessUploads();
}

void AssetTracker::UnloadAll()
{
	tbb::spin_mutex::scoped_lock lock(myAssetMutex);
	if (myAssets.size())
	{
		for (AssetPair& assetPair : myAssets)
		{
			assetPair.second->Unload();
			assetPair.second->SetState(Resource::State::PendingUpload);
		}
	}
}

void AssetTracker::ProcessReleases()
{
	GPUResource* resToRelease;
	while (myReleaseQueue.try_pop(resToRelease))
	{
		resToRelease->Unload();
		delete resToRelease;
	}
}

void AssetTracker::ProcessLoads()
{
	Handle<Resource> loadItem;
	while (myLoadQueue.try_pop(loadItem))
	{
		if (loadItem.IsLastHandle())
		{
			// if we're holding the last handle, means noone needs this anymore,
			// so can skip it (which will replace the loadIter), and destroy it
			continue;
		}
		Resource* resource = loadItem.Get();
		// TODO: good candidate for multithreading
		resource->Load();
		if (resource->GetState() == Resource::State::PendingUpload)
		{
			myUploadQueue.push(loadItem);
		}
		else if (resource->GetState() == Resource::State::Error)
		{
			// there was an error, so removing it from tracking
			// it'll get released once the user gets rid of the handle
		}
		else
		{
			ASSERT_STR(false, "Found a resource that didn't change it's state after Load!");
		}
	}
}

void AssetTracker::ProcessUploads()
{
	// using a separate queue to keep track of items that can't be uploaded this
	// update frame because they have dependencies that haven't uploaded
	queue<Handle<Resource>> delayQueue;
	Handle<Resource> uploadItem;
	while (myUploadQueue.try_pop(uploadItem))
	{
		if (uploadItem.IsLastHandle())
		{
			// resource was discarded by owner, so skip it - it'll get released
			continue;
		}

		Resource* resource = uploadItem.Get();

		// can it be uploaded?
		if (resource->GetState() == Resource::State::Invalid)
		{
			// it hasn't been fully loaded yet, so skip it
			delayQueue.push(uploadItem);
			continue;
		}

		// check whether dependencies have loaded
		const vector<Handle<Resource>>& deps = resource->GetDependencies();

		bool dependenciesValid = true;
		bool dependenciesUploaded = true;
		for(const Handle<Resource>& depHadle : deps)
		{
			const Resource* dependency = depHadle.Get();

			if (dependency->GetState() == Resource::State::Error)
			{
				dependenciesValid = false;
				break;
			}

			if (dependency->GetState() != Resource::State::Ready)
			{
				dependenciesUploaded = false;
				break;
			}
		}

		// check if one of the dependencies is missing
		if (!dependenciesValid)
		{
			// mark it as a broken resource
			resource->SetErrMsg("Dependency loading failed!");
			// lose the handle, so that the owner has the last one
			continue;
		}

		// check if dependencies haven't uploaded yet
		if (!dependenciesUploaded)
		{
			delayQueue.push(uploadItem);
			continue;
		}

		// we passed all the checks, means we can upload it to the GPU
		// first, request the GPU allocator to create a resource for us
		ASSERT_STR(!resource->myGPUResource, "Overwriting a GPU resource!");
		GPUResource* gpuRes = myGPUAllocator->Create(resource->GetResType());
		resource->Upload(gpuRes);

		if (resource->GetState() == Resource::State::Error)
		{
			// there was an error, so don't do anything - owner should discard it
		}
		else if (resource->GetState() != Resource::State::Ready)
		{
			ASSERT_STR(false, "Found a resource that didn't change it's state after upload!");
		}
	}

	// after all is done, reinsert the delayed resources back to the upload queue
	while (!delayQueue.empty())
	{
		myUploadQueue.push(delayQueue.front());
		delayQueue.pop();
	}
}

void AssetTracker::RemoveResource(const Resource* aRes)
{
	const string& path = aRes->GetPath();

	if(path.length())
	{
		tbb::spin_mutex::scoped_lock registerLock(myRegisterMutex);
		myRegister.erase(path);
	}

	{
		tbb::spin_mutex::scoped_lock assetsLock(myAssetMutex);
		myAssets.erase(aRes->GetId());
	}

	GPUResource* gpuResource = aRes->GetGPUResource_Int();
	if (gpuResource)
	{
		myReleaseQueue.push(gpuResource);
	}
}