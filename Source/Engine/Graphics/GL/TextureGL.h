#pragma once

#include <Graphics/Resources/GPUTexture.h>

class Texture;

class TextureGL : public GPUTexture
{
public:
	TextureGL();

	void Bind();

private:
	void OnCreate(Graphics& aGraphics) override final;
	bool OnUpload(Graphics& aGraphics) override final;
	void OnUnload(Graphics& aGraphics) override final;

	void UpdateTexParams(const Texture* aTexture);

	uint32_t myGLTexture;
	WrapMode myWrapMode;
	Filter myMinFilter;
	Filter myMagFilter;
};