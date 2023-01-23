#include "Precomp.h"
#include "UniformAdapterRegister.h"

#include "Graphics.h"

void UniformAdapter::CreateGlobalUBO(Graphics& aGraphics)
{
	ASSERT_STR(myIsGlobal, "Non-global adaptors don't need a UBO!");
	ASSERT_STR(!myGlobalUBO.IsValid(), "Already initialized the global UBO!");
	myGlobalUBO = aGraphics.CreateUniformBuffer(myDescriptor.GetBlockSize());
}

const UniformAdapter& UniformAdapterRegister::GetAdapter(std::string_view aName) const
{
#ifdef _DEBUG
	AssertReadLock lock(myRegisterMutex);
#endif

	const auto iter = myAdapters.find(aName);
	ASSERT_STR(iter != myAdapters.end(), "Requested unregistered adapter: %s", aName.data());
	return iter->second;
}