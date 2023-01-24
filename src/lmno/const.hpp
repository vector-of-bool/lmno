#pragma once

#include "./concepts/typed_constant.hpp"

#include <neo/fwd.hpp>
#include <neo/returns.hpp>
#include <neo/type_traits.hpp>

#include <compare>
#include <concepts>
#include <cstdint>

namespace lmno {

using neo::remove_cvref_t;

template <auto V, typename Type = remove_cvref_t<decltype(V)>>
struct Const : typed_constant_base {
    /**
     * @brief The actual value that is bound within the type
     */
    static constexpr const auto& value = V;

    /**
     * @brief The type of the value of this constant.
     */
    using type = remove_cvref_t<decltype(V)>;

    static_assert(std::same_as<type, Type>, "Do not specify a different type for Const<>");

    // An explicit conversion to the underlying type is allowed:
    constexpr explicit operator type const&() const noexcept { return value; }

    template <typename Other>
        requires std::constructible_from<Other, const type&>
    constexpr explicit operator Other() const noexcept {
        return static_cast<Other>(value);
    }

    template <auto U>
        requires std::equality_comparable_with<type, decltype(U)>
    [[nodiscard]] constexpr bool operator==(Const<U>) const noexcept {
        return V == U;
    }
    template <auto U>
        requires std::totally_ordered_with<type, decltype(U)>
    [[nodiscard]] constexpr auto operator<=>(Const<U>) const noexcept {
        return V <=> U;
    }

    [[nodiscard]] constexpr bool operator==(const type& t) const noexcept { return value == t; }
    [[nodiscard]] constexpr auto operator<=>(const type& t) const noexcept { return value <=> t; }

#define DECL_WRAPPER(Func)                                                                         \
    [[nodiscard]] constexpr decltype(auto) Func(auto&&... args) const noexcept                     \
        requires requires { value.Func(NEO_FWD(args)...); }                                        \
    {                                                                                              \
        return value.Func(NEO_FWD(args)...);                                                       \
    }

    DECL_WRAPPER(size);
    DECL_WRAPPER(data);
    DECL_WRAPPER(begin);
    DECL_WRAPPER(end);
    DECL_WRAPPER(cbegin);
    DECL_WRAPPER(cend);
    DECL_WRAPPER(front);
    DECL_WRAPPER(back);

#undef DECL_WRAPPER
};

template <std::int64_t N>
using ConstInt64 = Const<N>;

}  // namespace lmno
