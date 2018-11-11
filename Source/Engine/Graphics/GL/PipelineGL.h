#pragma once

#include "Graphics/Pipeline.h"

class UniformBufferGL;

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

	UniformBufferGL* GetUBO() const { return myBuffer; }

private:
	uint32_t myGLProgram;
	vector<uint32_t> mySamplerUniforms;
	vector<uint32_t> mySamplerTypes;

	// TODO: extend to support multiple buffers
	UniformBufferGL* myBuffer;
};