#pragma once

#include "Resource.h"

// Base class for creating specialized Textures
class Texture : public Resource
{
public:
	enum class Type
	{
		Color,
		DepthStencil
	};

	// TODO: split this into SourceFormat and DestFormat
	enum class Format
	{
		// Generic
		// 8 byte per component
		UNorm_R, 
		UNorm_RG, 
		UNorm_RGB,
		UNorm_RGBA,

		// Special
		UNorm_BGRA // VK relies on this for swapchain initialization
	};

	struct CreateDescriptor
	{
		GPUResource::WrapMode myWrapMode;
		GPUResource::Filter myMinFilter;
		GPUResource::Filter myMagFilter;
	};

	struct UploadDescriptor
	{
		bool myEnableMipmaps;
		Format myFormat;
		int myWidth;
		int myHeight;
		const void* myPixels;
	};

	static unsigned char* LoadFromDisk(const std::string& aPath, Format aFormat, int& aWidth, int& aHeight);
	static unsigned short* LoadFromDisk16(const std::string& aPath, Format aFormat, int& aWidth, int& aHeight);
	static void FreePixels(unsigned char* aBuffer);
	static void FreePixels(unsigned short* aBuffer);

public:
	Texture(Resource::Id anId);
	Texture(Resource::Id anId, const std::string& aPath);

	Resource::Type GetResType() const override { return Resource::Type::Texture; }

	Type GetType() const { return myType; }
	Format GetFormat() const { return myFormat; }
	int GetWidth() const { return myWidth; }
	int GetHeight() const { return myHeight; }

protected:
	Type myType;
	Format myFormat;
	int myWidth;
	int myHeight;
	unsigned char* myPixels;

	void FreeTextMem();

private:
	void OnLoad(AssetTracker& anAssetTracker) override;
	void OnUpload(GPUResource* aGPUResource) override;
};