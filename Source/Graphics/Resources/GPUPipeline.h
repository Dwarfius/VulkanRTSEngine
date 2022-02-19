#pragma once

#include "../GPUResource.h"
#include "../Descriptor.h"
#include "../Interfaces/IPipeline.h"

class GPUPipeline : public GPUResource, public IPipeline
{
public:
	size_t GetDescriptorCount() const override final { return myDescriptors.size(); }
	const Descriptor& GetDescriptor(size_t anIndex) const override final { return myDescriptors[anIndex]; }
	UniformAdapterRegister::FillUBCallback GetAdapter(size_t anIndex) const override final { return myAdapters[anIndex]; }

protected:
	std::vector<Descriptor> myDescriptors;
	std::vector<UniformAdapterRegister::FillUBCallback> myAdapters;
};