#pragma once

#ifdef _DEBUG
#include <Core/Threading/AssertRWMutex.h>
#endif

class UniformAdapter;

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

	std::reference_wrapper<const UniformAdapter> GetAdapter(const std::string& aName) const;

	// Before adapters can be fetched, they must be registered
	template<class Type>
	void Register()
	{
		static_assert(sizeof(Type) == sizeof(void*), "Uniform adapter must not have any state(except vtable)!"
			" We only store 1 instance of an adapter type, meaning adapter will"
			" be shared across multiple execution contexts!");

#ifdef _DEBUG
		AssertWriteLock lock(myRegisterMutex);
#endif
		std::string name = Type::GetName();
		std::unique_ptr<UniformAdapter> adapter = std::make_unique<Type>();
		myAdapters.insert(std::make_pair(std::move(name), std::move(adapter)));
	}
	
private:
	std::unordered_map<std::string, std::unique_ptr<UniformAdapter>> myAdapters;
#ifdef _DEBUG
	mutable AssertRWMutex myRegisterMutex;
#endif
};