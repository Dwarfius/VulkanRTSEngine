#include "Precomp.h"
#include "Resource.h"

// TODO: move it to IO
bool Resource::ReadFile(const std::string& aPath, std::string& aContents)
{
	// opening at the end allows us to know size quickly
	std::ifstream file(aPath, std::ios::ate | std::ios::binary);
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

Resource::Resource(Id anId, const std::string& aPath)
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