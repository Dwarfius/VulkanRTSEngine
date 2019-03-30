#pragma once

#include <Core/Graphics/Pipeline.h>
#include "UniformBufferGL.h"

class PipelineGL : public GPUResource
{
public:
	PipelineGL();
	~PipelineGL();

	// Only binds the GL program, and sends the sampler
	// uniforms. Caller must bing UBOs manually.
	// Changes OpenGL state, not thread safe.
	void Bind();

	// Changes OpenGL state, not thread safe.
	void Create(any aDescriptor) override;
	bool Upload(any aDescriptor) override;
	void Unload() override;

	size_t GetUBOCount() const { return myBuffers.size(); }
	UniformBufferGL& GetUBO(size_t anIndex) { return myBuffers[anIndex]; }

private:
	uint32_t myGLProgram;
	vector<uint32_t> mySamplerUniforms;
	vector<uint32_t> mySamplerTypes;

	vector<UniformBufferGL> myBuffers;
};