#pragma once

#include "VariantUtils.h"
#include <map>

class VariantMap
{
public:
    using SupportedTypes = VariantUtil<
        bool,
        uint64_t, 
        int64_t,
        float,
        VariantMap,
        std::string,
        std::vector<bool>,
        std::vector<uint64_t>,
        std::vector<int64_t>,
        std::vector<float>,
        std::vector<std::string>,
        std::vector<VariantMap>
    >;
    using Variant = typename SupportedTypes::VariantType;

    template<class T, std::enable_if_t<SupportedTypes::Matches<T, std::is_convertible>, bool> = true>
    void Get(const std::string& aKey, T& aValue) const;

    // Be warned, JSON serializer doesn't support ordering for now!
    template<class T, std::enable_if_t<SupportedTypes::Matches<T, std::is_convertible>, bool> = true>
    void Get(size_t anIndex, T& aValue) const;

    template<class T, std::enable_if_t<SupportedTypes::Contains<T>, bool> = true>
    void Get(const std::string& aKey, std::vector<T>& aValue) const;

    // Be warned, JSON serializer doesn't support ordering for now!
    template<class T, std::enable_if_t<SupportedTypes::Contains<T>, bool> = true>
    void Get(size_t anIndex, std::vector<T>& aValue) const;

    template<class T, std::enable_if_t<std::is_enum_v<T>, bool> = true>
    void Get(const std::string& aKey, T& aValue) const
    {
        uint64_t enumVal = 0;
        Get(aKey, enumVal);
        aValue = static_cast<T>(enumVal);
    }

    template<class T, std::enable_if_t<SupportedTypes::Matches<T, std::is_convertible>, bool> = true>
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