#pragma once

#include <Core/Resources/Resource.h>
#include "../Interfaces/ITexture.h"

// Base class for creating specialized Textures
class Texture : public Resource, public ITexture
{
public:
	static constexpr StaticString kDir = Resource::AssetsFolder + "textures/";

	static unsigned char* LoadFromDisk(const std::string& aPath, Format aFormat, int& aWidth, int& aHeight);
	static unsigned short* LoadFromDisk16(const std::string& aPath, Format aFormat, int& aWidth, int& aHeight);
	static void FreePixels(unsigned char* aBuffer);
	static void FreePixels(unsigned short* aBuffer);

public:
	Texture();
	Texture(Resource::Id anId, const std::string& aPath);
	~Texture();

	Resource::Type GetResType() const override final { return Resource::Type::Texture; }

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
	void SetPixels(unsigned char* aPixels);

protected:
	unsigned char* myPixels;
	uint32_t myWidth;
	uint32_t myHeight;
	Format myFormat;
	WrapMode myWrapMode;
	Filter myMinFilter;
	Filter myMagFilter;
	bool myEnableMipmaps;

private:
	void FreePixels();

	bool UsesDescriptor() const override final;
	void OnLoad(const File& aFile) override;
	void Serialize(Serializer& aSerializer) override final;
};