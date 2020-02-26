#pragma once

#include <Core/RefCounted.h>
#include <Core/StaticString.h>

class AssetTracker;
class File;

// Base class for resources. Disables copying. All resources must be loaded from disk.
class Resource : public RefCounted
{
	using Callback = std::function<void(const Resource*)>;

public:
	using Id = uint32_t;

	constexpr static Id InvalidId = 0;
	constexpr static StaticString AssetsFolder = "../assets/";

	enum class State
	{
		Error,
		Uninitialized,
		PendingLoad,
		Ready
	};

	enum class Type
	{
		Model,
		Texture,
		Pipeline,
		Shader
	};

	virtual Type GetResType() const = 0;

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

	// TODO: add DebugAsserts for scheduling callbacks during AssetTracker::Process time
	// Sets the callback to call when the object finishes loading from disk
	void ExecLambdaOnLoad(Callback aOnLoadCB);
	// Sets the callback to call when the object gets destroyed
	void AddOnDestroyCB(Callback aOnDestroyCB) { myOnDestroyCBs.push_back(aOnDestroyCB); }

protected:
	void SetReady();
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

	// Loads a raw resource (png, obj, etc)
	virtual void OnLoad(AssetTracker& assetTracker, const File& aFile) = 0;
	// Loads a resource descriptor. Returns true if an actual resource 
	// load is needed, false otherwise.
	virtual bool LoadResDescriptor(AssetTracker& anAssetTracker, std::string& aPath) { return true; }

#ifdef _DEBUG
	// used for tracking what went wrong
	std::string myErrString;
#endif

	std::string myPath;
	std::vector<Callback> myOnLoadCBs;
	std::vector<Callback> myOnDestroyCBs;
	Id myId;
	State myState;

	tbb::spin_rw_mutex myStateMutex;
	tbb::spin_mutex myLoadCBMutex;
};
