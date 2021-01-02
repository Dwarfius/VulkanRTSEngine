#pragma once

#include "Common.h"

#include <Core/Utils.h>
#include <Core/File.h>

namespace glTF
{
	struct Buffer : std::vector<char>
	{
		Buffer& operator=(std::vector<char>&& aVec)
		{
			*static_cast<std::vector<char>*>(this) = std::move(aVec);
			return *this;
		}

		static std::vector<Buffer> Parse(const nlohmann::json& aRootJson)
		{
			std::vector<Buffer> buffers;
			const nlohmann::json& buffersJson = aRootJson["buffers"];
			buffers.reserve(buffersJson.size());
			for (const nlohmann::json& bufferJson : buffersJson)
			{
				Buffer buffer;
				std::string uri = bufferJson["uri"].get<std::string>();
				std::string_view uriPrefix(uri.c_str(), 5);
				if (uriPrefix == "data:")
				{
					size_t separatorInd = uri.find(';', 5);
					std::string_view mimeType(uri.c_str() + 5, separatorInd - 5);

					// https://github.com/KhronosGroup/glTF-Tutorials/issues/21#issuecomment-443319562
					if (mimeType != "application/octet-stream"
						&& mimeType != "application/gltf-buffer")
					{
						ASSERT_STR(false, "Only Octet-stream data uri is supported!");
						continue;
					}

					std::string_view encoding(uri.c_str() + separatorInd + 1, 6);
					if (encoding != "base64")
					{
						ASSERT_STR(false, "Only base64 encoding of data uri is supported!");
						continue;
					}

					std::string_view data(uri.c_str() + separatorInd + 8);
					buffer = std::move(Utils::Base64Decode(data));
				}
				else
				{
					// it's a file, and we assume it's located nearby
					File file(uri);
					if (!file.Read())
					{
						ASSERT_STR(false, "Failed to read %s", file.GetPath());
						continue;
					}
					// TODO: this is a copy, after File's TODO is done,
					// change to use buffer stealing
					const std::string& fileBuff = file.GetBuffer();
					buffer.assign(fileBuff.begin(), fileBuff.end());
				}

				size_t byteLength = bufferJson["byteLength"].get<size_t>();
				ASSERT(buffer.size() == byteLength);
				buffers.emplace_back(std::move(buffer));
			}
			return buffers;
		}
	};
}