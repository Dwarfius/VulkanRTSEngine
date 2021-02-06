#pragma once

#include "Common.h"

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

		static std::vector<Buffer> Parse(const nlohmann::json& aRootJson, const std::string& aRelPath)
		{
			std::vector<Buffer> buffers;
			const nlohmann::json& buffersJson = aRootJson["buffers"];
			buffers.reserve(buffersJson.size());
			for (const nlohmann::json& bufferJson : buffersJson)
			{
				Buffer buffer;
				std::string uri = bufferJson["uri"].get<std::string>();
				if (IsDataURI(uri))
				{
					buffer = ParseDataURI(uri);
				}
				else
				{
					// it's a file, and we assume it's located nearby
					File file(aRelPath + uri);
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