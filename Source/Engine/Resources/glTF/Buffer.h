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

		static void ParseItem(const nlohmann::json& aBufferJson, Buffer& aBuffer, const std::string& aRelPath)
		{
			std::string uri = aBufferJson["uri"].get<std::string>();
			if (IsDataURI(uri))
			{
				aBuffer = std::move(ParseDataURI(uri));
			}
			else
			{
				// it's a file, and we assume it's located nearby
				File file(aRelPath + uri);
				if (!file.Read())
				{
					ASSERT_STR(false, "Failed to read %s", file.GetPath());
					return;
				}
				// TODO: this is a copy, after File's TODO is done,
				// change to use buffer stealing
				const std::string& fileBuff = file.GetBuffer();
				aBuffer.assign(fileBuff.begin(), fileBuff.end());
			}

			size_t byteLength = aBufferJson["byteLength"].get<size_t>();
			ASSERT(aBuffer.size() == byteLength);
		}
	};
}