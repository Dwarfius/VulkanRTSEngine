#pragma once

#ifdef _DEBUG
#include <Core/Threading/AssertRWMutex.h>
#endif

#include <Core/Utils.h>
#include <Core/RefCounted.h>

class UniformBlock;
class GPUBuffer;
class Camera;
class Graphics;
class Descriptor;

// For Global adapters
struct AdapterSourceData
{
	const Graphics& myGraphics;
};

// For per-camera adapters
struct CameraAdapterSourceData : AdapterSourceData
{
	const Camera& myCam;
};

class UniformAdapter
{
public:
	using FillUBCallback = void(*)(const AdapterSourceData& aData, UniformBlock& aUB);
	
	UniformAdapter(std::string_view aName, FillUBCallback aUBCallback, const Descriptor& aDesc, bool aIsGlobal)
		: myName(aName)
		, myUBFiller(aUBCallback)
		, myDescriptor(aDesc)
		, myIsGlobal(aIsGlobal)
	{
	}

	void Fill(const AdapterSourceData& aData, UniformBlock& aUB) const
	{
		myUBFiller(aData, aUB);
	}

	const Descriptor& GetDescriptor() const { return myDescriptor; }
	std::string_view GetName() const { return myName; }
	bool IsGlobal() const { return myIsGlobal; }

	void CreateGlobalUBO(Graphics& aGraphics);
	// TODO: find a way to avoid copies here
	Handle<GPUBuffer> GetGlobalUBO() const { return myGlobalUBO; }
	// TODO: find a way to just expose a mutable reference to avoid this API
	void ReleaseUBO() { myGlobalUBO = Handle<GPUBuffer>(); }

private:
	const FillUBCallback myUBFiller;
	const Descriptor& myDescriptor;
	const std::string_view myName;
	const bool myIsGlobal;
	Handle<GPUBuffer> myGlobalUBO;
};

// A Singleton class used for tracking all the uniform adapters
// that can be used to bridge the game state and rendering
class UniformAdapterRegister
{
public:
	static UniformAdapterRegister& GetInstance()
	{
		static UniformAdapterRegister adapterRegister;
		return adapterRegister;
	}

	const UniformAdapter& GetAdapter(std::string_view aName) const;

	// Before adapters can be fetched, they must be registered
	template<class Type>
	void Register(std::string_view aName, bool aIsGlobal)
	{
#ifdef _DEBUG
		AssertWriteLock lock(myRegisterMutex);
#endif
		UniformAdapter::FillUBCallback adapterCallback = &Type::FillUniformBlock;
		const Descriptor& desc = Type::ourDescriptor;
		myAdapters.insert({ aName, { aName, adapterCallback, desc, aIsGlobal } });
	}

	template<class TFunc>
	void ForEach(const TFunc& aFunc)
	{
#ifdef _DEBUG
		AssertReadLock lock(myRegisterMutex);
#endif
		// Not the most efficient, but due to low count
		// it's negligible 
		for (auto& [name, adapter] : myAdapters)
		{
			aFunc(adapter);
		}
	}
	
private:
	std::unordered_map<std::string_view, UniformAdapter> myAdapters;
#ifdef _DEBUG
	mutable AssertRWMutex myRegisterMutex;
#endif
};

template<class CRTP, bool kIsGlobal = false>
class RegisterUniformAdapter
{
private:
	static bool Register()
	{
		UniformAdapterRegister::GetInstance().Register<CRTP>(Utils::NameOf<CRTP>, kIsGlobal);
		return true;
	}

	static const bool ourIsRegistered; 

	// This forces the instantiation of ourIsRegistered,
	// which in turn forces the static initialization to happen
	// as it has "side-effects". Having said so, not sure why
	// a type-alias doesn't get ignored since it's not used
	template<const bool&>
	struct InstantiateForcer {};
	using Force = InstantiateForcer<ourIsRegistered>;
};

template<class CRTP, bool kIsGlobal>
const bool RegisterUniformAdapter<CRTP, kIsGlobal>::ourIsRegistered = RegisterUniformAdapter::Register();