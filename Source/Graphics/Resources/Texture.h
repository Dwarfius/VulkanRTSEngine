#pragma once

#include <Core/Resources/Resource.h>
#include "../Interfaces/ITexture.h"

// Base class for creating specialized Textures
class Texture : public Resource, public ITexture
{
public:
	static constexpr StaticString kDir = Resource::AssetsFolder + "textures/";

	static Handle<Texture> LoadFromDisk(const std::string& aPath);
	static Handle<Texture> LoadFromMemory(const unsigned char* aBuffer, size_t aLength);

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

protected:
	unsigned char* myPixels = nullptr;
	uint32_t myWidth = 0;
	uint32_t myHeight = 0;
	Format myFormat = Format::UNorm_RGB;
	WrapMode myWrapMode = WrapMode::Clamp;
	Filter myMinFilter = Filter::Nearest;
	Filter myMagFilter = Filter::Nearest;
	bool myEnableMipmaps = false;

private:
	void FreePixels();

	bool UsesDescriptor() const override final;
	void OnLoad(const std::vector<char>& aBuffer) override;
	void Serialize(Serializer& aSerializer) override final;

	bool myOwnsBuffer = false;
	bool myIsSTBIBuffer = false;
};