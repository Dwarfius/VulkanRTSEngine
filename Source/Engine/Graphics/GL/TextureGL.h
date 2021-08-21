#pragma once

#include <Graphics/Resources/GPUTexture.h>

class Texture;

class TextureGL : public GPUTexture
{
public:
	void Bind();

	static uint32_t TranslateInternalFormat(Format aFormat);
	static uint32_t TranslateFormat(Format aFormat);
	static uint32_t DeterminePixelDataType(Format aFormat);

private:
	void OnCreate(Graphics& aGraphics) override final;
	bool OnUpload(Graphics& aGraphics) override final;
	void OnUnload(Graphics& aGraphics) override final;

	void UpdateTexParams(const Texture* aTexture);

	uint32_t myGLTexture = 0;
	WrapMode myWrapMode = static_cast<WrapMode>(-1);
	Filter myMinFilter = static_cast<Filter>(-1);
	Filter myMagFilter = static_cast<Filter>(-1);
};