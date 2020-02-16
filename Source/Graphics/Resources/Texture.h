#pragma once

#include "../Resource.h"
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

	Resource::Type GetResType() const override { return Resource::Type::Texture; }

	Format GetFormat() const { return myFormat; }
	int GetWidth() const { return myWidth; }
	int GetHeight() const { return myHeight; }
	WrapMode GetWrapMode() const { return myWrapMode; }
	Filter GetMinFilter() const { return myMinFilter; }
	Filter GetMagFilter() const { return myMagFilter; }
	bool IsUsingMipMaps() const { return myEnableMipmaps; }
	unsigned char* GetPixels() const { return myPixels; }

protected:
	unsigned char* myPixels;
	Format myFormat;
	WrapMode myWrapMode;
	Filter myMinFilter;
	Filter myMagFilter;
	int myWidth;
	int myHeight;
	bool myEnableMipmaps;

	void FreeTextMem();

private:
	void OnLoad(AssetTracker& anAssetTracker, const File& aFile) override;
	bool LoadResDescriptor(AssetTracker& anAssetTracker, std::string& aPath) override final;
};