#include "Precomp.h"
#include "Resource.h"

// TODO: move it to IO
bool Resource::ReadFile(const string& aPath, string& aContents)
{
	// opening at the end allows us to know size quickly
	ifstream file(aPath, ios::ate | ios::binary);
	if (!file.is_open())
	{
		return false;
	}

	size_t size = file.tellg();
	aContents.resize(size);
	file.seekg(0);
	file.read(&aContents[0], size);
	file.close();
	return true;
}

Resource::Resource(Id anId)
	: Resource(anId, "")
{
}

Resource::Resource(Id anId, const string& aPath)
	: myId(anId)
	, myState(State::Invalid)
	, myPath(aPath)
	, myGPUResource(nullptr)
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

void Resource::SetErrMsg(string&& anErrString)
{
	myState = State::Error;
#ifdef _DEBUG
	myErrString = move(anErrString);
	printf("[Error] %s: %s\n", myPath.c_str(), myErrString.c_str());
#endif
}

void Resource::Load(AssetTracker& anAssetTracker)
{
	OnLoad(anAssetTracker);
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

void Resource::Upload(GPUResource* aResource)
{
	OnUpload(aResource);
	if (myOnUploadCBs.size())
	{
		for (const Callback& uploadCB : myOnUploadCBs)
		{
			uploadCB(this);
		}
		myOnUploadCBs.clear();
		myOnUploadCBs.shrink_to_fit();
	}
}

void Resource::Unload()
{
	myGPUResource->Unload();
	myGPUResource = nullptr;
}