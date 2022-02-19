#include "Precomp.h"
#include "UniformAdapterRegister.h"

UniformAdapterRegister::FillUBCallback UniformAdapterRegister::GetAdapter(std::string_view aName) const
{
#ifdef _DEBUG
	AssertReadLock lock(myRegisterMutex);
#endif

	const auto iter = myAdapters.find(aName);
	ASSERT_STR(iter != myAdapters.end(), "Requested unregistered adapter: %s", aName.data());
	return iter->second;
}