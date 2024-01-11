#include "Precomp.h"
#include "Texture.h"

#include <Core/File.h>
#include <Core/Resources/Serializer.h>
#include <STB_Image/stb_image.h>

Handle<Texture> Texture::LoadFromDisk(std::string_view aPath)
{
	File file(aPath);
	bool success = file.Read();
	DEBUG_ONLY(std::string path(aPath.data(), aPath.size()););
	ASSERT_STR(success, "Failed to read a file: {}", path);

	Handle<Texture> textureHandle = new Texture();
	Texture* texture = textureHandle.Get();

	int actualChannels = 0;
	texture->LoadFromMemory(file.GetCBuffer(), file.GetSize(), STBI_default, actualChannels);
	if (texture->GetState() == State::Error)
	{
		return texture;
	}
	Format format = Format::UNorm_RGB;
	switch (actualChannels)
	{
	case STBI_grey: format = Format::UNorm_R; break;
	case STBI_grey_alpha: format = Format::UNorm_RG; break;
	case STBI_rgb:	format = Format::UNorm_RGB; break;
	case STBI_rgb_alpha: format = Format::UNorm_RGBA; break;
	default: ASSERT(false);
	}

	texture->SetFormat(format);
	texture->myRawSource = file.ConsumeBuffer();

	// preserve extension
	size_t ind = aPath.rfind('.');
	if (ind != std::string::npos)
	{
		texture->myImgExtension = aPath.substr(ind + 1);
	}

	return textureHandle;
}

Handle<Texture> Texture::LoadFromMemory(const char* aBuffer, size_t aLength)
{
	Handle<Texture> textureHandle = new Texture();
	Texture* texture = textureHandle.Get();

	int actualChannels = 0;
	texture->LoadFromMemory(aBuffer, aLength, STBI_default, actualChannels);
	ASSERT(texture->GetState() != State::Error);
	Format format = Format::UNorm_RGB;
	switch (actualChannels)
	{
	case STBI_grey: format = Format::UNorm_R; break;
	case STBI_grey_alpha: format = Format::UNorm_RG; break;
	case STBI_rgb:	format = Format::UNorm_RGB; break;
	case STBI_rgb_alpha: format = Format::UNorm_RGBA; break;
	default: ASSERT(false);
	}
	
	texture->SetFormat(format);
	texture->myRawSource.resize(aLength);
	std::memcpy(texture->myRawSource.data(), aBuffer, aLength);
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

void Texture::LoadFromMemory(const char* aData, size_t aLength, int aDesiredChannels, int& aActualChannels)
{
	ASSERT_STR(aLength < std::numeric_limits<int>::max(),
		"Buffer too large, STBI can't parse it!");
	myPixels = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(aData), static_cast<int>(aLength),
		reinterpret_cast<int*>(&myWidth), reinterpret_cast<int*>(&myHeight),
		&aActualChannels, aDesiredChannels);
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
	aSerializer.Serialize("myFormat", myFormat);
	aSerializer.Serialize("myWrapMode", myWrapMode);
	aSerializer.Serialize("myMinFilter", myMinFilter);
	aSerializer.Serialize("myMagFilter", myMagFilter);
	aSerializer.Serialize("myEnableMipMaps", myEnableMipmaps);

	std::string oldExt = myImgExtension;
	aSerializer.Serialize("myImgExtension", myImgExtension);

	bool shouldLoadImg = aSerializer.IsReading() && (!myPixels || oldExt != myImgExtension);

	std::string imgFile = GetPath();
	imgFile = imgFile.replace(imgFile.size() - 3, 3, myImgExtension);
	aSerializer.SerializeExternal(imgFile, myRawSource);

	if (shouldLoadImg)
	{
		if (myPixels)
		{
			FreePixels();
		}

		const stbi_uc* buffer = reinterpret_cast<const stbi_uc*>(myRawSource.data());
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

		int actualChannels;
		LoadFromMemory(myRawSource.data(), myRawSource.size(), desiredChannels, actualChannels);
	}
}