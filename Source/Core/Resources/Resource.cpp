#include "Precomp.h"
#include "Resource.h"

#include "AssetTracker.h"
#include "File.h"
#include "Resources/Serializer.h"

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

std::string_view Resource::GetName() const
{
	ASSERT(!myPath.empty());
	const std::string_view path = myPath;
	const size_t end = path.rfind('.');
	const size_t start = path.rfind('/', end) + 1;
	return path.substr(start, end - start);
}

void Resource::ExecLambdaOnLoad(const Callback& aOnLoadCB)
{
	tbb::queuing_mutex::scoped_lock lockState(myStateMutex);
	if (myState != State::Ready)
	{
		// we have to delay it till OnLoad runs
		myOnLoadCBs.push_back(aOnLoadCB);
	}
	else
	{
		// scheduled callbacks already ran,
		// we can just execute new one right now
		aOnLoadCB(this);
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

void Resource::Load(AssetTracker& anAssetTracker, Serializer& aSerializer)
{
	ASSERT_STR(myPath.size(), "Empty path during resource load!");
	ASSERT_STR(myState == State::Uninitialized, "Double load detected!");
	
	File file(myPath);
	if (!file.Read())
	{
		SetErrMsg("Failed to read file!");
		return;
	}
	const std::vector<char>& buffer = file.GetBuffer();
	aSerializer.ReadFrom(buffer);
	Serialize(aSerializer);

	if (myState == State::Error)
	{
		return;
	}

	// Either execute the callbacks now, or get in queue for when the
	// callback schedulign finishes
	tbb::queuing_mutex::scoped_lock lockState(myStateMutex);
	myState = State::Ready;
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

void Resource::Save(AssetTracker& anAssetTracker, Serializer& aSerializer)
{
	ASSERT_STR(myPath.size(), "Empty path during resource save!");
	ASSERT_STR(myState == State::Ready, "Must be fully loaded in to save!");

	Serialize(aSerializer);

	std::vector<char> buffer;
	aSerializer.WriteTo(buffer);
	
	File file(myPath, std::move(buffer));
	bool success = file.Write();
	ASSERT_STR(success, "Failed to write a file!");
}