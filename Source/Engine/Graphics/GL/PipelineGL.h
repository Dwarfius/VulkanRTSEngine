#pragma once

#include <Graphics/Resources/GPUPipeline.h>

class UniformBufferGL;

class PipelineGL : public GPUPipeline
{
public:
	PipelineGL();
	~PipelineGL();

	// Only binds the GL program, and sends the sampler
	// uniforms. Caller must bing UBOs manually.
	// Changes OpenGL state, not thread safe.
	void Bind();

	size_t GetUBOCount() const { return myBuffers.size(); }
	UniformBufferGL& GetUBO(size_t anIndex) { return *myBuffers[anIndex].Get(); }

	bool AreDependenciesValid() const override final;

private:
	// Changes OpenGL state, not thread safe.
	void OnCreate(Graphics& aGraphics) override final;
	bool OnUpload(Graphics& aGraphics) override final;
	void OnUnload(Graphics& aGraphics) override final;

	uint32_t myGLProgram;
	std::vector<uint32_t> mySamplerUniforms;
	std::vector<uint32_t> mySamplerTypes; 

	vector<Handle<UniformBufferGL>> myBuffers;
	
};