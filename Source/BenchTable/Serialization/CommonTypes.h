#pragma once

class SerializerStream;

// tempalted to simplify benchmark testing
template<class SerializerT>
class Resource
{
public:
    Resource() = default;
    Resource(const std::string& aPath) : myPath(aPath) {}
    const std::string& GetPath() const { return myPath; }
    virtual void Serialize(SerializerT& serializer) = 0;
private:
    std::string myPath;
};

// use example/test
template<class SerializerT>
class B : public Resource<SerializerT>
{
public:
    using Resource::Resource;
    void Serialize(SerializerT&) override final
    {
    }
};

template<class SerializerT>
class A : public Resource<SerializerT>
{
public:
    void Serialize(SerializerT& serializer) override final
    {
        int version = kVersion;
        serializer.SerializeVersion(version);
        serializer.Serialize("myInt", myInt);
        serializer.Serialize("myString", myString);
        if (version >= 2)
        {
            serializer.Serialize("myNewBool", myNewBool);
        }
        if (version >= 3)
        {
            serializer.Serialize("myB", myB);
        }
    }

private:
    constexpr static int kVersion = 3;
    int myInt{ 0 };
    std::string myString{};
    bool myNewBool{ false };
    B< SerializerT> myB{ "abc" };
};

constexpr static const char* kNominal = "{\"version\":3,\"myInt\":0,\"myString\":\"\",\"myNewBool\":false,\"myB\":{\"Res\":\"abc\"}}";
constexpr static const char* kOldVer = "{\"version\":2,\"myInt\":0,\"myString\":\"\",\"myNewBool\":false}";
constexpr static const char* kNoVer = "{\"myInt\":0,\"myString\":\"\"}";