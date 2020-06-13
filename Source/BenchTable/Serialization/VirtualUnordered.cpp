#include "Precomp.h"
#include "CommonTypes.h"

using json = nlohmann::json;

// central piece, connects the resource classes with the serialized storage
// Or more like a stream that accumulates the data before passing it through the
// compressors or connecting with other data
class SerializerStreamUnorderedBase
{
public:
    struct ResourceProxy { std::string myPath; };
    using VariantType = std::variant<bool, int, float, std::string, ResourceProxy>;
    using NameType = std::string; // or 16-32byte static string?
    using EntryType = std::pair<NameType, VariantType>;

    SerializerStreamUnorderedBase(bool aIsReading) : myIsReading(aIsReading) {};

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
            if constexpr (std::is_base_of_v<Resource<SerializerStreamUnorderedBase>, T>)
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
                if constexpr (std::is_base_of_v<Resource<SerializerStreamUnorderedBase>, T>)
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
    struct adl_serializer<SerializerStreamUnorderedBase::ResourceProxy>
    {
        static void to_json(::json& j, const SerializerStreamUnorderedBase::ResourceProxy& res)
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

        static void from_json(const ::json& j, SerializerStreamUnorderedBase::ResourceProxy& res)
        {
            if (!j.is_null())
            {
                res.myPath = j["Res"].get<std::string>();
            }
        }
    };
}

// Serializers that know how to read from the storage
class JSONSerializerUnordered : public SerializerStreamUnorderedBase
{
public:
    using SerializerStreamUnorderedBase::SerializerStreamUnorderedBase;
private:
    std::string SerializeImpl() const override final
    {
        return myJsonObj.dump();
    }
    void DeserializeImpl(const std::string& aBuffer) override final
    {
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
            const json& foundObj = myJsonObj[std::string(aName)];

            if (!foundObj.is_null())
            {
                using ArgT = decltype(arg);
                aValue = foundObj.get<ArgT>();
            }
        }, aValue);
    }
    json myJsonObj;
};

static void WriteVirtualUnordered(benchmark::State& aState)
{
    A<SerializerStreamUnorderedBase> a;
    for (auto _ : aState)
    {
        JSONSerializerUnordered serializer(false);
        a.Serialize(serializer);
    }
}

static void ReadVirtualUnordered(benchmark::State& aState)
{
    A<SerializerStreamUnorderedBase> a;
    for (auto _ : aState)
    {
        JSONSerializerUnordered serializer(true);
        serializer.Parse(kNominal);
        a.Serialize(serializer);
    }
}

static void UpgradeVirtualUnordered(benchmark::State& aState)
{
    A<SerializerStreamUnorderedBase> a;
    for (auto _ : aState)
    {
        JSONSerializerUnordered serializer(true);
        serializer.Parse(kOldVer);
        a.Serialize(serializer);
    }
}

static void NoVerVirtualUnordered(benchmark::State& aState)
{
    A<SerializerStreamUnorderedBase> a;
    for (auto _ : aState)
    {
        JSONSerializerUnordered serializer(true);
        serializer.Parse(kNoVer);
        a.Serialize(serializer);
    }
}

BENCHMARK(WriteVirtualUnordered);
BENCHMARK(ReadVirtualUnordered);
BENCHMARK(UpgradeVirtualUnordered);
BENCHMARK(NoVerVirtualUnordered);