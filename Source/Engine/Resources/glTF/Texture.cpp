#include "Precomp.h"
#include "Texture.h"

namespace glTF
{
	// TODO: refactor out all the boilerplate in Parse
	std::vector<Image> Image::Parse(const nlohmann::json& aRoot)
	{
		std::vector<Image> images;

		const auto& imagesJsonIter = aRoot.find("images");
		if (imagesJsonIter == aRoot.end())
		{
			return images;
		}

		const std::vector<nlohmann::json>& imagesJson = *imagesJsonIter;
		images.reserve(imagesJson.size());
		for (const auto& imageJson : imagesJson)
		{
			Image image;
			image.myUri = ReadOptional(imageJson, "uri", std::string());
			
			std::string mimeType = ReadOptional(imageJson, "mimeType", std::string());
			image.myMimeType = Image::MimeType::None;
			if (mimeType == "image/jpeg")
			{
				image.myMimeType = Image::MimeType::Jpeg;
			}
			else if (mimeType == "image/png")
			{
				image.myMimeType = Image::MimeType::PNG;
			}
			else
			{
				ASSERT_STR(mimeType.empty(), "Unrecognized MIME type!");
			}

			image.myBufferView = ReadOptional(imageJson, "bufferView", kInvalidInd);
			image.myName = ReadOptional(imageJson, "name", std::string());

			images.push_back(std::move(image));
		}
		return images;
	}

	std::vector<Sampler> Sampler::Parse(const nlohmann::json& aRoot)
	{
		std::vector<Sampler> samplers;

		const auto& samplersJsonIter = aRoot.find("samplers");
		if (samplersJsonIter == aRoot.end())
		{
			return samplers;
		}

		const std::vector<nlohmann::json>& samplersJson = *samplersJsonIter;
		samplers.reserve(samplersJson.size());
		for (const auto& samplerJson : samplersJson)
		{
			Sampler sampler;

			int magFilter = ReadOptional(samplerJson, "magFilter", 9728); // GL_NEAREST
			switch (magFilter)
			{
			case 9728: // GL_NEAREST
				sampler.myMagFilter = ::Texture::Filter::Nearest;
				break;
			case 9729: // GL_LINEAR
				sampler.myMagFilter = ::Texture::Filter::Linear;
				break;
			default:
				ASSERT_STR(false, "Unrecognized Mag filter!");
			}
			int minFilter = ReadOptional(samplerJson, "magFilter", 9728);  // GL_NEAREST
			switch (minFilter)
			{
			case 9728: // GL_NEAREST
				sampler.myMinFilter = ::Texture::Filter::Nearest;
				break;
			case 9729: // GL_LINEAR
				sampler.myMinFilter = ::Texture::Filter::Linear;
				break;
			case 9984: // GL_NEAREST_MIPMAP_NEAREST
				sampler.myMinFilter = ::Texture::Filter::Nearest_MipMapNearest;
				break;
			case 9985: // GL_LINEAR_MIPMAP_NEAREST
				sampler.myMinFilter = ::Texture::Filter::Linear_MipMapNearest;
				break;
			case 9986: // GL_NEAREST_MIPMAP_LINEAR
				sampler.myMinFilter = ::Texture::Filter::Nearest_MipMapLinear;
				break;
			case 9987: // GL_LINEAR_MIPMAP_LINEAR
				sampler.myMinFilter = ::Texture::Filter::Linear_MipMapLinear;
				break;
			default:
				ASSERT_STR(false, "Unrecognized Min filter!");
			}

			int wrapS = ReadOptional(samplerJson, "wrapS", 10497); // GL_REPEAT
			switch (wrapS)
			{
			case 10497: // GL_REPEAT
				sampler.myWrapU = ::Texture::WrapMode::Repeat;
				break;
			case 33071: // GL_CLAMP_TO_EDGE
				sampler.myWrapU = ::Texture::WrapMode::Clamp;
				break;
			case 33648: // GL_MIRRORED_REPEAT
				sampler.myWrapU = ::Texture::WrapMode::MirroredRepeat;
				break;
			default:
				ASSERT_STR(false, "Unrecognized WrapS!");
			}

			int wrapT = ReadOptional(samplerJson, "wrapT", 10497); // GL_REPEAT
			switch (wrapT)
			{
			case 10497: // GL_REPEAT
				sampler.myWrapV = ::Texture::WrapMode::Repeat;
				break;
			case 33071: // GL_CLAMP_TO_EDGE
				sampler.myWrapV = ::Texture::WrapMode::Clamp;
				break;
			case 33648: // GL_MIRRORED_REPEAT
				sampler.myWrapV = ::Texture::WrapMode::MirroredRepeat;
				break;
			default:
				ASSERT_STR(false, "Unrecognized WrapT!");
			}
			sampler.myName = ReadOptional(samplerJson, "name", std::string());

			samplers.push_back(std::move(sampler));
		}
		return samplers;
	}

	std::vector<Texture> Texture::Parse(const nlohmann::json& aRoot)
	{
		std::vector<Texture> textures;

		const auto& texturesJsonIter = aRoot.find("textures");
		if (texturesJsonIter == aRoot.end())
		{
			return textures;
		}

		const std::vector<nlohmann::json>& texturesJson = *texturesJsonIter;
		textures.reserve(texturesJson.size());
		for (const auto& textureJson : texturesJson)
		{
			Texture texture;

			texture.mySource = ReadOptional(textureJson, "source", kInvalidInd);
			texture.mySampler = ReadOptional(textureJson, "source", kInvalidInd);
			texture.myName = ReadOptional(textureJson, "name", std::string());

			textures.push_back(std::move(texture));
		}

		return textures;
	}

	void Texture::ConstructTextures(const TextureInputs& aInputs,
		std::vector<Handle<::Texture>>& aTextures)
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
					std::string amendedPath = ::Texture::kDir.CStr() + image.myUri;
					texture = ::Texture::LoadFromDisk(amendedPath);
				}

			}
			ASSERT_STR(texture.IsValid(), "Missing texture!");

			aTextures.push_back(std::move(texture));
		}
	}
}