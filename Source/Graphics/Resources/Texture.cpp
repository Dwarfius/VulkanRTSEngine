#include "Precomp.h"
#include "Texture.h"

#include <nlohmann/json.hpp>
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

bool Texture::LoadResDescriptor(AssetTracker& anAssetTracker, std::string& aPath)
{
	ASSERT_STR(aPath.length() >= 4, "Invalid name passed!");
	using json = nlohmann::json;

	std::string ext = aPath.substr(aPath.length() - 4);
	const bool isDescriptor = ext.compare("desc") == 0;
	if (isDescriptor)
	{
		File descFile(aPath);
		if (!descFile.Read())
		{
			SetErrMsg("Failed to read descriptor!");
			return false;
		}
		
		const json jsonObj = json::parse(descFile.GetBuffer(), nullptr, false);
		{
			const json& formatHandle = jsonObj["format"];
			if (!formatHandle.is_null())
			{
				myFormat = formatHandle.get<Format>();
			}
		}

		{
			const json& wrapModeHandle = jsonObj["wrapMode"];
			if (!wrapModeHandle.is_null())
			{
				myWrapMode = wrapModeHandle.get<WrapMode>();
			}
		}

		{
			const json& minFilterHandle = jsonObj["minFilter"];
			if (!minFilterHandle.is_null())
			{
				myMinFilter = minFilterHandle.get<Filter>();
			}
		}

		{
			const json& magFilterHandle = jsonObj["magFilter"];
			if (!magFilterHandle.is_null())
			{
				myMagFilter = magFilterHandle.get<Filter>();
			}
		}

		{
			const json& enableMipMapsHandle = jsonObj["enableMipMaps"];
			if (!enableMipMapsHandle.is_null())
			{
				myEnableMipmaps = enableMipMapsHandle.get<bool>();
			}
		}

		{
			const json& pathHandle = jsonObj["path"];
			if (pathHandle.is_null())
			{
				SetErrMsg("Texture missing 'path'!");
				return false;
			}
			aPath = pathHandle.get<std::string>();
		}
	}
	return true;
}