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

void JsonSerializer::SerializeImpl(std::string_view aName, const VariantType& aValue)
{
	std::visit([&](auto&& aVarValue) {
		myJsonObj[std::string(aName)] = aVarValue;
	}, aValue);
}

void JsonSerializer::SerializeImpl(std::string_view aName, const VecVariantType& aValue, const VariantType& aHint)
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

void JsonSerializer::DeserializeImpl(std::string_view aName, VariantType& aValue)
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

void JsonSerializer::DeserializeImpl(std::string_view aName, VecVariantType& aValue, const VariantType& aHint)
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
        static void to_json(json& j, const JsonSerializer::ResourceProxy& res)
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

        static void from_json(const json& j, JsonSerializer::ResourceProxy& res)
        {
            if (j.is_object())
            {
                res.myPath = j["Res"].get<std::string>();
            }
        }
    };

    template<>
    struct adl_serializer<VariantMap>
    {
        static void to_json(json& j, const VariantMap& res)
        {
            for (const auto& [key, variant] : res)
            {
                const std::string& jsonName = key;
                std::visit([&](const auto& aValue) {
                    j[jsonName] = aValue;
                }, variant);
            }
        }

        static void from_json(const json& j, VariantMap& res)
        {
            res = VariantMap();
            if (!j.is_object() || j.is_null())
            {
                return;
            }

            json::object_t jsonObj = j.get<json::object_t>();
            for (const auto& [key, value] : jsonObj)
            {
                switch (value.type())
                {
                case json::value_t::boolean:
                    res.Set(key, value.get<json::boolean_t>());
                    break;
                case json::value_t::number_unsigned:
                    res.Set(key, value.get<json::number_unsigned_t>());
                    break;
                case json::value_t::number_integer:
                    res.Set(key, value.get<json::number_integer_t>());
                    break;
                case json::value_t::number_float:
                    res.Set(key, value.get<float>());
                    break;
                case json::value_t::string:
                    res.Set(key, value.get<json::string_t>());
                    break;
                case json::value_t::array:
                {
                    if (value.size())
                    {
                        json::array_t childArray = value.get<json::array_t>();
                        switch (childArray.front().type())
                        {
                        case json::value_t::boolean:
                            res.Set(key, value.get<std::vector<json::boolean_t>>());
                            break;
                        case json::value_t::number_unsigned:
                            res.Set(key, value.get<std::vector<json::number_unsigned_t>>());
                            break;
                        case json::value_t::number_integer:
                            res.Set(key, value.get<std::vector<json::number_integer_t>>());
                            break;
                        case json::value_t::number_float:
                            res.Set(key, value.get<std::vector<float>>());
                            break;
                        case json::value_t::string:
                            res.Set(key, value.get< std::vector<json::string_t>>());
                            break;
                        case json::value_t::array:
                        {
                            ASSERT_STR(false, "Array of arrays not supported!");
                            break;
                        }
                        case json::value_t::object:
                        {
                            res.Set(key, value.get<std::vector<VariantMap>>());
                            break;
                        }
                        default:
                            ASSERT(false);
                        }
                    }
                    break;
                }
                case json::value_t::object:
                {
                    res.Set(key, value.get<VariantMap>());
                    break;
                }
                default:
                    ASSERT(false);
                }
            }
        }
    };
}