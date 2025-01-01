#pragma once

#include <Graphics/Resources/GPUTexture.h>

class Texture;

class TextureGL final : public GPUTexture
{
public:
	void BindTexture();
	void BindImage(uint32_t aSlot, uint8_t aLevel, uint32_t anAccessType);

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
	Format myFormat = static_cast<Format>(-1);
	WrapMode myWrapMode = static_cast<WrapMode>(-1);
	Filter myMinFilter = static_cast<Filter>(-1);
	Filter myMagFilter = static_cast<Filter>(-1);
};