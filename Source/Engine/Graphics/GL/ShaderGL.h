#pragma once

#include "Graphics/Shader.h"

class ShaderGL : public GPUResource
{
public:
	ShaderGL();
	~ShaderGL();

	uint32_t GetShaderId() const { return myGLShader; }

	void Create(any aDescriptor) override;
	bool Upload(any aDescriptor) override;
	void Unload() override;

private:
	uint32_t myGLShader;
};