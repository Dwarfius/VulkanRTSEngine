#include "Precomp.h"
#include "Resource.h"

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
	, myOnLoadCB(nullptr)
	, myOnUploadCB(nullptr)
	, myOnDestroyCB(nullptr)
{
}

Resource::~Resource()
{
	myOnDestroyCB(this);
}

const vector<Handle<Resource>>& Resource::GetDependencies() const
{
	return myDependencies;
}

void Resource::SetErrMsg(string&& anErrString)
{
	myState = State::Error;
#ifdef _DEBUG
	myErrString = move(anErrString);
	printf("[Error] %s: %s\n", myPath.c_str(), myErrString.c_str());
#endif
}

void Resource::Load()
{
	OnLoad();
	if (myOnLoadCB)
	{
		myOnLoadCB(this);
	}
}

void Resource::Upload(GPUResource* aResource)
{
	OnUpload(aResource);
	if (myOnUploadCB)
	{
		myOnUploadCB(this);
	}
}

void Resource::Unload()
{
	myGPUResource->Unload();
	myGPUResource = nullptr;
}