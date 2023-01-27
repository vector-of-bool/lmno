#pragma once

#include <neo/declval.hpp>

#include <ranges>

namespace lmno::stdlib {

namespace _sr = std::ranges;

constexpr inline struct as_range_fn {
    template <typename R>
    constexpr decltype(auto) operator()(R&& r) const noexcept
        requires _sr::range<R>  //
        or requires {
               { r.as_range() } -> _sr::range;
           }
    {
        if constexpr (_sr::range<R>) {
            return NEO_FWD(r);
        } else {
            return NEO_FWD(r).as_range();
        }
    }
} as_range;

template <typename T>
using as_range_t = decltype(as_range(NEO_DECLVAL(T)));

template <typename T>
concept as_range_convertible = requires { typename as_range_t<T>; };

template <typename T>
concept viewable_range_convertible = as_range_convertible<T> and _sr::viewable_range<as_range_t<T>>;

template <typename T>
concept input_range_convertible = as_range_convertible<T> and _sr::input_range<as_range_t<T>>;

template <typename T>
concept bidirectional_range_convertible
    = as_range_convertible<T> and _sr::bidirectional_range<as_range_t<T>>;

template <as_range_convertible T>
using range_reference_t = _sr::range_reference_t<as_range_t<T>>;

template <as_range_convertible T>
using range_value_t = _sr::range_value_t<as_range_t<T>>;

}  // namespace lmno::stdlib