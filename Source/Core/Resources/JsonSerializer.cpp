#include "Precomp.h"
#include "JsonSerializer.h"

#include "../File.h"

void JsonSerializer::ReadFrom(const std::vector<char>& aBuffer)
{
    myCurrObj = nlohmann::json::parse(aBuffer.cbegin(), aBuffer.cend(), nullptr, false);
    ASSERT_STR(myCurrObj.is_object(), "Error parsing Root entry must be a json object!");
}

void JsonSerializer::WriteTo(std::vector<char>& aBuffer) const
{
    ASSERT_STR(myObjStack.empty(), "Tried to write to buffer before unrolling the entire obj stack!");
    const std::string& generatedJson = myCurrObj.dump();
    aBuffer.resize(generatedJson.size());
	generatedJson.copy(aBuffer.data(), generatedJson.size());
}

void JsonSerializer::SerializeExternal(std::string_view aFile, std::vector<char>& aBlob)
{
    File file(aFile);
    if (IsReading())
    {
        file.Read();
        aBlob = file.ConsumeBuffer();
    }
    else
    {
        file.Write(aBlob.data(), aBlob.size());
    }
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
        ASSERT_STR(myCurrObj.is_object(), "Current object is not a json object!");
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
        ASSERT_STR(myCurrObj.is_array(), "Current object is not a json array!");
        using Type = std::decay_t<decltype(aVarValue)>;
        const nlohmann::json& jsonElem = myCurrObj[anIndex];
        if (!jsonElem.is_null())
        {
            aVarValue = std::move(jsonElem.get<Type>());
        }
    }, aValue);
}

void JsonSerializer::SerializeEnumImpl(std::string_view aName, size_t anEnumValue, const char* const*, size_t)
{
    myCurrObj[std::string(aName)] = anEnumValue;
}

void JsonSerializer::SerializeEnumImpl(size_t anIndex, size_t anEnumValue, const char* const*, size_t)
{
    myCurrObj[anIndex] = anEnumValue;
}

void JsonSerializer::DeserializeEnumImpl(std::string_view aName, size_t& anEnumValue, const char* const*, size_t) const
{
    const nlohmann::json& jsonElem = myCurrObj[std::string(aName)];
    if (!jsonElem.is_null())
    {
        anEnumValue = jsonElem.get<size_t>();
    }
}

void JsonSerializer::DeserializeEnumImpl(size_t anIndex, size_t& anEnumValue, const char* const*, size_t) const
{
    const nlohmann::json& jsonElem = myCurrObj[anIndex];
    if (!jsonElem.is_null())
    {
        anEnumValue = jsonElem.get<size_t>();
    }
}

void JsonSerializer::BeginSerializeObjectImpl(std::string_view /*aName*/)
{
    ASSERT_STR(myCurrObj.is_object(), "Current object is not a json object!");
    myObjStack.push(std::move(myCurrObj));
    myCurrObj = nlohmann::json::object();
}

void JsonSerializer::BeginSerializeObjectImpl(size_t /*anIndex*/)
{
    ASSERT_STR(myCurrObj.is_array(), "Current object is not a json array!");
    myObjStack.push(std::move(myCurrObj));
    myCurrObj = nlohmann::json::object();
}

void JsonSerializer::EndSerializeObjectImpl(std::string_view aName)
{
    nlohmann::json parent = std::move(myObjStack.top());
    myObjStack.pop();
    ASSERT_STR(parent.is_object(), "Current object is not a json object!");
    parent[std::string(aName)] = std::move(myCurrObj);
    myCurrObj = std::move(parent);
}

void JsonSerializer::EndSerializeObjectImpl(size_t anIndex)
{
    nlohmann::json parent = std::move(myObjStack.top());
    myObjStack.pop();
    ASSERT_STR(parent.is_array(), "Current object is not a json array!");
    parent[anIndex] = std::move(myCurrObj);
    myCurrObj = std::move(parent);
}

bool JsonSerializer::BeginDeserializeObjectImpl(std::string_view aName) const
{
    ASSERT_STR(myCurrObj.is_object(), "Current object is not a json object!");
    auto iter = myCurrObj.find(std::string(aName));
    if (iter == myCurrObj.end()) // TODO: [[unlikely]]
    {
        return false;
    }
    nlohmann::json childObj = std::move(iter.value());
    ASSERT_STR(childObj.is_object(), "Current object is not a json object!");
    myObjStack.push(std::move(myCurrObj));
    myCurrObj = std::move(childObj);
    return true;
}

bool JsonSerializer::BeginDeserializeObjectImpl(size_t anIndex) const
{
    ASSERT_STR(myCurrObj.is_array(), "Current object is not a json array!");
    if (anIndex >= myCurrObj.size()) // TODO: [[unlikely]]
    {
        return false;
    }
    nlohmann::json childObj = myCurrObj.at(anIndex);
    ASSERT_STR(childObj.is_object(), "Current object is not a json object!");
    myObjStack.push(std::move(myCurrObj));
    myCurrObj = std::move(childObj);
    return true;
}

void JsonSerializer::EndDeserializeObjectImpl(std::string_view /*aName*/) const
{
    myCurrObj = std::move(myObjStack.top());
    myObjStack.pop();
    ASSERT_STR(myCurrObj.is_object(), "Current object is not a json object!");
}

void JsonSerializer::EndDeserializeObjectImpl(size_t /*anIndex*/) const
{
    myCurrObj = std::move(myObjStack.top());
    myObjStack.pop();
    ASSERT_STR(myCurrObj.is_array(), "Current object is not a json array!");
}

void JsonSerializer::BeginSerializeArrayImpl(std::string_view aName, size_t /*aCount*/)
{
    ASSERT_STR(myCurrObj.is_object(), "Current object is not a json object!");
    myObjStack.push(std::move(myCurrObj));
    myCurrObj = nlohmann::json::array();
}

void JsonSerializer::BeginSerializeArrayImpl(size_t anIndex, size_t /*aCount*/)
{
    ASSERT_STR(myCurrObj.is_array(), "Current object is not a json array!");
    myObjStack.push(std::move(myCurrObj));
    myCurrObj = nlohmann::json::array();
}

void JsonSerializer::EndSerializeArrayImpl(std::string_view aName)
{
    nlohmann::json parent = std::move(myObjStack.top());
    myObjStack.pop();
    ASSERT_STR(parent.is_object(), "Current object is not a json object!");
    parent[std::string(aName)] = std::move(myCurrObj);
    myCurrObj = std::move(parent);
}

void JsonSerializer::EndSerializeArrayImpl(size_t anIndex)
{
    nlohmann::json parent = std::move(myObjStack.top());
    myObjStack.pop();
    ASSERT_STR(parent.is_array(), "Current object is not a json array!");
    parent[anIndex] = std::move(myCurrObj);
    myCurrObj = std::move(parent);
}

bool JsonSerializer::BeginDeserializeArrayImpl(std::string_view aName, size_t& aCount) const
{
    ASSERT_STR(myCurrObj.is_object(), "Current object is not a json object!");
    auto iter = myCurrObj.find(std::string(aName));
    if (iter == myCurrObj.end()) // TODO: [[unlikely]]
    {
        return false;
    }
    nlohmann::json childObj = std::move(iter.value());
    ASSERT_STR(childObj.is_array(), "Current object is not a json array!");
    aCount = childObj.size();
    myObjStack.push(std::move(myCurrObj));
    myCurrObj = std::move(childObj);
    return true;
}

bool JsonSerializer::BeginDeserializeArrayImpl(size_t anIndex, size_t& aCount) const
{
    ASSERT_STR(myCurrObj.is_array(), "Current object is not a json array!");
    if (anIndex >= myCurrObj.size()) // TODO: [[unlikely]]
    {
        return false;
    }
    nlohmann::json childObj = myCurrObj.at(anIndex);
    ASSERT_STR(childObj.is_array(), "Current object is not a json array!");
    aCount = childObj.size();
    myObjStack.push(std::move(myCurrObj));
    myCurrObj = std::move(childObj);
    return true;
}

void JsonSerializer::EndDeserializeArrayImpl(std::string_view /*aName*/) const
{
    myCurrObj = std::move(myObjStack.top());
    myObjStack.pop();
    ASSERT_STR(myCurrObj.is_object(), "Current object is not a json object!");
}

void JsonSerializer::EndDeserializeArrayImpl(size_t /*anIndex*/) const
{
    myCurrObj = std::move(myObjStack.top());
    myObjStack.pop();
    ASSERT_STR(myCurrObj.is_array(), "Current object is not a json array!");
}

namespace nlohmann
{
    template<>
    struct adl_serializer<JsonSerializer::ResourceProxy>
    {
        static void to_json(json& j, const JsonSerializer::ResourceProxy& res)
        {
            if (!res.myPath.empty())
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
    struct adl_serializer<glm::vec2>
    {
        static void to_json(json& j, const glm::vec2& res)
        {
            j = json::array({ res.x, res.y });
        }

        static void from_json(const json& j, glm::vec2& res)
        {
            if (j.is_array())
            {
                constexpr int matLength = glm::vec2::length();
                for (uint8_t index = 0; index < matLength; index++)
                {
                    glm::value_ptr(res)[index] = j[index].get<float>();
                }
            }
            else
            {
                ASSERT(false);
                res = glm::vec2(0);
            }
        }
    };

    template<>
    struct adl_serializer<glm::vec3>
    {
        static void to_json(json& j, const glm::vec3& res)
        {
            j = json::array({ res.x, res.y, res.z });
        }

        static void from_json(const json& j, glm::vec3& res)
        {
            if (j.is_array())
            {
                constexpr int matLength = glm::vec3::length();
                for (uint8_t index = 0; index < matLength; index++)
                {
                    glm::value_ptr(res)[index] = j[index].get<float>();
                }
            }
            else
            {
                ASSERT(false);
                res = glm::vec3(0);
            }
        }
    };

    template<>
    struct adl_serializer<glm::vec4>
    {
        static void to_json(json& j, const glm::vec4& res)
        {
            j = json::array({ res.x, res.y, res.z, res.w });
        }

        static void from_json(const json& j, glm::vec4& res)
        {
            if (j.is_array())
            {
                constexpr int matLength = glm::vec4::length();
                for (uint8_t index = 0; index < matLength; index++)
                {
                    glm::value_ptr(res)[index] = j[index].get<float>();
                }
            }
            else
            {
                ASSERT(false);
                res = glm::vec4(0);
            }
        }
    };

    template<>
    struct adl_serializer<glm::quat>
    {
        static void to_json(json& j, const glm::quat& res)
        {
            j = json::array({ res.x, res.y, res.z, res.w });
        }

        static void from_json(const json& j, glm::quat& res)
        {
            if (j.is_array())
            {
                constexpr int matLength = glm::quat::length();
                for (uint8_t index = 0; index < matLength; index++)
                {
                    glm::value_ptr(res)[index] = j[index].get<float>();
                }
            }
            else
            {
                ASSERT(false);
                res = glm::quat(0, 0, 0, 1);
            }
        }
    };

    template<>
    struct adl_serializer<glm::mat4>
    {
        static void to_json(json& j, const glm::mat4& res)
        {
            const float* data = glm::value_ptr(res);
            j = json::array({ 
                data[0], data[1], data[2], data[3],
                data[4], data[5], data[6], data[7],
                data[8], data[9], data[10], data[11],
                data[12], data[13], data[14], data[15]
            });
        }

        static void from_json(const json& j, glm::mat4& res)
        {
            if (j.is_array())
            {
                constexpr int matLength = glm::mat4::length() * glm::mat4::length();
                for (uint8_t index = 0; index < matLength; index++)
                {
                    glm::value_ptr(res)[index] = j[index].get<float>();
                }
            }
            else
            {
                ASSERT(false);
                res = glm::mat4(1);
            }
        }
    };
}