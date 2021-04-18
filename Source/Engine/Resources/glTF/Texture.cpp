#include "Precomp.h"
#include "Texture.h"

namespace glTF
{
	void Image::ParseItem(const nlohmann::json& anImageJson, Image& anImage)
	{
		anImage.myUri = ReadOptional(anImageJson, "uri", std::string());

		std::string mimeType = ReadOptional(anImageJson, "mimeType", std::string());
		anImage.myMimeType = Image::MimeType::None;
		if (mimeType == "image/jpeg")
		{
			anImage.myMimeType = Image::MimeType::Jpeg;
		}
		else if (mimeType == "image/png")
		{
			anImage.myMimeType = Image::MimeType::PNG;
		}
		else
		{
			ASSERT_STR(mimeType.empty(), "Unrecognized MIME type!");
		}

		anImage.myBufferView = ReadOptional(anImageJson, "bufferView", kInvalidInd);
		anImage.myName = ReadOptional(anImageJson, "name", std::string());
	}

	void Sampler::ParseItem(const nlohmann::json& aSamplerJson, Sampler& aSampler)
	{
		int magFilter = ReadOptional(aSamplerJson, "magFilter", 9728); // GL_NEAREST
		switch (magFilter)
		{
		case 9728: // GL_NEAREST
			aSampler.myMagFilter = ::Texture::Filter::Nearest;
			break;
		case 9729: // GL_LINEAR
			aSampler.myMagFilter = ::Texture::Filter::Linear;
			break;
		default:
			ASSERT_STR(false, "Unrecognized Mag filter!");
		}
		int minFilter = ReadOptional(aSamplerJson, "magFilter", 9728);  // GL_NEAREST
		switch (minFilter)
		{
		case 9728: // GL_NEAREST
			aSampler.myMinFilter = ::Texture::Filter::Nearest;
			break;
		case 9729: // GL_LINEAR
			aSampler.myMinFilter = ::Texture::Filter::Linear;
			break;
		case 9984: // GL_NEAREST_MIPMAP_NEAREST
			aSampler.myMinFilter = ::Texture::Filter::Nearest_MipMapNearest;
			break;
		case 9985: // GL_LINEAR_MIPMAP_NEAREST
			aSampler.myMinFilter = ::Texture::Filter::Linear_MipMapNearest;
			break;
		case 9986: // GL_NEAREST_MIPMAP_LINEAR
			aSampler.myMinFilter = ::Texture::Filter::Nearest_MipMapLinear;
			break;
		case 9987: // GL_LINEAR_MIPMAP_LINEAR
			aSampler.myMinFilter = ::Texture::Filter::Linear_MipMapLinear;
			break;
		default:
			ASSERT_STR(false, "Unrecognized Min filter!");
		}

		int wrapS = ReadOptional(aSamplerJson, "wrapS", 10497); // GL_REPEAT
		switch (wrapS)
		{
		case 10497: // GL_REPEAT
			aSampler.myWrapU = ::Texture::WrapMode::Repeat;
			break;
		case 33071: // GL_CLAMP_TO_EDGE
			aSampler.myWrapU = ::Texture::WrapMode::Clamp;
			break;
		case 33648: // GL_MIRRORED_REPEAT
			aSampler.myWrapU = ::Texture::WrapMode::MirroredRepeat;
			break;
		default:
			ASSERT_STR(false, "Unrecognized WrapS!");
		}

		int wrapT = ReadOptional(aSamplerJson, "wrapT", 10497); // GL_REPEAT
		switch (wrapT)
		{
		case 10497: // GL_REPEAT
			aSampler.myWrapV = ::Texture::WrapMode::Repeat;
			break;
		case 33071: // GL_CLAMP_TO_EDGE
			aSampler.myWrapV = ::Texture::WrapMode::Clamp;
			break;
		case 33648: // GL_MIRRORED_REPEAT
			aSampler.myWrapV = ::Texture::WrapMode::MirroredRepeat;
			break;
		default:
			ASSERT_STR(false, "Unrecognized WrapT!");
		}
		aSampler.myName = ReadOptional(aSamplerJson, "name", std::string());
	}

	void Texture::ParseItem(const nlohmann::json& aTextureJson, Texture& aTexture)
	{
		aTexture.mySource = ReadOptional(aTextureJson, "source", kInvalidInd);
		aTexture.mySampler = ReadOptional(aTextureJson, "source", kInvalidInd);
		aTexture.myName = ReadOptional(aTextureJson, "name", std::string());
	}

	void Texture::ConstructTextures(const TextureInputs& aInputs,
		std::vector<Handle<::Texture>>& aTextures,
		const std::string& aRelPath)
	{
		for (const Texture& gltfTexture : aInputs.myTextures)
		{
			ASSERT_STR(gltfTexture.mySource != kInvalidInd,
				"Importing data through extensions not supported!");

			Handle<::Texture> texture;

			const Image& image = aInputs.myImages[gltfTexture.mySource];
			if (image.myBufferView != kInvalidInd) // spec recommends to use this first
			{
				ASSERT_STR(image.myMimeType != Image::MimeType::None,
					"According to gltf2 spec, mime type must be defined when buferView is used!");
				const BufferView& view = aInputs.myBufferViews[image.myBufferView];
				ASSERT_STR(view.myByteStride == 0, "Bad assumption, expected that pixel data is packed!");
				const char* bufferStart = aInputs.myBuffers[view.myBuffer].data() + view.myByteOffset;
				const unsigned char* dataPtr = reinterpret_cast<const unsigned char*>(bufferStart);
				texture = ::Texture::LoadFromMemory(dataPtr, view.myBufferLength);
			}
			else
			{
				ASSERT_STR(!image.myUri.empty(), "Either BufferView or uri must be defined!");
				
				if (IsDataURI(image.myUri))
				{
					std::vector<char> dataBuffer = ParseDataURI(image.myUri);
					const unsigned char* dataPtr = reinterpret_cast<const unsigned char*>(dataBuffer.data());
					texture = ::Texture::LoadFromMemory(dataPtr, dataBuffer.size());
				}
				else
				{
					std::string amendedPath = aRelPath + image.myUri;
					texture = ::Texture::LoadFromDisk(amendedPath);
				}

			}
			ASSERT_STR(texture.IsValid(), "Missing texture!");

			aTextures.push_back(std::move(texture));
		}
	}
}