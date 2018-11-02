#pragma once

#include "Resource.h"

// Simple interface that allows creation of GPU resources
class IGPUAllocator
{
public:
	virtual GPUResource* Create(Resource::Type aType) const = 0;
};