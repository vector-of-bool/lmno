#pragma once

#include "../define.hpp"
#include "../invoke.hpp"
#include "../render.hpp"
#include "./arithmetic.hpp"
#include "./constants.hpp"
#include "./ranges.hpp"

#include <neo/attrib.hpp>
#include <neo/returns.hpp>
#include <neo/type_traits.hpp>

#include <algorithm>
#include <ranges>

namespace lmno::stdlib {

namespace _sr = std::ranges;

/*
 .d88888b.                                                            888
d88P" "Y88b                                                           888
888     888                                                           888
888     888 888  888  .d88b.  888d888       .d88b.   8888b.   .d8888b 88888b.
888     888 888  888 d8P  Y8b 888P"        d8P  Y8b     "88b d88P"    888 "88b
888     888 Y88  88P 88888888 888   888888 88888888 .d888888 888      888  888
Y88b. .d88P  Y8bd8P  Y8b.     888          Y8b.     888  888 Y88b.    888  888
 "Y88888P"    Y88P    "Y8888  888           "Y8888  "Y888888  "Y8888P 888  888
*/

template <typename F, _sr::view V>
    requires invocable<F, _sr::range_reference_t<V>>
struct over_each_view {
    NEO_NO_UNIQUE_ADDRESS F _func;
    NEO_NO_UNIQUE_ADDRESS V _view;

    using transform_view = std::ranges::transform_view<V, F>;

    over_each_view() = default;

    constexpr explicit over_each_view(F f, V v) noexcept
        : _func(NEO_FWD(f))
        , _view(NEO_FWD(v)) {}

    constexpr auto as_range() const noexcept { return transform_view(_view, _func); }
};
LMNO_AUTO_CTAD_GUIDE(over_each_view);

template <typename F>
struct over_each {
    NEO_NO_UNIQUE_ADDRESS F _fn;

    LMNO_INDIRECT_INVOCABLE(over_each);

    template <viewable_range_convertible R>
        requires invocable<F, range_reference_t<R>>
    constexpr auto call(R&& r)
        NEO_RETURNS(over_each_view{_fn, std::views::all(as_range(NEO_FWD(r)))});

    template <viewable_range_convertible R>
        requires invocable<F, range_reference_t<R>>
    constexpr auto call(R&& r) const
        NEO_RETURNS(over_each_view{_fn, std::views::all(as_range(NEO_FWD(r)))});

    template <typename X>
    static auto error() {
        using Xu = unconst_t<X>;
        using err::fmt_error_t;
        using render::type_v;
        if constexpr (not input_range_convertible<Xu>) {
            return fmt_error_t<"Argument of type {:'} is not an input-range", type_v<X>>{};
        } else if constexpr (not viewable_range_convertible<Xu>) {
            return fmt_error_t<"Argument of type {:'} is a range, but is not a viewable range.",
                               type_v<X>>{};
        } else {
            using ref = range_reference_t<Xu>;
            if constexpr (not invocable<F, ref>) {
                return err::fmt_errorex_t<
                    invoke_error_t<F, ref>,
                    "Over-each function {:'} is not unary-invocable with the range's "
                    "reference-type {:'} (from range of type {:'})",
                    type_v<F>,
                    type_v<ref>,
                    type_v<X>>{};
            }
        }
    }
};
LMNO_AUTO_CTAD_GUIDE(over_each);

/*
8888888b.
888   Y88b
888    888
888   d88P .d88b.  888  888  .d88b.  888d888 .d8888b   .d88b.
8888888P" d8P  Y8b 888  888 d8P  Y8b 888P"   88K      d8P  Y8b
888 T88b  88888888 Y88  88P 88888888 888     "Y8888b. 88888888
888  T88b Y8b.      Y8bd8P  Y8b.     888          X88 Y8b.
888   T88b "Y8888    Y88P    "Y8888  888      88888P'  "Y8888
*/

template <_sr::view V>
struct reverse_view {
    NEO_NO_UNIQUE_ADDRESS V _view;

    reverse_view() = default;
    constexpr explicit reverse_view(V v) noexcept
        : _view(NEO_FWD(v)) {}

    constexpr auto as_range() const noexcept { return std::views::reverse(_view); }
};
LMNO_AUTO_CTAD_GUIDE(reverse_view);

struct reverse {
    LMNO_INDIRECT_INVOCABLE(reverse);

    template <viewable_range_convertible R>
        requires bidirectional_range_convertible<R>
    constexpr auto call(R&& r) const
        NEO_RETURNS(reverse_view(std::views::all(as_range(NEO_FWD(r)))));

    template <typename R_, typename R = unconst_t<R_>>
    static auto error() {
        if constexpr (not viewable_range_convertible<R>) {
            return err::fmt_error_t<"Type {:'} is not a viewable-range">{};
        } else if constexpr (not bidirectional_range_convertible<R>) {
            return err::fmt_error_t<"Type {:'} is not a bidirectional range">{};
        }
    }
};

}  // namespace lmno::stdlib

namespace lmno {

template <>
constexpr auto inline define<"¨"> = [](auto&& f) NEO_RETURNS_L(stdlib::over_each{NEO_FWD(f)});

template <>
constexpr auto inline define<"⌽"> = stdlib::reverse{};

}  // namespace lmno