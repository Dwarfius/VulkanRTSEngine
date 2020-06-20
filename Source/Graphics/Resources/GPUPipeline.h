#pragma once

#include "../GPUResource.h"
#include "../Descriptor.h"
#include "../Interfaces/IPipeline.h"

class GPUPipeline : public GPUResource, public IPipeline
{
public:
	size_t GetDescriptorCount() const override final { return myDescriptors.size(); }
	Handle<Descriptor> GetDescriptor(size_t anIndex) const override final { return myDescriptors[anIndex]; }

protected:
	std::vector<Handle<Descriptor>> myDescriptors;
};