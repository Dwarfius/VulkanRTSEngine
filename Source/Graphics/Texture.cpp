#include "Precomp.h"
#include "Texture.h"

#include <stb_image.h>

unsigned char* Texture::LoadFromDisk(const std::string& aPath, Format aFormat, int& aWidth, int& aHeight)
{
	int desiredChannels;
	switch (aFormat)
	{
	case Format::UNorm_R: desiredChannels = STBI_grey; break;
	case Format::UNorm_RG: desiredChannels = STBI_grey_alpha; break;
	case Format::UNorm_RGB:	desiredChannels = STBI_rgb; break;
	case Format::UNorm_RGBA: // fallthrough
	case Format::UNorm_BGRA: desiredChannels = STBI_rgb_alpha; break;
	}

	int actualChannels = 0;
	return stbi_load(aPath.c_str(), &aWidth, &aHeight, &actualChannels, desiredChannels);
}

void Texture::FreePixels(unsigned char* aBuffer)
{
	stbi_image_free(aBuffer);
}

Texture::Texture(Resource::Id anId)
	: Texture(anId, "")
{
}

Texture::Texture(Resource::Id anId, const std::string& aPath)
	: Resource(anId, aPath)
	, myType(Type::Color)
	, myFormat(Format::UNorm_BGRA)
	, myWidth(0)
	, myHeight(0)
	, myPixels(nullptr)
{
}

void Texture::FreeTextMem()
{
	ASSERT_STR(myPixels, "Attempted to double-free pixels");
	FreePixels(myPixels);
	myPixels = nullptr;
}

void Texture::OnLoad(AssetTracker& anAssetTracker)
{
	const std::string basePath = "assets/textures/";
	myPixels = LoadFromDisk(basePath + myPath, myFormat, myWidth, myHeight);
	if (!myPixels)
	{
		SetErrMsg("Failed to load texture");
		return;
	}

	myState = State::PendingUpload;
}

void Texture::OnUpload(GPUResource* aGPUResource)
{
	myGPUResource = aGPUResource;

	CreateDescriptor createDesc;
	createDesc.myWrapMode = GPUResource::WrapMode::Repeat;
	createDesc.myMinFilter = GPUResource::Filter::Linear;
	createDesc.myMagFilter = GPUResource::Filter::Nearest;
	myGPUResource->Create(createDesc);

	UploadDescriptor uploadDesc;
	uploadDesc.myEnableMipmaps = false;
	uploadDesc.myFormat = myFormat;
	uploadDesc.myWidth = myWidth;
	uploadDesc.myHeight = myHeight;
	uploadDesc.myPixels = myPixels;
	myGPUResource->Upload(uploadDesc);

	myState = State::Ready;
}