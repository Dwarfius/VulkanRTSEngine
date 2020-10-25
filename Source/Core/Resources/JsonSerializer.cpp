#include "Precomp.h"
#include "JsonSerializer.h"

#include "../File.h"

void JsonSerializer::ReadFrom(const File& aFile)
{
    myCurrObj = nlohmann::json::parse(aFile.GetCBuffer(), nullptr, false);
}

void JsonSerializer::WriteTo(std::string& aBuffer) const
{
    ASSERT_STR(myObjStack.empty(), "Tried to write to buffer before unrolling the entire obj stack!");
	aBuffer = myCurrObj.dump();
}

void JsonSerializer::SerializeImpl(std::string_view aName, const VariantType& aValue)
{
	std::visit([&](auto&& aVarValue) {
		myCurrObj[std::string(aName)] = aVarValue;
	}, aValue);
}

void JsonSerializer::SerializeImpl(size_t anIndex, const VariantType& aValue)
{
    std::visit([&](auto&& aVarValue) {
        myCurrObj[anIndex] = aVarValue;
    }, aValue);
}

void JsonSerializer::DeserializeImpl(std::string_view aName, VariantType& aValue) const
{
    std::visit([&](auto&& aVarValue) {
        ASSERT_STR(myCurrObj.is_object(), "Current object is not an object!");
        using Type = std::decay_t<decltype(aVarValue)>;
        const nlohmann::json& jsonElem = myCurrObj[std::string(aName)];
        if (!jsonElem.is_null())
        {
            aVarValue = std::move(jsonElem.get<Type>());
        }
    }, aValue);
}

void JsonSerializer::DeserializeImpl(size_t anIndex, VariantType& aValue) const
{
    std::visit([&](auto&& aVarValue) {
        ASSERT_STR(myCurrObj.is_array(), "Current object is not an array!");
        using Type = std::decay_t<decltype(aVarValue)>;
        const nlohmann::json& jsonElem = myCurrObj[anIndex];
        if (!jsonElem.is_null())
        {
            aVarValue = std::move(jsonElem.get<Type>());
        }
    }, aValue);
}

void JsonSerializer::BeginSerializeObjectImpl(std::string_view /*aName*/)
{
    ASSERT_STR(myCurrObj.is_object(), "Current object is not an object!");
    myObjStack.push(std::move(myCurrObj));
    myCurrObj = nlohmann::json::object();
}

void JsonSerializer::BeginSerializeObjectImpl(size_t /*anIndex*/)
{
    ASSERT_STR(myCurrObj.is_array(), "Current object is not an array!");
    myObjStack.push(std::move(myCurrObj));
    myCurrObj = nlohmann::json::object();
}

void JsonSerializer::EndSerializeObjectImpl(std::string_view aName)
{
    nlohmann::json parent = std::move(myObjStack.top());
    myObjStack.pop();
    ASSERT_STR(parent.is_object(), "Current object is not an object!");
    parent[std::string(aName)] = std::move(myCurrObj);
    myCurrObj = std::move(parent);
}

void JsonSerializer::EndSerializeObjectImpl(size_t anIndex)
{
    nlohmann::json parent = std::move(myObjStack.top());
    myObjStack.pop();
    ASSERT_STR(parent.is_array(), "Current object is not an array!");
    parent[anIndex] = std::move(myCurrObj);
    myCurrObj = std::move(parent);
}

bool JsonSerializer::BeginDeserializeObjectImpl(std::string_view aName) const
{
    ASSERT_STR(myCurrObj.is_object(), "Current object is not an object!");
    auto iter = myCurrObj.find(std::string(aName));
    if (iter == myCurrObj.end()) // TODO: [[unlikely]]
    {
        return false;
    }
    nlohmann::json childObj = std::move(iter.value());
    ASSERT_STR(childObj.is_object(), "Current object is not an object!");
    myObjStack.push(std::move(myCurrObj));
    myCurrObj = std::move(childObj);
    return true;
}

bool JsonSerializer::BeginDeserializeObjectImpl(size_t anIndex) const
{
    ASSERT_STR(myCurrObj.is_array(), "Current object is not an array!");
    if (anIndex >= myCurrObj.size()) // TODO: [[unlikely]]
    {
        return false;
    }
    nlohmann::json childObj = myCurrObj.at(anIndex);
    ASSERT_STR(childObj.is_object(), "Current object is not an object!");
    myObjStack.push(std::move(myCurrObj));
    myCurrObj = std::move(childObj);
    return true;
}

void JsonSerializer::EndDeserializeObjectImpl(std::string_view /*aName*/) const
{
    myCurrObj = std::move(myObjStack.top());
    myObjStack.pop();
    ASSERT_STR(myCurrObj.is_object(), "Current object is not an object!");
}

void JsonSerializer::EndDeserializeObjectImpl(size_t /*anIndex*/) const
{
    myCurrObj = std::move(myObjStack.top());
    myObjStack.pop();
    ASSERT_STR(myCurrObj.is_array(), "Current object is not an array!");
}

void JsonSerializer::BeginSerializeArrayImpl(std::string_view aName, size_t /*aCount*/)
{
    ASSERT_STR(myCurrObj.is_object(), "Current object is not an object!");
    myObjStack.push(std::move(myCurrObj));
    myCurrObj = nlohmann::json::array();
}

void JsonSerializer::BeginSerializeArrayImpl(size_t anIndex, size_t /*aCount*/)
{
    ASSERT_STR(myCurrObj.is_array(), "Current object is not an array!");
    myObjStack.push(std::move(myCurrObj));
    myCurrObj = nlohmann::json::array();
}

void JsonSerializer::EndSerializeArrayImpl(std::string_view aName)
{
    nlohmann::json parent = std::move(myObjStack.top());
    myObjStack.pop();
    ASSERT_STR(parent.is_object(), "Current object is not an object!");
    parent[std::string(aName)] = std::move(myCurrObj);
    myCurrObj = std::move(parent);
}

void JsonSerializer::EndSerializeArrayImpl(size_t anIndex)
{
    nlohmann::json parent = std::move(myObjStack.top());
    myObjStack.pop();
    ASSERT_STR(parent.is_array(), "Current object is not an array!");
    parent[anIndex] = std::move(myCurrObj);
    myCurrObj = std::move(parent);
}

bool JsonSerializer::BeginDeserializeArrayImpl(std::string_view aName, size_t& aCount) const
{
    ASSERT_STR(myCurrObj.is_object(), "Current object is not an object!");
    auto iter = myCurrObj.find(std::string(aName));
    if (iter == myCurrObj.end()) // TODO: [[unlikely]]
    {
        return false;
    }
    nlohmann::json childObj = std::move(iter.value());
    ASSERT_STR(childObj.is_array(), "Current object is not an array!");
    aCount = childObj.size();
    myObjStack.push(std::move(myCurrObj));
    myCurrObj = std::move(childObj);
    return true;
}

bool JsonSerializer::BeginDeserializeArrayImpl(size_t anIndex, size_t& aCount) const
{
    ASSERT_STR(myCurrObj.is_array(), "Current object is not an array!");
    if (anIndex >= myCurrObj.size()) // TODO: [[unlikely]]
    {
        return false;
    }
    nlohmann::json childObj = myCurrObj.at(anIndex);
    ASSERT_STR(childObj.is_array(), "Current object is not an array!");
    aCount = childObj.size();
    myObjStack.push(std::move(myCurrObj));
    myCurrObj = std::move(childObj);
    return true;
}

void JsonSerializer::EndDeserializeArrayImpl(std::string_view /*aName*/) const
{
    myCurrObj = std::move(myObjStack.top());
    myObjStack.pop();
    ASSERT_STR(myCurrObj.is_object(), "Current object is not an object!");
}

void JsonSerializer::EndDeserializeArrayImpl(size_t /*anIndex*/) const
{
    myCurrObj = std::move(myObjStack.top());
    myObjStack.pop();
    ASSERT_STR(myCurrObj.is_array(), "Current object is not an array!");
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