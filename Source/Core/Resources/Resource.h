#pragma once

#include "../RefCounted.h"
#include "../StaticString.h"

class File;
class Serializer;

// Base class for resources. Disables copying. All resources must be loaded from disk.
class Resource : public RefCounted
{
public:
	using Id = uint32_t;
	using Callback = std::function<void(Resource*)>;

	constexpr static Id InvalidId = 0;
	constexpr static StaticString AssetsFolder = "../assets/";

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
	Resource(Id anId, const std::string& aPath);
	~Resource();

	// We disable copies to avoid accidental resource duplication
	Resource(const Resource& anOther) = delete;
	Resource& operator=(const Resource& anOther) = delete;

	Id GetId() const { return myId; }
	const std::string& GetPath() const { return myPath; }
	
	// Not synchronized, might be out of date for a frame
	State GetState() const { return myState; }

	const std::vector<Handle<Resource>>& GetDependencies() const { return myDependencies; }

	// Sets the callback to call when the object finishes loading from disk
	// or executes it immediately if object is loaded. Guarantees queued
	// order of execution.
	void ExecLambdaOnLoad(const Callback& aOnLoadCB);
	// Sets the callback to call when the object gets destroyed
	void AddOnDestroyCB(const Callback& aOnDestroyCB) { myOnDestroyCBs.push_back(aOnDestroyCB); }

protected:
	// A convinience wrapper to set the error message in debug builds.
	// Sets the state to Error
	void SetErrMsg(std::string&& anErrString);

	std::vector<Handle<Resource>> myDependencies;

private:
	// ============================
	// AssetTracker support
	friend class AssetTracker;
	void Load(AssetTracker& anAssetTracker);
	// ============================

	// Determines whether this resource loads a descriptor via Serializer or a raw resorce
	virtual bool UsesDescriptor() const { return true; };
	// Loads a raw resource (png, obj, etc)
	virtual void OnLoad(const File& aFile) { ASSERT_STR(false, "Either Uses Descriptor is wrong, or this should be implemented!"); };
	// Loads a resource descriptor. Returns true if an actual resource 
	// load is needed, false otherwise.
	virtual void Serialize(Serializer& aSerializer) { ASSERT_STR(false, "Either Uses Descriptor is wrong, or this should be implemented!"); };

#ifdef _DEBUG
	// used for tracking what went wrong
	std::string myErrString;
#endif

	std::string myPath;
	std::vector<Callback> myOnLoadCBs;
	std::vector<Callback> myOnDestroyCBs;
	Id myId;
	State myState;

	tbb::queuing_mutex myStateMutex;
};
