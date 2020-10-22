#pragma once

#include <variant>
#include <map>

class VariantMap
{
public:
    template<class... TArgs>
    struct VariantTypes
    {
        using Variant = std::variant<
            TArgs...,
            std::vector<TArgs>...
        >;
        template<class T>
        constexpr static bool Contains = 
            (std::is_same_v<T, TArgs> || ...)
            || (std::is_convertible_v<T, TArgs> || ...);
    };
    using SupportedTypes = VariantTypes<bool,
        uint64_t, 
        int64_t,
        float,
        std::string,
        VariantMap
    >;
    using Variant = typename SupportedTypes::Variant;

    template<class T, std::enable_if_t<SupportedTypes::Contains<T>, bool> = true>
    void Get(const std::string& aKey, T& aValue) const;

    /* json serializer doesn't support ordering, for now
    template<class T, std::enable_if_t<SupportedTypes::Contains<T>, bool> = true>
    void Get(size_t anIndex, T& aValue) const;*/

    template<class T, std::enable_if_t<SupportedTypes::Contains<T>, bool> = true>
    void Get(const std::string& aKey, std::vector<T>& aValue) const;

    /* json serializer doesn't support ordering, for now
    template<class T, std::enable_if_t<SupportedTypes::Contains<T>, bool> = true>
    void Get(size_t anIndex, std::vector<T>& aValue) const;*/

    template<class T, std::enable_if_t<std::is_enum_v<T>, bool> = true>
    void Get(const std::string& aKey, T& aValue) const
    {
        uint64_t enumVal = 0;
        Get(aKey, enumVal);
        aValue = static_cast<T>(enumVal);
    }

    template<class T, std::enable_if_t<SupportedTypes::Contains<T>, bool> = true>
    void Set(const std::string& aKey, const T& aValue, bool aCanOverride = false);

    template<class T, std::enable_if_t<SupportedTypes::Contains<T>, bool> = true>
    void Set(const std::string& aKey, const std::vector<T>& aValue, bool aCanOverride = false);

    template<class T, std::enable_if_t<std::is_enum_v<T>, bool> = true>
    void Set(const std::string& aKey, const T& aValue, bool aCanOverride = false)
    {
        Set(aKey, static_cast<uint64_t>(aValue), aCanOverride);
    }

    size_t GetSize() const { return myVariants.size(); }

    auto begin() { return myVariants.begin(); }
    const auto begin() const { return myVariants.begin(); }
    auto end() { return myVariants.end(); }
    const auto end() const { return myVariants.end(); }

private:
    const Variant& GetVariant(const std::string& aKey) const;
    const Variant& GetVariant(size_t anIndex) const;

    std::map<std::string, Variant> myVariants;
};