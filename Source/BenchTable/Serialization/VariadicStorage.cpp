#include "Precomp.h"
#include "CommonTypes.h"

// helps with requirement of ordered read/writes, but is slower to populate...
// because we don't care about fast random lookups and want to capitalize on sequential
// lookups, can be replaced with a std::vector!
template<class K, class V, class dummy_compare, class A>
using fifo_map = nlohmann::fifo_map<K, V, nlohmann::fifo_map_compare<K>, A>;
using json = nlohmann::basic_json<fifo_map>;

// central piece, connects the resource classes with the serialized storage
// Or more like a stream that accumulates the data before passing it through the
// compressors or connecting with other data
class SerializerStream
{
public:
    struct ResourceProxy { std::string myPath; };
    using VariantType = std::variant<bool, int, float, std::string, ResourceProxy>;
    using NameType = std::string; // or 16-32byte static string?
    using EntryType = std::pair<NameType, VariantType>;
    using Storage = std::deque<EntryType>; // can be repalced with a vector to avoid double deref
    // in order to quickly populate or read from
    struct Access {
        static Storage& Get(SerializerStream& aStream) { return aStream.myEntries; }
        static const Storage& Get(const SerializerStream& aStream) { return aStream.myEntries; }
    };

    SerializerStream(bool aIsReading) : myIsReading(aIsReading) {};

    void SerializeVersion(int& aVersion)
    {
        if (myIsReading)
        {
            const EntryType entry = myEntries.front();

            // TODO: name the magic string constant
            if (entry.first.compare("version") == 0)
            {
                ASSERT_STR(std::holds_alternative<int>(entry.second), "Version msut hold int!");
                aVersion = std::get<int>(entry.second);
                myEntries.pop_front();
            }
            else
            {
                aVersion = 1;
            }
        }
        else
        {
            myEntries.emplace_back("version", aVersion);
        }
    }

    template<class T>
    void Serialize(const std::string_view& aName, T& aValue)
    {
        if (myIsReading)
        {
            const EntryType entry = myEntries.front();
            ASSERT_STR(entry.first.compare(aName) == 0, "Missmatching name!");

            if constexpr (std::is_base_of_v<Resource<SerializerStream>, T>)
            {
                ASSERT_STR(std::holds_alternative<ResourceProxy>(entry.second), "ResProxy invalid type!");
                ResourceProxy proxy = std::get<ResourceProxy>(entry.second);
                aValue = T(proxy.myPath);
            }
            else
            {
                ASSERT_STR(std::holds_alternative<T>(entry.second), "Mismatching type!");
                aValue = std::get<T>(entry.second);
            }
            myEntries.pop_front();
        }
        else
        {
            if constexpr (std::is_base_of_v<typename Resource<SerializerStream>, T>)
            {
                ResourceProxy proxy;
                proxy.myPath = aValue.GetPath();
                myEntries.emplace_back(aName, proxy);
            }
            else
            {
                myEntries.emplace_back(aName, aValue);
            }
        }
    }
private:
    Storage myEntries;
    bool myIsReading;
};

namespace nlohmann
{
    template<>
    struct adl_serializer<SerializerStream::ResourceProxy>
    {
        static void to_json(::json& j, const SerializerStream::ResourceProxy& res)
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

        static void from_json(const ::json& j, SerializerStream::ResourceProxy& res)
        {
            if (!j.is_null())
            {
                res.myPath = j["Res"].get<std::string>();
            }
        }
    };
}

// Serializers that know how to read from the storage
class JSONSerializer // produces a string representation
{
    //using json = nlohmann::json;
public:
    void ReadFrom(const SerializerStream& aStream)
    {
        myJsonObj.clear();
        const SerializerStream::Storage& storage = SerializerStream::Access::Get(aStream);
        for (const SerializerStream::EntryType& entry : storage)
        {
            std::visit([&](auto arg) {
                myJsonObj[std::string(entry.first)] = arg;
            }, entry.second);
        }
    }

    void WriteTo(SerializerStream& aStream) const
    {
        SerializerStream::Storage& storage = SerializerStream::Access::Get(aStream);
        storage.clear();
        for (auto& [key, value] : myJsonObj.items())
        {
            SerializerStream::EntryType entry;
            // <bool, int, float, std::string, ResourceProxy>;
            switch (value.type())
            {
            case json::value_t::boolean:
                entry = SerializerStream::EntryType(key, value.get<bool>());
                break;
            case json::value_t::number_unsigned:
            case json::value_t::number_integer:
                entry = SerializerStream::EntryType(key, value.get<int>());
                break;
            case json::value_t::number_float:
                entry = SerializerStream::EntryType(key, value.get<float>());
                break;
            case json::value_t::string:
                entry = SerializerStream::EntryType(key, value.get<std::string>());
                break;
            case json::value_t::object:
                entry = SerializerStream::EntryType(key, value.get<SerializerStream::ResourceProxy>());
                break;
            default:
                ASSERT(false);
                break;
            }
            storage.push_back(entry);
        }
    }

    void Parse(const std::string_view& buffer)
    {
        myJsonObj = json::parse(buffer, nullptr, false);
    }

    std::string Convert() const
    {
        return myJsonObj.dump();
    }

private:
    json myJsonObj;
};

static void WriteVariantStorage(benchmark::State& aState)
{
    A<SerializerStream> a;
    for (auto _ : aState)
    {
        JSONSerializer serializer;
        SerializerStream stream(false);
        a.Serialize(stream);
        serializer.ReadFrom(stream);
    }
}

static void ReadVariantStorage(benchmark::State& aState)
{
    A<SerializerStream> a;
    for (auto _ : aState)
    {
        JSONSerializer serializer;
        serializer.Parse(kNominal);
        SerializerStream stream(true);
        serializer.WriteTo(stream);
        a.Serialize(stream);
    }
}

static void UpgradeVariantStorage(benchmark::State& aState)
{
    A<SerializerStream> a;
    for (auto _ : aState)
    {
        JSONSerializer serializer;
        serializer.Parse(kOldVer);
        SerializerStream stream(true);
        serializer.WriteTo(stream);
        a.Serialize(stream);
    }
}

static void NoVerVariantStorage(benchmark::State& aState)
{
    A<SerializerStream> a;
    for (auto _ : aState)
    {
        JSONSerializer serializer;
        serializer.Parse(kNoVer);
        SerializerStream stream(true);
        serializer.WriteTo(stream);
        a.Serialize(stream);
    }
}

BENCHMARK(WriteVariantStorage);
BENCHMARK(ReadVariantStorage);
BENCHMARK(UpgradeVariantStorage);
BENCHMARK(NoVerVariantStorage);