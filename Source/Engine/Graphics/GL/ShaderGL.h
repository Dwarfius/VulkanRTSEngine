#pragma once

#include <Graphics/Resources/GPUShader.h>

class ShaderGL : public GPUShader
{
public:
	ShaderGL();

	uint32_t GetShaderId() const { return myGLShader; }

private:
	void OnCreate(Graphics& aGraphics) override final;
	bool OnUpload(Graphics& aGraphics) override final;
	void OnUnload(Graphics& aGraphics) override final;

	uint32_t myGLShader;
};