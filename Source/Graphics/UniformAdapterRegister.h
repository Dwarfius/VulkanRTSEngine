#pragma once

#ifdef _DEBUG
#include <Core/Threading/AssertRWMutex.h>
#endif

class UniformBlock;
class Camera;
class Graphics;

struct AdapterSourceData
{
	const Graphics& myGraphics;
	const Camera& myCam;
};

// A Singleton class used for tracking all the uniform adapters
// that can be used to bridge the game state and rendering
class UniformAdapterRegister
{
public:
	using FillUBCallback = void(*)(const AdapterSourceData& aData, UniformBlock& aUB);

	static UniformAdapterRegister& GetInstance()
	{
		static UniformAdapterRegister adapterRegister;
		return adapterRegister;
	}

	FillUBCallback GetAdapter(std::string_view aName) const;

	// Before adapters can be fetched, they must be registered
	template<class Type>
	void Register()
	{
#ifdef _DEBUG
		AssertWriteLock lock(myRegisterMutex);
#endif
		std::string_view name = Type::kName;
		FillUBCallback adapterCallback = &Type::FillUniformBlock;
		myAdapters.insert({ name, adapterCallback });
	}
	
private:
	std::unordered_map<std::string_view, FillUBCallback> myAdapters;
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
		UniformAdapterRegister::GetInstance().Register<CRTP>();
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