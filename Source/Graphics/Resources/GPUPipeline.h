#pragma once

#include "../GPUResource.h"
#include "../Interfaces/IPipeline.h"
#include "../UniformAdapterRegister.h"

class GPUPipeline : public GPUResource, public IPipeline
{
public:
	size_t GetAdapterCount() const final { return myAdapters.size(); }
	const UniformAdapter& GetAdapter(size_t anIndex) const final { return *myAdapters[anIndex]; }
	
	std::string_view GetTypeName() const final { return "Pipeline"; }

protected:
	std::vector<const UniformAdapter*> myAdapters;
};