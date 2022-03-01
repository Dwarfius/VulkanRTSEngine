#pragma once

#include <Graphics/Resources/GPUPipeline.h>

class UniformBufferGL;

class PipelineGL final : public GPUPipeline
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

	bool AreDependenciesValid() const override;

private:
	// Changes OpenGL state, not thread safe.
	void OnCreate(Graphics& aGraphics) override;
	bool OnUpload(Graphics& aGraphics) override;
	void OnUnload(Graphics& aGraphics) override;

#ifdef _DEBUG
	bool AreUBOsValid();
#endif

	uint32_t myGLProgram;
	std::vector<uint32_t> mySamplerUniforms;
	std::vector<uint32_t> mySamplerTypes; 

	std::vector<Handle<UniformBufferGL>> myBuffers;
};