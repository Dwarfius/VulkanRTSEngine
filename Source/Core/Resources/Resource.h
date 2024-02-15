#pragma once

#include "../RefCounted.h"
#include "../StaticString.h"

class File;
class Serializer;

// Base class for resources that can be loaded from disk. Disables copying. 
class Resource : public RefCounted
{
public:
	using Id = uint32_t;
	using Callback = std::function<void(Resource*)>;

	constexpr static Id InvalidId = 0;
	constexpr static StaticString kAssetsFolder = "../assets/";

	enum class State
	{
		Error,
		Uninitialized,
		PendingLoad,
		Ready
	};

public:
	// creates a dynamic resource
	Resource();
	// creates a resource from a file
	Resource(Id anId, std::string_view aPath);
	~Resource();

	// We disable copies to avoid accidental resource duplication
	Resource(const Resource& anOther) = delete;
	Resource& operator=(const Resource& anOther) = delete;

	Id GetId() const { return myId; }
	const std::string& GetPath() const { return myPath; }
	std::string_view GetName() const;
	
	// Not synchronized, might be out of date for a frame
	State GetState() const { return myState; }

	const std::vector<Handle<Resource>>& GetDependencies() const { return myDependencies; }

	// Sets the callback to call when the object finishes loading from disk
	// or executes it immediately if object is loaded. Guarantees queued
	// order of execution.
	void ExecLambdaOnLoad(const Callback& aOnLoadCB);

protected:
	// A convinience wrapper to set the error message in debug builds.
	// Sets the state to Error
	void SetErrMsg(std::string&& anErrString);

	std::vector<Handle<Resource>> myDependencies;

private:
	// ============================
	// AssetTracker support
	friend class AssetTracker;
	void Load(AssetTracker& anAssetTracker, Serializer& aSerializer);
	void Save(AssetTracker& anAssetTracker, Serializer& aSerializer);
	// ============================

	// Controls whether it should be using textual serialization or binary
	virtual bool PrefersBinarySerialization() const { return false; }
	virtual void Serialize(Serializer& aSerializer) = 0;

#ifdef _DEBUG
	// used for tracking what went wrong
	std::string myErrString;
#endif

	std::string myPath;
	std::vector<Callback> myOnLoadCBs;
	Callback myOnDestroyCB;
	Id myId;
	State myState;

	tbb::queuing_mutex myStateMutex;
};
