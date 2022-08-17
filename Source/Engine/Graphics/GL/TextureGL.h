#pragma once

#include <Graphics/Resources/GPUTexture.h>

class Texture;

class TextureGL final : public GPUTexture
{
public:
	void Bind();

	static uint32_t TranslateInternalFormat(Format aFormat);
	static uint32_t TranslateFormat(Format aFormat);
	static uint32_t DeterminePixelDataType(Format aFormat);
	static size_t GetPixelDataTypeSize(Format aFormat);

private:
	void OnCreate(Graphics& aGraphics) override;
	bool OnUpload(Graphics& aGraphics) override;
	void OnUnload(Graphics& aGraphics) override;

	void UpdateTexParams(const Texture* aTexture);

	uint32_t myGLTexture = 0;
	WrapMode myWrapMode = static_cast<WrapMode>(-1);
	Filter myMinFilter = static_cast<Filter>(-1);
	Filter myMagFilter = static_cast<Filter>(-1);
};