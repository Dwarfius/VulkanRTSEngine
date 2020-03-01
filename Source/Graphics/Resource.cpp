#include "Precomp.h"
#include "Resource.h"
#include <Core/File.h>

Resource::Resource()
	: myId(InvalidId)
	, myState(State::Ready) // dynamic resources should be fully 
							// initialized before use!
{
}

Resource::Resource(Id anId, const std::string& aPath)
	: myId(anId)
	, myState(State::Uninitialized)
	, myPath(aPath)
{
}

Resource::~Resource()
{
	if (myOnDestroyCBs.size())
	{
		for (const Callback& destroyCB : myOnDestroyCBs)
		{
			destroyCB(this);
		}
	}
}

void Resource::ExecLambdaOnLoad(Callback aOnLoadCB)
{
	// we rely on the fact that we lock on state = Status::Ready
	// before executing the callbacks once OnLoad finishes, so
	// we can rely on status to be up-to-date
	tbb::spin_rw_mutex::scoped_lock lockState(myStateMutex, false);
	if (myState != State::Ready)
	{
		// we have to delay it till OnLoad runs
		tbb::spin_mutex::scoped_lock lockCB(myLoadCBMutex);
		myOnLoadCBs.push_back(aOnLoadCB);
	}
	else
	{
		// scheduled callbacks already ran,
		// we can just execute new one right now
		aOnLoadCB(this);
		// Note: it's okay if above call prolongs mutex read-lock 
		// since nothing should rely on it at this point
	}
}

void Resource::SetReady()
{
	// we only care about synchronizing Ready state to guarantee
	// that the OnLoad callbacks will be executed
	tbb::spin_rw_mutex::scoped_lock lock(myStateMutex);
	myState = State::Ready;
}

void Resource::SetErrMsg(std::string&& anErrString)
{
	myState = State::Error;
#ifdef _DEBUG
	myErrString = std::move(anErrString);
	printf("[Error] %s: %s\n", myPath.c_str(), myErrString.c_str());
#endif
}

void Resource::Load(AssetTracker& anAssetTracker)
{
	ASSERT_STR(myPath.size(), "Empty path during resource load!");
	ASSERT_STR(myState == State::Uninitialized, "Double load detected!");
	
	const bool needsRawRes = LoadResDescriptor(anAssetTracker, myPath);
	if (needsRawRes)
	{
		File file(myPath);
		if (!file.Read())
		{
			SetErrMsg("Failed to read file!");
			return;
		}
		OnLoad(anAssetTracker, file);
	}

	if (myState == State::Error)
	{
		return;
	}

	// The order of the following locks/execution is important:
	// we do the status change first to ensure we can reliably
	// execute both the scheduled callbacks as well as the ones
	// that are about to be scheduled
	SetReady();
	{
		tbb::spin_mutex::scoped_lock lock(myLoadCBMutex);
		if (myOnLoadCBs.size())
		{
			for (const Callback& loadCB : myOnLoadCBs)
			{
				loadCB(this);
			}
			myOnLoadCBs.clear();
			myOnLoadCBs.shrink_to_fit();
		}
	}
}