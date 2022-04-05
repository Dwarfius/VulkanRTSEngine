#pragma once

#include <Graphics/Resources/GPUPipeline.h>

class PipelineGL final : public GPUPipeline
{
public:
	PipelineGL();
	~PipelineGL();

	// Only binds the GL program, and sends the sampler
	// uniforms. Caller must bing UBOs manually.
	// Changes OpenGL state, not thread safe.
	void Bind();

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
};