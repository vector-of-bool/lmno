#pragma once

#include "../define.hpp"
#include "../func_wrap.hpp"
#include "../render.hpp"
#include "./valences.hpp"

#include <neo/returns.hpp>
#include <neo/tl.hpp>
#include <neo/type_traits.hpp>

namespace lmno::stdlib {

template <typename A, typename B>
concept addable =  //
    requires(A&& a, B&& b) {
        { a + b };
        { b + a };
        { a + b + a };
        { a + b + b };
        { b + a + b };
        { b + a + a };
        { a - b };
        { b - a };
        { a - b - a };
        { a - b - b };
        { b - a - b };
        { b - a - b };
    };

template <typename L, typename R>
concept multipliable =  //
    requires(const L& l, const R& r) {
        { (l * r) };
        { (l * r) * l };
        { (l * r) * r };
        { (r * l) };
        { (r * l) * l };
        { (r * l) * r };
    };

template <typename Numer, typename Denom>
concept dividable_by =  //
    requires(const Numer& num, const Denom& den) {
        { num / den } -> multipliable<Denom>;
        { (num / den) / den } -> multipliable<Denom>;
    };

constexpr inline auto _power = []<typename Base, typename Power>(const Base& b, Power p)
    requires requires {
                 { b* b } -> neo::weak_same_as<Base>;
                 { p = p - p };
             }
{
    Base acc = Base(1);
    while (p > Power(0)) {
        acc = acc * b;
        p   = p - Power(1);
    }
    return acc;
};

constexpr inline auto _mod
    = []<typename Numer, neo::totally_ordered_with<Numer> Denom>(const Numer& b,
                                                                 const Denom& d) noexcept  //
    -> Numer { return std::modulus<>{}(b, d); };

constexpr inline auto _abs = []<neo::totally_ordered X>(const X& x) noexcept -> X
    requires requires {
                 static_cast<X>(0);
                 { -x } -> neo::weak_same_as<X>;
             }
{
    if (x < static_cast<X>(0)) {
        return -x;
    } else {
        return x;
    }
};

constexpr inline auto _exponential =
    [](const auto& x) NEO_RETURNS_L(lmno::invoke(_power, rational{271'801, 99'990}, NEO_FWD(x)));

constexpr inline auto _equal
    = []<typename W, neo::equality_comparable_with<W> X>(const W& w, const X& x) noexcept -> int {
    return w == x;
};

constexpr inline auto _not_equal
    = []<typename W, neo::equality_comparable_with<W> X>(const W& w, const X& x) noexcept -> int {
    return w != x;
};

constexpr inline auto _less
    = []<typename W, neo::totally_ordered_with<W> X>(const W& w, const X& x) noexcept -> int {
    using common = neo::common_reference_t<W, X>;
    common w1    = w;
    common x1    = x;
    return std::compare_strong_order_fallback(w1, x1) < 0;
};

constexpr inline auto _less_equal
    = []<typename W, neo::totally_ordered_with<W> X>(const W& w, const X& x) noexcept -> int {
    using common = neo::common_reference_t<W, X>;
    common w1    = w;
    common x1    = x;
    return std::compare_strong_order_fallback(w1, x1) <= 0;
};

constexpr inline auto _greater
    = []<typename W, neo::totally_ordered_with<W> X>(const W& w, const X& x) noexcept -> int {
    using common = neo::common_reference_t<W, X>;
    common w1    = w;
    common x1    = x;
    return std::compare_strong_order_fallback(w1, x1) > 0;
};

constexpr inline auto _greater_equal
    = []<typename W, neo::totally_ordered_with<W> X>(const W& w, const X& x) noexcept -> int {
    using common = neo::common_reference_t<W, X>;
    common w1    = w;
    common x1    = x;
    return std::compare_strong_order_fallback(w1, x1) >= 0;
};

constexpr inline auto _max = []<typename W, neo::totally_ordered_with<W> X>(W&& w, X&& x) noexcept
    -> neo::common_reference_t<W, X> {
    using common   = neo::common_reference_t<W, X>;
    common w1      = w;
    common x1      = x;
    auto   compare = std::compare_strong_order_fallback(w1, x1);
    if (compare < 0) {
        return x;
    } else {
        return w;
    }
};

constexpr inline auto _min = []<typename W, neo::totally_ordered_with<W> X>(W&& w, X&& x) noexcept
    -> neo::common_reference_t<W, X> {
    using common   = neo::common_reference_t<W, X>;
    common w1      = w;
    common x1      = x;
    auto   compare = std::compare_strong_order_fallback(w1, x1);
    if (compare < 0) {
        return w;
    } else {
        return x;
    }
};

struct plus {
    LMNO_INDIRECT_INVOCABLE(plus);

    template <typename W>
    constexpr static auto call(W&& w, addable<W> auto&& x) NEO_RETURNS(w + x);

    template <typename X>
    static auto error() {
        using show = err::shower<plus, X>;
        return err::fmt_errorex_t<show, "The {:'} operator is not unary-invocable", cx_str{"+"}>{};
    }

    template <typename W, typename X>
    static auto error() {
        if constexpr (not addable<unconst_t<W>, unconst_t<X>>) {
            return err::fmt_errorex_t<err::shower<plus, W, X>,
                                      "α of type {:'} and ω of type {:'} are not compatible by "
                                      "arithmetic addition in (α + ω)",
                                      render::type_v<W>,
                                      render::type_v<X>>{};
        }
    }
};

constexpr inline auto _reciprocal = [](const auto& x) NEO_RETURNS_L(rational(x).recip());
constexpr inline auto _divide     = [](const auto& w, const auto& x) NEO_RETURNS_L(rational{w} / x);

struct reciprocal : func_wrap<_reciprocal> {
    template <typename X>
    static auto error() {
        using render::type_v;
        if constexpr (not neo::constructible_from<rational, unconst_t<X>>) {
            return err::fmt_error_t<
                "Value of type {:'} cannot be converted to a rational number (i.e. lmno::rational)",
                type_v<X>>{};
        }
    }
};
struct divide : func_wrap<_divide> {
    template <typename W, typename X>
    static auto error() {
        using render::type_v;
        if constexpr (not neo::constructible_from<rational, unconst_t<W>>) {
            return err::fmt_error_t<
                "Value of type {:'} cannot be converted to a rational number (i.e. lmno::rational)",
                type_v<W>>{};
        } else if constexpr (not dividable_by<rational, unconst_t<X>>) {
            return err::fmt_error_t<
                "Value of type {:'} is not valid as the divisor of a rational number",
                type_v<X>>{};
        }
    }
};

struct power : func_wrap<_power> {};
struct exponential : func_wrap<_exponential> {};

struct times_or_sign {
    LMNO_INDIRECT_INVOCABLE(times_or_sign);

    constexpr static auto call(auto&& x) NEO_RETURNS((x < 0) ? -1 : 1);

    template <typename L, multipliable<L> R>
    constexpr static auto call(const L& l, const R& r) NEO_RETURNS(l* r);
};

struct minus_or_negative {
    LMNO_INDIRECT_INVOCABLE(minus_or_negative);

    constexpr static auto call(auto&& w, auto&& x) NEO_RETURNS(w - x);
    constexpr static auto call(auto&& x) NEO_RETURNS(-x);
};

struct divide_or_reciprocal : polyfun<reciprocal, divide> {
    LMNO_INDIRECT_INVOCABLE(divide_or_reciprocal);
};
struct power_or_exponential : polyfun<exponential, power> {
    LMNO_INDIRECT_INVOCABLE(power_or_exponential);
};

struct requires_total_ordering {
    template <typename W, typename X, typename Wu = unconst_t<W>, typename Xu = unconst_t<X>>
    static auto error() {
        if constexpr (not neo::totally_ordered_with<W, X>) {
            return err::fmt_error_t<
                "Values of type {:'} are not totally-ordered with values of type {:'}",
                render::type_v<W>,
                render::type_v<X>>{};
        }
    }
};

struct equal : func_wrap<_equal> {
    LMNO_INDIRECT_INVOCABLE(equal);
};
struct not_equal : func_wrap<_not_equal> {
    LMNO_INDIRECT_INVOCABLE(not_equal);
};
struct less : func_wrap<_less>, requires_total_ordering {
    LMNO_INDIRECT_INVOCABLE(less);
};
struct less_equal : func_wrap<_less_equal>, requires_total_ordering {
    LMNO_INDIRECT_INVOCABLE(less_equal);
};
struct greater : func_wrap<_greater>, requires_total_ordering {
    LMNO_INDIRECT_INVOCABLE(greater);
};
struct greater_equal : func_wrap<_greater_equal>, requires_total_ordering {
    LMNO_INDIRECT_INVOCABLE(greater_equal);
};
struct min : func_wrap<_min> {
    LMNO_INDIRECT_INVOCABLE(min);
};
struct max : func_wrap<_max> {
    LMNO_INDIRECT_INVOCABLE(max);
};

struct abs : func_wrap<_abs> {
    LMNO_INDIRECT_INVOCABLE(abs);
};
struct mod : func_wrap<_mod> {
    LMNO_INDIRECT_INVOCABLE(mod);
};
struct abs_or_mod : polyfun<abs, mod> {
    LMNO_INDIRECT_INVOCABLE(abs_or_mod);
};

}  // namespace lmno::stdlib

namespace lmno {

#define DECL_DEFINE(Symbol, HumanName, Func)                                                       \
    template <>                                                                                    \
    constexpr inline auto define<Symbol> = Func{};                                                 \
    template <>                                                                                    \
    constexpr inline auto render::type_v<Func>                                                     \
        = cx_fmt_v<"{:'}", cx_fmt_v<"{} ({})", cx_str{Symbol}, cx_str{HumanName}>>

DECL_DEFINE("=", "equality", stdlib::equal);

DECL_DEFINE("+", "addition", stdlib::plus);
DECL_DEFINE("×", "multiply/sign-of", stdlib::times_or_sign);
DECL_DEFINE("-", "subtract/negative-of", stdlib::minus_or_negative);
DECL_DEFINE("÷", "divide/reciprocal-of", stdlib::divide_or_reciprocal);
DECL_DEFINE("^", "power/expondential-of", stdlib::power_or_exponential);
DECL_DEFINE("≠", "inequality", stdlib::not_equal);
DECL_DEFINE(">", "greater-than", stdlib::greater);
DECL_DEFINE("≥", "greater-than-or-equal-to", stdlib::greater_equal);
DECL_DEFINE("<", "less-than", stdlib::less);
DECL_DEFINE("≤", "less-than-or-equal-to", stdlib::less_equal);
DECL_DEFINE("⌊", "minumum", stdlib::min);  // TODO: Define ceil with rationals
DECL_DEFINE("⌈", "maxumum", stdlib::max);  // TODO: Define floor with rationals
DECL_DEFINE("|", "modulus/absolute-value", stdlib::abs_or_mod);
#undef DECL_DEFINE

}  // namespace lmno
