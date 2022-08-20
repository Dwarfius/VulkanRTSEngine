#include "Precomp.h"
#include "JsonSerializer.h"

#include "../File.h"

namespace Impl
{
    template<class T>
    void SerializeBasic(std::string_view aName, T& aValue, nlohmann::json& aJson, bool aIsReading)
    {
        if (aIsReading)
        {
            aValue = aJson[aName].get<T>();
        }
        else
        {
            aJson[aName] = aValue;
        }
    }

    template<class T>
    void SerializeBasic(size_t anIndex, T& aValue, nlohmann::json& aJson, bool aIsReading)
    {
        if (aIsReading)
        {
            aValue = aJson[anIndex].get<T>();
        }
        else
        {
            aJson.push_back(aValue);
        }
    }

    template<class T>
    void SerializeSpan(T* aValues, size_t aSize, nlohmann::json& aJson, bool aIsReading)
    {
        if (aIsReading)
        {
            ASSERT_STR(aJson.is_array(), "Unexpected type!");
            ASSERT_STR(aJson.size() == aSize, "Missmatching size!");

            const nlohmann::json::array_t& vector = 
                *aJson.get_ptr<const nlohmann::json::array_t*>();
            for (size_t i=0; i<aSize; i++)
            {
                aValues[i] = vector[i].get<T>();
            }
        }
        else
        {
            for (size_t i = 0; i < aSize; i++)
            {
                aJson.emplace_back(aValues[i]);
            }
        }
    }
}

void JsonSerializer::ReadFrom(const std::vector<char>& aBuffer)
{
    myCurrObj = nlohmann::json::parse(aBuffer.cbegin(), aBuffer.cend(), nullptr, false);
    ASSERT_STR(myCurrObj.is_object(), "Error parsing Root entry must be a json object!");
}

void JsonSerializer::WriteTo(std::vector<char>& aBuffer) const
{
    ASSERT_STR(myStateStack.empty(), "Tried to write to buffer before unrolling the entire obj stack!");
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

void JsonSerializer::Serialize(std::string_view aName, bool& aValue)
{
    if (aName.empty())
    {
        State& state = myStateStack.top();
        Impl::SerializeBasic(state.myCurrIndex++, aValue, myCurrObj, IsReading());

    }
    else
    {
        Impl::SerializeBasic(aName, aValue, myCurrObj, IsReading());
    }
}

void JsonSerializer::Serialize(std::string_view aName, uint8_t& aValue)
{
    if (aName.empty())
    {
        State& state = myStateStack.top();
        Impl::SerializeBasic(state.myCurrIndex++, aValue, myCurrObj, IsReading());

    }
    else
    {
        Impl::SerializeBasic(aName, aValue, myCurrObj, IsReading());
    }
}

void JsonSerializer::Serialize(std::string_view aName, uint16_t& aValue)
{
    if (aName.empty())
    {
        State& state = myStateStack.top();
        Impl::SerializeBasic(state.myCurrIndex++, aValue, myCurrObj, IsReading());

    }
    else
    {
        Impl::SerializeBasic(aName, aValue, myCurrObj, IsReading());
    }
}

void JsonSerializer::Serialize(std::string_view aName, uint32_t& aValue)
{
    if (aName.empty())
    {
        State& state = myStateStack.top();
        Impl::SerializeBasic(state.myCurrIndex++, aValue, myCurrObj, IsReading());

    }
    else
    {
        Impl::SerializeBasic(aName, aValue, myCurrObj, IsReading());
    }
}

void JsonSerializer::Serialize(std::string_view aName, uint64_t& aValue)
{
    if (aName.empty())
    {
        State& state = myStateStack.top();
        Impl::SerializeBasic(state.myCurrIndex++, aValue, myCurrObj, IsReading());

    }
    else
    {
        Impl::SerializeBasic(aName, aValue, myCurrObj, IsReading());
    }
}

void JsonSerializer::Serialize(std::string_view aName, int8_t& aValue)
{
    if (aName.empty())
    {
        State& state = myStateStack.top();
        Impl::SerializeBasic(state.myCurrIndex++, aValue, myCurrObj, IsReading());

    }
    else
    {
        Impl::SerializeBasic(aName, aValue, myCurrObj, IsReading());
    }
}

void JsonSerializer::Serialize(std::string_view aName, int16_t& aValue)
{
    if (aName.empty())
    {
        State& state = myStateStack.top();
        Impl::SerializeBasic(state.myCurrIndex++, aValue, myCurrObj, IsReading());

    }
    else
    {
        Impl::SerializeBasic(aName, aValue, myCurrObj, IsReading());
    }
}

void JsonSerializer::Serialize(std::string_view aName, int32_t& aValue)
{
    if (aName.empty())
    {
        State& state = myStateStack.top();
        Impl::SerializeBasic(state.myCurrIndex++, aValue, myCurrObj, IsReading());

    }
    else
    {
        Impl::SerializeBasic(aName, aValue, myCurrObj, IsReading());
    }
}

void JsonSerializer::Serialize(std::string_view aName, int64_t& aValue)
{
    if (aName.empty())
    {
        State& state = myStateStack.top();
        Impl::SerializeBasic(state.myCurrIndex++, aValue, myCurrObj, IsReading());

    }
    else
    {
        Impl::SerializeBasic(aName, aValue, myCurrObj, IsReading());
    }
}

void JsonSerializer::Serialize(std::string_view aName, float& aValue)
{
    if (aName.empty())
    {
        State& state = myStateStack.top();
        Impl::SerializeBasic(state.myCurrIndex++, aValue, myCurrObj, IsReading());

    }
    else
    {
        Impl::SerializeBasic(aName, aValue, myCurrObj, IsReading());
    }
}

void JsonSerializer::Serialize(std::string_view aName, std::string& aValue)
{
    if (aName.empty())
    {
        State& state = myStateStack.top();
        Impl::SerializeBasic(state.myCurrIndex++, aValue, myCurrObj, IsReading());

    }
    else
    {
        Impl::SerializeBasic(aName, aValue, myCurrObj, IsReading());
    }
}

void JsonSerializer::Serialize(std::string_view aName, glm::vec2& aValue)
{
    if (aName.empty())
    {
        State& state = myStateStack.top();
        Impl::SerializeBasic(state.myCurrIndex++, aValue, myCurrObj, IsReading());

    }
    else
    {
        Impl::SerializeBasic(aName, aValue, myCurrObj, IsReading());
    }
}

void JsonSerializer::Serialize(std::string_view aName, glm::vec3& aValue)
{
    if (aName.empty())
    {
        State& state = myStateStack.top();
        Impl::SerializeBasic(state.myCurrIndex++, aValue, myCurrObj, IsReading());

    }
    else
    {
        Impl::SerializeBasic(aName, aValue, myCurrObj, IsReading());
    }
}

void JsonSerializer::Serialize(std::string_view aName, glm::vec4& aValue)
{
    if (aName.empty())
    {
        State& state = myStateStack.top();
        Impl::SerializeBasic(state.myCurrIndex++, aValue, myCurrObj, IsReading());

    }
    else
    {
        Impl::SerializeBasic(aName, aValue, myCurrObj, IsReading());
    }
}

void JsonSerializer::Serialize(std::string_view aName, glm::quat& aValue)
{
    if (aName.empty())
    {
        State& state = myStateStack.top();
        Impl::SerializeBasic(state.myCurrIndex++, aValue, myCurrObj, IsReading());

    }
    else
    {
        Impl::SerializeBasic(aName, aValue, myCurrObj, IsReading());
    }
}

void JsonSerializer::Serialize(std::string_view aName, glm::mat4& aValue)
{
    if (aName.empty())
    {
        State& state = myStateStack.top();
        Impl::SerializeBasic(state.myCurrIndex++, aValue, myCurrObj, IsReading());

    }
    else
    {
        Impl::SerializeBasic(aName, aValue, myCurrObj, IsReading());
    }
}

void JsonSerializer::SerializeEnum(std::string_view aName, size_t& anEnumValue, const char* const* aNames, size_t aNamesLength)
{
    // it is possible to have an array of objects, and this is
    // denoted by a missing name
    const bool isPartOfArray = aName.empty();
    if (IsReading())
    {
        nlohmann::json::iterator iter;
        if (isPartOfArray)
        {
            State& state = myStateStack.top();
            iter = myCurrObj.begin() + state.myCurrIndex;
            state.myCurrIndex++;
        }
        else
        {
            iter = myCurrObj.find(std::string(aName));
            
        }
        ASSERT_STR(iter != myCurrObj.end(), "Failed to find the object!");

        anEnumValue = (*iter).get<size_t>();
    }
    else
    {
        if (isPartOfArray)
        {
            myCurrObj.push_back(anEnumValue);
        }
        else
        {
            myCurrObj[std::string(aName)] = anEnumValue;
        }
    }
}

bool JsonSerializer::BeginSerializeObjectImpl(std::string_view aName)
{
    // it is possible to have an array of objects, and this is
    // denoted by a missing name
    const bool isPartOfArray = aName.empty();
    if (IsReading())
    {
        nlohmann::json::iterator iter;
        if (isPartOfArray)
        {
            State& state = myStateStack.top();
            iter = myCurrObj.begin() + state.myCurrIndex;
            state.myCurrIndex++;
        }
        else
        {
            iter = myCurrObj.find(std::string(aName));
        } 
        ASSERT_STR(iter != myCurrObj.end(), "Failed to find the object!");
        
        // TODO: replace all the copies by values with a copy of pointers to
        // objects
        myStateStack.push({ myCurrObj, 0 });
        myCurrObj = iter.value();
    }
    else
    {
        myStateStack.push({ std::move(myCurrObj), 0 });
        myCurrObj = nlohmann::json::object();
    }
    return true;
}

void JsonSerializer::EndSerializeObjectImpl(std::string_view aName)
{
    // it is possible to have an array of objects, and this is
    // denoted by a missing name
    const bool isPartOfArray = aName.empty();
    if (IsReading())
    {
        State state = std::move(myStateStack.top());
        myCurrObj = std::move(state.myCurrObj);
        myStateStack.pop();
    }
    else
    {
        State state = std::move(myStateStack.top());
        nlohmann::json parent = std::move(state.myCurrObj);
        myStateStack.pop();

        if (isPartOfArray)
        {
            parent.emplace_back(std::move(myCurrObj));
        }
        else
        {
            parent[std::string(aName)] = std::move(myCurrObj);
        }
        myCurrObj = std::move(parent);
    }
}

bool JsonSerializer::BeginSerializeArrayImpl(std::string_view aName, size_t& aCount)
{
    // We don't support arrays-of-arrays serialization
    // as there's no way to invoke this serialization right now
    if (IsReading())
    {
        const auto& iter = myCurrObj.find(std::string(aName));
        ASSERT_STR(iter != myCurrObj.end(), "Failed to find an array!");
        
        myStateStack.push({ myCurrObj, 0 });
        myCurrObj = iter.value();
        aCount = myCurrObj.size();
    }
    else
    {
        myStateStack.push({ std::move(myCurrObj), 0 });
        myCurrObj = nlohmann::json::array();
    }
    return true;
}

void JsonSerializer::EndSerializeArrayImpl(std::string_view aName)
{
    // We don't support arrays-of-arrays serialization
    // as there's no way to invoke this serialization right now
    if (IsReading())
    {
        State state = std::move(myStateStack.top());
        myCurrObj = std::move(state.myCurrObj);
        myStateStack.pop();
    }
    else
    {
        State state = std::move(myStateStack.top());
        nlohmann::json parent = std::move(state.myCurrObj);
        myStateStack.pop();

        parent[std::string(aName)] = std::move(myCurrObj);
        myCurrObj = std::move(parent);
    }
}

void JsonSerializer::SerializeSpan(bool* aValues, size_t aSize)
{
    Impl::SerializeSpan(aValues, aSize, myCurrObj, IsReading());
}

void JsonSerializer::SerializeSpan(uint8_t* aValues, size_t aSize)
{
    Impl::SerializeSpan(aValues, aSize, myCurrObj, IsReading());
}

void JsonSerializer::SerializeSpan(uint16_t* aValues, size_t aSize)
{
    Impl::SerializeSpan(aValues, aSize, myCurrObj, IsReading());
}

void JsonSerializer::SerializeSpan(uint32_t* aValues, size_t aSize)
{
    Impl::SerializeSpan(aValues, aSize, myCurrObj, IsReading());
}

void JsonSerializer::SerializeSpan(uint64_t* aValues, size_t aSize)
{
    Impl::SerializeSpan(aValues, aSize, myCurrObj, IsReading());
}

void JsonSerializer::SerializeSpan(int8_t* aValues, size_t aSize)
{
    Impl::SerializeSpan(aValues, aSize, myCurrObj, IsReading());
}

void JsonSerializer::SerializeSpan(int16_t* aValues, size_t aSize)
{
    Impl::SerializeSpan(aValues, aSize, myCurrObj, IsReading());
}

void JsonSerializer::SerializeSpan(int32_t* aValues, size_t aSize)
{
    Impl::SerializeSpan(aValues, aSize, myCurrObj, IsReading());
}

void JsonSerializer::SerializeSpan(int64_t* aValues, size_t aSize)
{
    Impl::SerializeSpan(aValues, aSize, myCurrObj, IsReading());
}

void JsonSerializer::SerializeSpan(float* aValues, size_t aSize)
{
    Impl::SerializeSpan(aValues, aSize, myCurrObj, IsReading());
}

void JsonSerializer::SerializeSpan(std::string* aValues, size_t aSize)
{
    Impl::SerializeSpan(aValues, aSize, myCurrObj, IsReading());
}

void JsonSerializer::SerializeSpan(glm::vec2* aValues, size_t aSize)
{
    Impl::SerializeSpan(aValues, aSize, myCurrObj, IsReading());
}

void JsonSerializer::SerializeSpan(glm::vec3* aValues, size_t aSize)
{
    Impl::SerializeSpan(aValues, aSize, myCurrObj, IsReading());
}

void JsonSerializer::SerializeSpan(glm::vec4* aValues, size_t aSize)
{
    Impl::SerializeSpan(aValues, aSize, myCurrObj, IsReading());
}

void JsonSerializer::SerializeSpan(glm::quat* aValues, size_t aSize)
{
    Impl::SerializeSpan(aValues, aSize, myCurrObj, IsReading());
}

void JsonSerializer::SerializeSpan(glm::mat4* aValues, size_t aSize)
{
    Impl::SerializeSpan(aValues, aSize, myCurrObj, IsReading());
}

namespace nlohmann
{
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