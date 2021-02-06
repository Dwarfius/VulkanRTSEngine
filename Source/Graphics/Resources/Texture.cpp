#include "Precomp.h"
#include "Texture.h"

#include <Core/File.h>
#include <Core/Resources/Serializer.h>
#include <STB_Image/stb_image.h>

Handle<Texture> Texture::LoadFromDisk(const std::string& aPath)
{
	int width = 0;
	int height = 0;
	int actualChannels = 0;
	int desiredChannels = STBI_default;
	unsigned char* data = stbi_load(aPath.c_str(), &width, &height, &actualChannels, STBI_default);
	ASSERT_STR(data, "FAiled to load texture from file: %s", aPath.c_str());

	Format format = Format::UNorm_RGB;
	switch (actualChannels)
	{
	case STBI_grey: format = Format::UNorm_R; break;
	case STBI_grey_alpha: format = Format::UNorm_RG; break;
	case STBI_rgb:	format = Format::UNorm_RGB; break;
	case STBI_rgb_alpha: format = Format::UNorm_RGBA; break;
	default: ASSERT(false);
	}

	Handle<Texture> textureHandle = new Texture();
	Texture* texture = textureHandle.Get();
	texture->SetFormat(format);
	texture->SetHeight(height);
	texture->SetWidth(width);
	texture->SetPixels(data);
	texture->myIsSTBIBuffer = true;
	return textureHandle;
}

Handle<Texture> Texture::LoadFromMemory(const unsigned char* aBuffer, size_t aLength)
{
	int width = 0;
	int height = 0;
	int actualChannels = 0;
	int desiredChannels = STBI_default;
	ASSERT_STR(aLength > std::numeric_limits<int>::max(),
		"Bbuffer too large, STBI can't parse it!");
	unsigned char* data = stbi_load_from_memory(aBuffer, static_cast<int>(aLength), &width, &height, &actualChannels, STBI_default);
	ASSERT_STR(data, "Failed to load texture from memory!");

	Format format = Format::UNorm_RGB;
	switch (actualChannels)
	{
	case STBI_grey: format = Format::UNorm_R; break;
	case STBI_grey_alpha: format = Format::UNorm_RG; break;
	case STBI_rgb:	format = Format::UNorm_RGB; break;
	case STBI_rgb_alpha: format = Format::UNorm_RGBA; break;
	default: ASSERT(false);
	}

	Handle<Texture> textureHandle = new Texture();
	Texture* texture = textureHandle.Get();
	texture->SetFormat(format);
	texture->SetHeight(height);
	texture->SetWidth(width);
	texture->SetPixels(data);
	texture->myIsSTBIBuffer = true;
	return textureHandle;
}

Texture::Texture(Resource::Id anId, const std::string& aPath)
	: Resource(anId, aPath)
{
}

Texture::~Texture()
{
	if (myPixels)
	{
		FreePixels();
	}
}

void Texture::SetPixels(unsigned char* aPixels, bool aShouldOwn /* = true */)
{
	ASSERT_STR(GetId() == Resource::InvalidId, "NYI. Textures from disk are currently static due to FreePixels implementation.");
	if (myPixels)
	{
		FreePixels();
	}
	myPixels = aPixels;
	myOwnsBuffer = aShouldOwn;
	myIsSTBIBuffer = false;
}

void Texture::FreePixels()
{
	ASSERT_STR(myPixels, "Double free of texture!");
	if (myOwnsBuffer)
	{
		if (myIsSTBIBuffer)
		{
			stbi_image_free(myPixels);
		}
		else
		{
			// TODO: unsafe assumption that new[] was used
			delete[] myPixels;
		}
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

	myOwnsBuffer = true;
	myIsSTBIBuffer = true;
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