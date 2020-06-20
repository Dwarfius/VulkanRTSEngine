#include "Precomp.h"
#include "JsonSerializer.h"

#include "../File.h"

void JsonSerializer::ReadFrom(const File& aFile)
{
	myJsonObj = nlohmann::json::parse(aFile.GetCBuffer(), nullptr, false);
}

void JsonSerializer::WriteTo(std::string& aBuffer) const
{
	aBuffer = myJsonObj.dump();
}

void JsonSerializer::SerializeImpl(const std::string_view& aName, const VariantType& aValue)
{
	std::visit([&](auto&& aVarValue) {
		myJsonObj[std::string(aName)] = aVarValue;
	}, aValue);
}

void JsonSerializer::SerializeImpl(const std::string_view& aName, const VecVariantType& aValue, const VariantType& aHint)
{
    std::visit([&](auto&& aHintValue) {
        using Type = std::decay_t<decltype(aHintValue)>;
        nlohmann::json valueArray;
        const std::vector<Type>& valueSet = std::get<std::vector<Type>>(aValue);
        for (const Type& value : valueSet)
        {
            valueArray.push_back(value);
        }
        myJsonObj[std::string(aName)] = valueArray;
    }, aHint);
}

void JsonSerializer::DeserializeImpl(const std::string_view& aName, VariantType& aValue)
{
    std::visit([&](auto&& aVarValue) {
        using Type = std::decay_t<decltype(aVarValue)>;
        const nlohmann::json& jsonElem = myJsonObj[std::string(aName)];
        if (!jsonElem.is_null())
        {
            aVarValue = std::move(jsonElem.get<Type>());
        }
    }, aValue);
}

void JsonSerializer::DeserializeImpl(const std::string_view& aName, VecVariantType& aValue, const VariantType& aHint)
{
    nlohmann::json valueArray = myJsonObj[std::string(aName)];
    if (valueArray.is_array())
    {
        std::visit([&](auto&& aHintValue) {
            using Type = std::decay_t<decltype(aHintValue)>;
            std::vector<Type>& valueSet = std::get<std::vector<Type>>(aValue);
            valueSet.resize(valueArray.size());
            for (size_t i = 0; i < valueSet.size(); i++)
            {
                valueSet[i] = std::move(valueArray[i].get<Type>());
            }
        }, aHint);
    }
}

namespace nlohmann
{
    template<>
    struct adl_serializer<JsonSerializer::ResourceProxy>
    {
        static void to_json(nlohmann::json& j, const JsonSerializer::ResourceProxy& res)
        {
            if (res.myPath.size() > 0)
            {
                j = json{ {"Res", res.myPath} };
            }
            else
            {
                j = nullptr;
            }
        }

        static void from_json(const nlohmann::json& j, JsonSerializer::ResourceProxy& res)
        {
            if (j.is_object())
            {
                res.myPath = j["Res"].get<std::string>();
            }
        }
    };
}