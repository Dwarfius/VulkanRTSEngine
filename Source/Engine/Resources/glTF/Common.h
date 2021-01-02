#pragma once

#include <nlohmann/json.hpp>

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
}