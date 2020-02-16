#pragma once

#include "../GPUResource.h"
#include "../Descriptor.h"
#include "../Interfaces/IPipeline.h"

class GPUPipeline : public GPUResource, public IPipeline
{
public:
	size_t GetDescriptorCount() const override final { return myDescriptors.size(); }
	const Descriptor& GetDescriptor(size_t anIndex) { return myDescriptors[anIndex]; }

protected:
	std::vector<Descriptor> myDescriptors;
};