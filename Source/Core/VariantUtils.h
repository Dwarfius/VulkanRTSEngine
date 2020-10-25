#pragma once

#include <type_traits>
#include <utility>
#include <variant>

template<class... Types>
struct VariantUtil
{
    using VariantType = std::variant<Types...>;
    constexpr static int kNotFound = -1;

    template<class T>
    constexpr static bool Contains = (std::is_same_v<T, Types> || ...);

    template<class TFind, template<class, class> class TChecker>
    constexpr static int FindIndexOf()
    {
        return FindFirstImpl<TFind, TChecker>(Seq{});
    }

    template<class TFind, template<class, class> class TChecker>
    using FindMatchingType = std::variant_alternative_t<
        FindIndexOf<TFind, TChecker>(), VariantType
    >;

    template<class TFind, template<class, class> class TChecker>
    constexpr static bool Matches = FindIndexOf<TFind, TChecker>() != kNotFound;

private:
    using Seq = std::index_sequence_for<Types...>;
    template<class TFind, template<class, class> class TChecker,
        class T, T... Ints>
        constexpr static int FindFirstImpl(std::integer_sequence<T, Ints...> aSeq)
    {
        int foundIndex = kNotFound;
        (void)((TChecker<TFind, std::variant_alternative_t<Ints, VariantType>>::value ?
            (foundIndex = Ints, true) : false) || ...);
        return foundIndex;
    }
};

// A stricter version of is_convertible that restricts 
// arithmetic types (except bool) to use same class of 
// numbers (integer vs real) and same sign for promotion. 
// For non-arithmetic types falls back to is_convertible
template<class TFrom, class TTo>
struct IsImplicitlyPromotable : std::integral_constant<bool,
    std::is_arithmetic_v<TFrom> && std::is_arithmetic_v<TTo> ?
    (
        // 1. check arithmtic types match
        (std::is_integral_v<TFrom> == std::is_integral_v<TTo>
            || std::is_floating_point_v<TFrom> == std::is_floating_point_v<TTo>)
        // 2. ensure they're either both bool or not
        && std::is_same_v<TFrom, bool> == std::is_same_v<TTo, bool>
        // 3. ensure same sign
        && std::is_signed_v<TFrom> == std::is_signed_v<TTo>
        // 4. ensure promotion results in expansion, not shrinking!
        && sizeof(TFrom) <= sizeof(TTo)
    ) : std::is_convertible_v<TFrom, TTo>> {};