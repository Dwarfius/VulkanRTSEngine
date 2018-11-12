#pragma once

#include "RefCounted.h"

// Base class for resources on the GPU.
class GPUResource
{
public:
	enum class Usage
	{
		Static, // upload once and use
		Dynamic // can upload multiple times
	};

	enum class Primitive
	{
		Lines,
		Triangles
	};

	enum class WrapMode
	{
		Clamp,
		Repeat,
		MirroredRepeat
	};

	enum class Filter
	{
		// Mag/Min
		Nearest,
		Linear,

		// Min only
		Nearest_MipMapNearest,
		Linear_MipMapNearest,
		Nearest_MipMapLinear,
		Linear_MipMapLinear
	};

	GPUResource() {}
	GPUResource(const GPUResource& anOther) = delete;
	GPUResource& operator=(const GPUResource& anOther) = delete;
	GPUResource(GPUResource&& anOther) = default;

public:
	virtual void Create(any aDescriptor) = 0;
	virtual bool Upload(any aDescriptor) = 0;
	virtual void Unload() = 0;

#ifdef _DEBUG
	string GetErrorMsg() const { return myErrorMsg; }

protected:
	// used for tracking what went wrong
	string myErrorMsg;
#else
	string GetErrorMsg() const { return ""; }
#endif
};

// Base class for resources. Disables copying. All resources must be loaded from drive.
class Resource : public RefCounted
{
private:
	using Callback = function<void(const Resource*)>;

public:
	using Id = uint32_t;
	constexpr static Id InvalidId = 0;

	enum class State
	{
		Error,
		Invalid,
		PendingUpload,
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
	static bool ReadFile(const string& aPath, string& aContents);

public:
	// creates a dynamic resource
	Resource(Id anId);

	// creates a resource from a file
	Resource(Id anId, const string& aPath);

	~Resource();

	// We disable copies to avoid accidental resource duplication
	Resource(const Resource& anOther) = delete;
	Resource& operator=(const Resource& anOther) = delete;

	Id GetId() const { return myId; }
	const string& GetPath() const { return myPath; }

	// collects dependency Ids into the queue
	const vector<Handle<Resource>>& GetDependencies() const;

	State GetState() const { return myState; }
	void SetState(State aNewState) { myState = aNewState; }

	const GPUResource& GetGPUResource() const { return *myGPUResource; }

	// Sets the callback to call when the object finishes loading from disk
	void SetOnLoadCB(Callback aOnLoadCB) { myOnLoadCB = aOnLoadCB; }
	// Sets the callback to call when the object finishes uploading to GPU
	void SetOnUploadCB(Callback aOnUploadCB) { myOnUploadCB = aOnUploadCB; }
	// Sets the callback to call when the object gets destroyed
	void SetOnDestroyCB(Callback aOnDestroyCB) { myOnDestroyCB = aOnDestroyCB; }

protected:
	// A list of all items that this resource depended on. Will be released on destructor call
	vector<Handle<Resource>> myDependencies;

	// Current state of the resource
	State myState;
	string myPath;

	// A convinience wrapper to set the error message in debug builds.
	// Sets the state to Error
	void SetErrMsg(string&& anErrString);

#ifdef _DEBUG
	// used for tracking what went wrong
	string myErrString;
#endif

	// AssetTracker will set this for the upload
	GPUResource* myGPUResource;

private:
	// ============================
	// AssetTracker support
	friend class AssetTracker;
	void Load();
	void Upload(GPUResource* aResource);
	void Unload();
	// ============================

	// ============================
	// Graphics && AssetTracker Support
	friend class GraphicsGL;
	friend class GraphicsVK;
	GPUResource* GetGPUResource_Int() const { return myGPUResource; }
	// ============================

	// This is strictly for interacting with files!
	virtual void OnLoad() = 0;
	// This is for interacting with the GPU
	virtual void OnUpload(GPUResource* aResource) = 0;

	Id myId;
	Callback myOnLoadCB;
	Callback myOnUploadCB;
	Callback myOnDestroyCB;
};