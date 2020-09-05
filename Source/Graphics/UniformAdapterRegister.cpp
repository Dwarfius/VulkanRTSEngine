#include "Precomp.h"
#include "UniformAdapterRegister.h"

#include "UniformAdapter.h"

std::reference_wrapper<const UniformAdapter> UniformAdapterRegister::GetAdapter(const std::string& aName) const
{
#ifdef _DEBUG
	AssertReadLock lock(myRegisterMutex);
#endif

	const auto iter = myAdapters.find(aName);
	ASSERT_STR(iter != myAdapters.end(), "Requested unregistered adapter!");
	return std::cref(*iter->second);
}