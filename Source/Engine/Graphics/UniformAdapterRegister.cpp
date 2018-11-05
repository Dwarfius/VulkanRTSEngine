#include "Precomp.h"
#include "UniformAdapterRegister.h"

#include "UniformAdapter.h"

UniformAdapterRegister* UniformAdapterRegister::GetInstance()
{
	if (!myInstance)
	{
		myInstance = new UniformAdapterRegister();
		myInstance->RegisterTypes();
	}
	return myInstance;
}

void UniformAdapterRegister::RegisterTypes()
{
	Register<UniformAdapter>();
}

shared_ptr<UniformAdapter> UniformAdapterRegister::GetAdapter
	(const string& aName, const GameObject& aGO, const VisualObject& aVO) const
{
	unordered_map<string, CreationMethod>::const_iterator iter = myCreationMethods.find(aName);
	ASSERT_STR(iter != myCreationMethods.end(), "Requested unregistered adapter!");
	const CreationMethod& createMethod = iter->second;
	return createMethod(aGO, aVO);
}