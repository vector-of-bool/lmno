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

struct plus {
    template <typename W>
    constexpr auto operator()(W&& w, addable<W> auto&& x) const NEO_RETURNS(w + x);

    template <typename X>
    static auto error() {
        return err::fmt_error_t<"The {:'} operator is not unary-invocable", cx_str{"+"}>{};
    }

    template <typename W, typename X>
    static auto error() {
        if constexpr (not addable<unconst_t<W>, unconst_t<X>>) {
            return err::fmt_error_t<
                "α of type {:'} and ω of type {:'} are not compatible by "
                "arithmetic addition in (α + ω)",
                render::type_v<W>,
                render::type_v<X>>{};
        }
    }
};

inline auto _recip  = [] NEO_CTL(rational{_1}.recip());
inline auto _divide = [] NEO_CTL(rational{_1} / _2);
struct divide_or_reciprocal : polyfun<func_wrap<_recip>, func_wrap<_divide>> {
    template <typename X>
    static auto error() {
        using render::type_v;
        if constexpr (not neo::constructible_from<rational, unconst_t<X>>) {
            return err::fmt_error_t<
                "Value of type {:'} cannot be converted to a rational number (i.e. lmno::rational)",
                type_v<X>>{};
        }
    }

    template <typename W, typename X>
    static auto error() {
        using render::type_v;
        using B = decltype(error<X>());
        if constexpr (not neo::same_as<B, void>) {
            return B{};
        } else if constexpr (not dividable_by<rational, unconst_t<X>>) {
            return err::fmt_error_t<
                "Value of type {:'} is not valid as the divisor of a rational number",
                type_v<X>>{};
        }
    }
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
inline auto _exponential = [] NEO_CTL(_power(rational{271'801, 99'990}, _1));

struct power_or_exponential : polyfun<func_wrap<_exponential>, func_wrap<_power>> {};

inline auto sign  = [] NEO_CTL((_1 < 0) ? -1 : (_1 > 0) ? 1 : 0);
inline auto times = [](auto&& w, multipliable<decltype(w)> auto&& x) NEO_RETURNS_L(w * x);

struct times_or_sign : polyfun<func_wrap<sign>, func_wrap<times>> {};

inline auto negative = [] NEO_CTL(-_1);
inline auto minus    = [] NEO_CTL(_1 - _2);
struct minus_or_negative : polyfun<func_wrap<negative>, func_wrap<minus>> {};

struct requires_equality_comparable {
    template <typename W, typename X, typename Wu = unconst_t<W>, typename Xu = unconst_t<X>>
    static auto error() {
        if constexpr (not neo::equality_comparable_with<W, X>) {
            return err::fmt_error_t<
                "Values of type {:'} are not equality-comparable with values of type {:'}",
                render::type_v<W>,
                render::type_v<X>>{};
        }
    }
};

inline auto _eq =
    [](auto&& w, neo::equality_comparable_with<decltype(w)> auto&& x) NEO_RETURNS_L(w == x);
struct equal : func_wrap<_eq>, requires_equality_comparable {};

inline auto _neq = [] NEO_CTL(not _eq(_1, _2));
struct not_equal : func_wrap<_neq>, requires_equality_comparable {};

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

// Bind the operands to their common_reference type, and the perform a three-way copmare
inline auto normalize_and_compare
    = []<typename W, neo::totally_ordered_with<W> X>(W&& w, X&& x) noexcept {
          using common = neo::common_reference_t<W, X>;
          common w1    = w;
          common x1    = x;
          return std::compare_strong_order_fallback(w1, x1);
      };

inline auto _lt = [] NEO_CTL(normalize_and_compare(_1, _2) < 0);
struct less : func_wrap<_lt>, requires_total_ordering {};

inline auto _lte = [] NEO_CTL(normalize_and_compare(_1, _2) <= 0);
struct less_equal : func_wrap<_lte>, requires_total_ordering {};

inline auto _gt = [] NEO_CTL(normalize_and_compare(_1, _2) > 0);
struct greater : func_wrap<_gt>, requires_total_ordering {};

inline auto _gte = [] NEO_CTL(normalize_and_compare(_1, _2) >= 0);
struct greater_equal : func_wrap<_gte>, requires_total_ordering {};

inline auto _min   = [] NEO_CTL(_lt(_1, _2) ? _1 : _2);
inline auto _floor = [] NEO_CTL(rational{_1}.floor());
struct min_or_floor : polyfun<func_wrap<_floor>, func_wrap<_min>>, requires_total_ordering {};

inline auto _max  = [] NEO_CTL(+(_lt(unconst(_1), unconst(_2)) ? unconst(_2) : unconst(_1)));
inline auto _ceil = [] NEO_CTL(rational{unconst(_1)}.ceil());

struct max_or_ceil : polyfun<func_wrap<_ceil>, func_wrap<_max>>, requires_total_ordering {};

inline auto mod = [] NEO_CTL(_1 % _2);

constexpr inline auto abs = []<typename X>(const X& x) noexcept -> X
    requires requires {
                 requires neo::totally_ordered<X>;
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

struct mod_or_abs : polyfun<func_wrap<abs>, func_wrap<mod>> {};

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
DECL_DEFINE("⌊", "minumum/floor", stdlib::min_or_floor);
DECL_DEFINE("⌈", "maxumum/ceiling", stdlib::max_or_ceil);
DECL_DEFINE("|", "modulus/absolute-value", stdlib::mod_or_abs);
#undef DECL_DEFINE

}  // namespace lmno
