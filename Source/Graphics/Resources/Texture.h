#pragma once

#include <Core/Resources/Resource.h>
#include "../Interfaces/ITexture.h"

// Base class for creating specialized Textures
class Texture : public Resource, public ITexture
{
public:
	constexpr static StaticString kExtension = ".img";

	static Handle<Texture> LoadFromDisk(std::string_view aPath);
	static Handle<Texture> LoadFromMemory(const char* aBuffer, size_t aLength);

public:
	Texture() = default;
	Texture(Resource::Id anId, const std::string& aPath);
	~Texture();

	Format GetFormat() const { return myFormat; }
	void SetFormat(Format aFormat) { myFormat = aFormat; }

	uint32_t GetWidth() const { return myWidth; }
	void SetWidth(uint32_t aWidth) { myWidth = aWidth; }
	uint32_t GetHeight() const { return myHeight; }
	void SetHeight(uint32_t aHeight) { myHeight = aHeight; }

	WrapMode GetWrapMode() const { return myWrapMode; }
	void SetWrapMode(WrapMode aWrapMode) { myWrapMode = aWrapMode; }

	Filter GetMinFilter() const { return myMinFilter; }
	void SetMinFilter(Filter aMinFilter) { myMinFilter = aMinFilter; }

	Filter GetMagFilter() const { return myMagFilter; }
	void SetMagFilter(Filter aMAgFilter) { myMagFilter = aMAgFilter; }

	bool IsUsingMipMaps() const { return myEnableMipmaps; }
	void EnableMipMaps(bool aEnabled) { myEnableMipmaps = aEnabled; }

	unsigned char* GetPixels() const { return myPixels; }
	void SetPixels(unsigned char* aPixels, bool aShouldOwn = true);

private:
	void FreePixels();

	void LoadFromMemory(const char* aData, size_t aLength, int aDesiredChannels, int& aActualChannels);
	void Serialize(Serializer& aSerializer) final;

	unsigned char* myPixels = nullptr;
	uint32_t myWidth = 0;
	uint32_t myHeight = 0;
	Format myFormat = Format::UNorm_RGB;
	WrapMode myWrapMode = WrapMode::Clamp;
	Filter myMinFilter = Filter::Nearest;
	Filter myMagFilter = Filter::Nearest;
	bool myEnableMipmaps = false;
	bool myOwnsBuffer = false;
	bool myIsSTBIBuffer = false;

	std::string myImgExtension = "png";
	std::vector<char> myRawSource;
};