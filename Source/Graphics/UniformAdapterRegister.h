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

// TODO: move to CameraAdapter
// For per-camera adapters
struct CameraAdapterSourceData : AdapterSourceData
{
	const Camera& myCam;
};

class UniformAdapter
{
public:
	using FillUBCallback = void(*)(const AdapterSourceData& aData, UniformBlock& aUB);
	
	UniformAdapter(std::string_view aName, uint8_t aBindpoint, FillUBCallback aUBCallback, const Descriptor& aDesc, bool aIsGlobal)
		: myName(aName)
		, myBindpoint(aBindpoint)
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
	uint8_t GetBindpoint() const { return myBindpoint; }
	bool IsGlobal() const { return myIsGlobal; }

private:
	const FillUBCallback myUBFiller;
	const Descriptor& myDescriptor;
	const std::string_view myName;
	const uint8_t myBindpoint;
	const bool myIsGlobal;
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
	void Register(bool aIsGlobal)
	{
#ifdef _DEBUG
		AssertWriteLock lock(myRegisterMutex);
#endif
		UniformAdapter::FillUBCallback adapterCallback = &Type::FillUniformBlock;
		const std::string_view name = Utils::NameOf<Type>;
		const Descriptor& desc = Type::ourDescriptor;
		const uint8_t bindpoint = Type::kBindpoint;
		myAdapters.insert({ name, { name, bindpoint, adapterCallback, desc, aIsGlobal } });
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
		UniformAdapterRegister::GetInstance().Register<CRTP>(kIsGlobal);
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