#pragma once

#include "Graphics/Pipeline.h"

class PipelineGL : public GPUResource
{
public:
	PipelineGL();
	~PipelineGL();

	void Bind();

	// Changes OpenGL state, not thread safe.
	void Create(any aDescriptor) override;
	bool Upload(any aDescriptor) override;
	void Unload() override;

private:
	uint32_t myGLProgram;
};