#pragma once

#ifdef _DEBUG
#include <Core/Threading/AssertRWMutex.h>
#endif

class UniformBlock;
class Camera;
class Graphics;
class Descriptor;

struct AdapterSourceData
{
	const Graphics& myGraphics;
	const Camera& myCam;
};

class UniformAdapter
{
public:
	using FillUBCallback = void(*)(const AdapterSourceData& aData, UniformBlock& aUB);
	
	UniformAdapter(std::string_view aName, FillUBCallback aUBCallback, const Descriptor& aDesc)
		: myName(aName)
		, myUBFiller(aUBCallback)
		, myDescriptor(aDesc)
	{
	}

	void Fill(const AdapterSourceData& aData, UniformBlock& aUB) const
	{
		myUBFiller(aData, aUB);
	}

	const Descriptor& GetDescriptor() const { return myDescriptor; }
	std::string_view GetName() const { return myName; }

private:
	const FillUBCallback myUBFiller;
	const Descriptor& myDescriptor;
	const std::string_view myName;
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
	void Register(std::string_view aName)
	{
#ifdef _DEBUG
		AssertWriteLock lock(myRegisterMutex);
#endif
		UniformAdapter::FillUBCallback adapterCallback = &Type::FillUniformBlock;
		const Descriptor& desc = Type::ourDescriptor;
		myAdapters.insert({ aName, { aName, adapterCallback, desc } });
	}
	
private:
	std::unordered_map<std::string_view, UniformAdapter> myAdapters;
#ifdef _DEBUG
	mutable AssertRWMutex myRegisterMutex;
#endif
};

template<class CRTP>
class RegisterUniformAdapter
{
private:
	static bool Register()
	{
		UniformAdapterRegister::GetInstance().Register<CRTP>(Utils::NameOf<CRTP>);
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

template<class CRTP>
const bool RegisterUniformAdapter<CRTP>::ourIsRegistered = RegisterUniformAdapter::Register();