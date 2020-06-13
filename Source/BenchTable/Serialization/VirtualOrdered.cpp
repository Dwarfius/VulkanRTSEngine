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
class SerializerStreamBase
{
public:
    struct ResourceProxy { std::string myPath; };
    using VariantType = std::variant<bool, int, float, std::string, ResourceProxy>;
    using NameType = std::string; // or 16-32byte static string?
    using EntryType = std::pair<NameType, VariantType>;

    SerializerStreamBase(bool aIsReading) : myIsReading(aIsReading) {};

    std::string Convert() const
    {
        return SerializeImpl();
    }

    void Parse(const std::string& aBuffer)
    {
        return DeserializeImpl(aBuffer);
    }

    void SerializeVersion(int& aVersion)
    {
        if (myIsReading)
        {
            aVersion = 1;
        }
        Serialize("version", aVersion);
    }

    template<class T>
    void Serialize(const std::string_view& aName, T& aValue)
    {
        if (myIsReading)
        {
            if constexpr (std::is_base_of_v<Resource<SerializerStreamBase>, T>)
            {
                VariantType var(ResourceProxy{});
                DeserializeProperty(aName, var);
                ResourceProxy proxy = std::get<ResourceProxy>(var);
                aValue = T{ proxy.myPath };
            }
            else
            {
                VariantType var(T{});
                DeserializeProperty(aName, var);
                aValue = std::get<T>(var);
            }

        }
        else
        {
            VariantType var = [&] {
                if constexpr (std::is_base_of_v<Resource<SerializerStreamBase>, T>)
                {
                    ResourceProxy proxy;
                    proxy.myPath = aValue.GetPath();
                    return proxy;
                }
                else
                {
                    return aValue;
                }
            }();

            SerializeProperty(aName, var);
        }
    }

private:
    virtual std::string SerializeImpl() const = 0;
    virtual void DeserializeImpl(const std::string& aBuffer) = 0;
    virtual void SerializeProperty(const std::string_view& aName, const VariantType& aValue) = 0;
    virtual void DeserializeProperty(const std::string_view& aName, VariantType& aValue) = 0;
    bool myIsReading;
};

namespace nlohmann
{
    template<>
    struct adl_serializer<SerializerStreamBase::ResourceProxy>
    {
        static void to_json(::json& j, const SerializerStreamBase::ResourceProxy& res)
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

        static void from_json(const ::json& j, SerializerStreamBase::ResourceProxy& res)
        {
            if (!j.is_null())
            {
                res.myPath = j["Res"].get<std::string>();
            }
        }
    };
}

// Serializers that know how to read from the storage
class JSONSerializer : public SerializerStreamBase
{
public:
    using SerializerStreamBase::SerializerStreamBase;
private:
    std::string SerializeImpl() const override final
    {
        return myJsonObj.dump();
    }
    void DeserializeImpl(const std::string& aBuffer) override final
    {
        myIndex = 0;
        myJsonObj = json::parse(aBuffer, nullptr, false);
    }
    void SerializeProperty(const std::string_view& aName, const VariantType& aValue) override final
    {
        std::visit([&](auto arg) {
            myJsonObj[std::string(aName)] = arg;
        }, aValue);
    }
    void DeserializeProperty(const std::string_view& aName, VariantType& aValue) override final
    {
        std::visit([&](auto arg) {
            const json::object_t* objMap = myJsonObj.get_ptr<json::object_t*>();

            unsigned long index = myIndex;
            ASSERT_STR(index < objMap->size(), "Out of properties!");
            
            auto iter = objMap->begin();
            std::advance(iter, index);
            if (aName.compare(iter->first) != 0)
            {
                return;
            }
            const json& foundObj = iter->second;

            if (!foundObj.is_null())
            {
                myIndex++;
                using ArgT = decltype(arg);
                aValue = foundObj.get<ArgT>();
            }
        }, aValue);
    }
    json myJsonObj;
    int myIndex{ 0 };
};

static void WriteVirtualOrdered(benchmark::State& aState)
{
    A<SerializerStreamBase> a;
    for (auto _ : aState)
    {
        JSONSerializer serializer(false);
        a.Serialize(serializer);
    }
}

static void ReadVirtualOrdered(benchmark::State& aState)
{
    A<SerializerStreamBase> a;
    for (auto _ : aState)
    {
        JSONSerializer serializer(true);
        serializer.Parse(kNominal);
        a.Serialize(serializer);
    }
}

static void UpgradeVirtualOrdered(benchmark::State& aState)
{
    A<SerializerStreamBase> a;
    for (auto _ : aState)
    {
        JSONSerializer serializer(true);
        serializer.Parse(kOldVer);
        a.Serialize(serializer);
    }
}

static void NoVerVirtualOrdered(benchmark::State& aState)
{
    A<SerializerStreamBase> a;
    for (auto _ : aState)
    {
        JSONSerializer serializer(true);
        serializer.Parse(kNoVer);
        a.Serialize(serializer);
    }
}

BENCHMARK(WriteVirtualOrdered);
BENCHMARK(ReadVirtualOrdered);
BENCHMARK(UpgradeVirtualOrdered);
BENCHMARK(NoVerVirtualOrdered);