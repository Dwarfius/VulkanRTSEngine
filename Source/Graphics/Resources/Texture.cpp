#include "Precomp.h"
#include "Texture.h"

#include <Core/File.h>
#include <Core/Resources/Serializer.h>
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
	: Resource(anId, aPath)
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

Texture::~Texture()
{
	if (myPixels)
	{
		FreePixels();
	}
}

void Texture::SetPixels(unsigned char* aPixels)
{
	ASSERT_STR(GetId() == Resource::InvalidId, "NYI. Textures from disk are currently static due to FreePixels implementation.");
	if (myPixels)
	{
		FreePixels();
	}
	myPixels = aPixels;
}

void Texture::FreePixels()
{
	ASSERT_STR(myPixels, "Double free of texture!");
	if (GetId() == Resource::InvalidId)
	{
		delete[] myPixels;
	}
	else
	{
		stbi_image_free(myPixels);
	}
	myPixels = nullptr;
}

bool Texture::UsesDescriptor() const
{
	const std::string& path = GetPath();
	ASSERT_STR(path.length() >= 4, "Invalid name passed!");
	std::string_view pathExt(path);
	pathExt = pathExt.substr(path.length() - 4);
	return pathExt.compare("desc") == 0;
}

void Texture::OnLoad(const File& aFile)
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
	default: ASSERT_STR(false, "STBI doesn't support this format!");
	}

	int actualChannels = 0;
	myPixels = stbi_load_from_memory(buffer, static_cast<int>(aFile.GetSize()), 
						reinterpret_cast<int*>(&myWidth), reinterpret_cast<int*>(&myHeight),
						&actualChannels, desiredChannels);
	if (!myPixels)
	{
		SetErrMsg("Failed to load texture");
		return;
	}
}

void Texture::Serialize(Serializer& aSerializer)
{
	aSerializer.Serialize("format", myFormat);
	aSerializer.Serialize("wrapMode", myWrapMode);
	aSerializer.Serialize("minFilter", myMinFilter);
	aSerializer.Serialize("magFilter", myMagFilter);
	aSerializer.Serialize("enableMipMaps", myEnableMipmaps);

	std::string texturePath;
	aSerializer.Serialize("path", texturePath);
	if (!texturePath.length())
	{
		SetErrMsg("Texture missing 'path'!");
		return;
	}
	File file(texturePath);
	if (!file.Read())
	{
		SetErrMsg("Failed to find texture file!");
		return;
	}
	OnLoad(file);
}