#include "Precomp.h"
#include "Texture.h"

#include <Core/File.h>
#include <stb_image.h>

unsigned char* Texture::LoadFromDisk(const std::string& aPath, Format aFormat, int& aWidth, int& aHeight)
{
	int desiredChannels = STBI_default;
	switch (aFormat)
	{
	case Format::UNorm_R: desiredChannels = STBI_grey; break;
	case Format::UNorm_RG: desiredChannels = STBI_grey_alpha; break;
	case Format::UNorm_RGB:	desiredChannels = STBI_rgb; break;
	case Format::UNorm_RGBA: // fallthrough
	case Format::UNorm_BGRA: desiredChannels = STBI_rgb_alpha; break;
	default: ASSERT(false);
	}

	int actualChannels = 0;
	unsigned char* data = stbi_load(aPath.c_str(), &aWidth, &aHeight, &actualChannels, desiredChannels);;
	return data;
}

unsigned short* Texture::LoadFromDisk16(const std::string& aPath, Format aFormat, int& aWidth, int& aHeight)
{
	int desiredChannels = STBI_default;
	switch (aFormat)
	{
	case Format::UNorm_R: desiredChannels = STBI_grey; break;
	case Format::UNorm_RG: desiredChannels = STBI_grey_alpha; break;
	case Format::UNorm_RGB:	desiredChannels = STBI_rgb; break;
	case Format::UNorm_RGBA: // fallthrough
	case Format::UNorm_BGRA: desiredChannels = STBI_rgb_alpha; break;
	default: ASSERT(false);
	}

	int actualChannels = 0;
	return stbi_load_16(aPath.c_str(), &aWidth, &aHeight, &actualChannels, desiredChannels);
}

void Texture::FreePixels(unsigned char* aBuffer)
{
	stbi_image_free(aBuffer);
}

void Texture::FreePixels(unsigned short* aBuffer)
{
	stbi_image_free(aBuffer);
}

Texture::Texture()
	: myPixels(nullptr)
	, myFormat(Format::UNorm_RGB)
	, myWrapMode(WrapMode::Clamp)
	, myMinFilter(Filter::Linear)
	, myMagFilter(Filter::Linear)
	, myWidth(0)
	, myHeight(0)
	, myEnableMipmaps(false)
{
}

Texture::Texture(Resource::Id anId, const std::string& aPath)
	: Resource(anId, kDir.CStr() + aPath)
	, myPixels(nullptr)
	, myFormat(Format::UNorm_RGB)
	, myWrapMode(WrapMode::Clamp)
	, myMinFilter(Filter::Linear)
	, myMagFilter(Filter::Linear)
	, myWidth(0)
	, myHeight(0)
	, myEnableMipmaps(false)
{
}

void Texture::FreeTextMem()
{
	ASSERT_STR(myPixels, "Attempted to double-free pixels");
	FreePixels(myPixels);
	myPixels = nullptr;
}

void Texture::OnLoad(AssetTracker& anAssetTracker, const File& aFile)
{
	const stbi_uc* buffer = reinterpret_cast<const stbi_uc*>(aFile.GetCBuffer());
	int desiredChannels = STBI_default;
	switch (myFormat)
	{
	case Format::UNorm_R: desiredChannels = STBI_grey; break;
	case Format::UNorm_RG: desiredChannels = STBI_grey_alpha; break;
	case Format::UNorm_RGB:	desiredChannels = STBI_rgb; break;
	case Format::UNorm_RGBA: // fallthrough
	case Format::UNorm_BGRA: desiredChannels = STBI_rgb_alpha; break;
	}

	int actualChannels = 0;
	myPixels = stbi_load_from_memory(buffer, static_cast<int>(aFile.GetSize()), 
										&myWidth, &myHeight, 
										&actualChannels, desiredChannels);
	if (!myPixels)
	{
		SetErrMsg("Failed to load texture");
		return;
	}
}

bool Texture::LoadResDescriptor(AssetTracker& anAssetTracker, std::string& aPath)
{
	// TODO: finish this for different texture support
	return true;
}