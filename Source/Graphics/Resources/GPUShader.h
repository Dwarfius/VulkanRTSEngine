#pragma once

#include "../GPUResource.h"
#include "../Interfaces/IShader.h"

class GPUShader : public GPUResource, public IShader
{
public:
	std::string_view GetTypeName() const final { return "Shader"; }
};