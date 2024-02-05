#pragma once

#include <Graphics/Resources/GPUPipeline.h>

class ShaderGL;

class PipelineGL final : public GPUPipeline
{
public:
	PipelineGL();

	// Only binds the GL program, and sends the sampler
	// uniforms. Caller must bing UBOs manually.
	// Changes OpenGL state, not thread safe.
	void Bind();

private:
	void Cleanup() override;

	// Changes OpenGL state, not thread safe.
	void OnCreate(Graphics& aGraphics) override;
	bool OnUpload(Graphics& aGraphics) override;
	void OnUnload(Graphics& aGraphics) override;

	bool AreDependenciesValid() const override;

#ifdef _DEBUG
	bool AreUBOsValid();
#endif

	uint32_t myGLProgram;
	std::vector<uint32_t> mySamplerUniforms;
	std::vector<uint32_t> mySamplerTypes;
	std::vector<Handle<ShaderGL>> myShaders;
};