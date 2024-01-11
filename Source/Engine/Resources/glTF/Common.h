#pragma once

#include <nlohmann/json.hpp>
#include <Core/Utils.h>

namespace glTF
{
	constexpr static int kInvalidInd = -1;

	template<class T>
	T ReadOptional(const nlohmann::json& aJson, std::string_view aKey, T aDefaultVal)
	{
		const auto& iter = aJson.find(aKey);
		if (iter != aJson.end())
		{
			return iter->get<T>();
		}
		return aDefaultVal;
	}

	template<class T, class... TParseItemArgs>
	std::vector<T> Parse(const nlohmann::json& aRoot, std::string_view aName, TParseItemArgs&&... aArgs)
	{
		std::vector<T> items;

		const auto& itemsJsonIter = aRoot.find(aName);
		if (itemsJsonIter == aRoot.end())
		{
			return items;
		}

		const std::vector<nlohmann::json>& itemsJson = *itemsJsonIter;
		items.resize(itemsJson.size());
		for (size_t i=0; i<items.size(); i++)
		{
			T::ParseItem(itemsJson[i], items[i], std::forward<TParseItemArgs>(aArgs)...);
		}
		return items;
	}

	inline bool IsDataURI(const std::string& aUri)
	{
		if (aUri.size() <= 5)
		{
			return false;
		}

		std::string_view uriPrefix(aUri.c_str(), 5);
		return uriPrefix == "data:";
	}

	inline std::vector<char> ParseDataURI(const std::string& aUri)
	{
		size_t separatorInd = aUri.find(';', 5);
		std::string_view mimeType(aUri.c_str() + 5, separatorInd - 5);

		// https://github.com/KhronosGroup/glTF-Tutorials/issues/21#issuecomment-443319562
		if (mimeType != "application/octet-stream"
			&& mimeType != "application/gltf-buffer"
			&& mimeType != "image/png"
			&& mimeType != "image/jpeg")
		{
			std::string mimeTypeStr(aUri.c_str() + 5, separatorInd - 5);
			ASSERT_STR(false, "Data uri {} not supported!", mimeTypeStr);
			return std::vector<char>();
		}

		std::string_view encoding(aUri.c_str() + separatorInd + 1, 6);
		if (encoding != "base64")
		{
			ASSERT_STR(false, "Only base64 encoding of data uri is supported!");
			return std::vector<char>();
		}

		std::string_view data(aUri.c_str() + separatorInd + 8);
		return Utils::Base64Decode(data);
	}
}