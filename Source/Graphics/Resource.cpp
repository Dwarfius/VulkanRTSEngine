#include "Precomp.h"
#include "Resource.h"
#include <Core/File.h>

Resource::Resource()
	: myId(InvalidId)
	, myState(State::Uninitialized)
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

	if (myOnLoadCBs.size())
	{
		for (const Callback& loadCB : myOnLoadCBs)
		{
			loadCB(this);
		}
		myOnLoadCBs.clear();
		myOnLoadCBs.shrink_to_fit();
	}
	myState = State::Ready;
}