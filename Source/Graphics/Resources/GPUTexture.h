#pragma once

#include "../GPUResource.h"
#include "../Interfaces/ITexture.h"

class GPUTexture : public GPUResource, public ITexture
{
public:
	glm::vec2 GetSize() const { return mySize; }

protected:
	glm::vec2 mySize;
};