#pragma once

#include "./ast.hpp"
#include "./define.hpp"
#include "./eval.hpp"

#include <neo/fwd.hpp>
#include <neo/invoke.hpp>
#include <neo/ranges.hpp>
#include <neo/returns.hpp>

#include <algorithm>

namespace lmno {

namespace _sr = std::ranges;

// π is rational 'round these parts :)
template <>
constexpr auto define<"π"> = Const<rational{104348 / 33215}>{};

struct infinity {};

// ∞ is it's own thing...
template <>
constexpr auto define<"∞"> = infinity{};

/**
 * @brief Create a bivalent function that invokes one of two functions depending on whether the
 * polyfun is invoked with one or two arguments. Use with CTAD.
 *
 * @tparam F The unary function to be invoked when given one argument.
 * @tparam G The binary function to be invoked when given two argumetns.
 *
 * @note This can created in code with the "⊘" infix operator (\-%)
 */
template <typename F, typename G>
struct polyfun {
    NEO_NO_UNIQUE_ADDRESS F _unary;
    NEO_NO_UNIQUE_ADDRESS G _binary;

    constexpr auto operator()(auto&& x) LMNO_INVOKES(_unary, NEO_FWD(x));
    constexpr auto operator()(auto&& x) const LMNO_INVOKES(_unary, NEO_FWD(x));

    constexpr auto operator()(auto&& w, auto&& x) LMNO_INVOKES(_binary, NEO_FWD(w), NEO_FWD(x));
    constexpr auto operator()(auto&& w, auto&& x) const
        LMNO_INVOKES(_binary, NEO_FWD(w), NEO_FWD(x));

    template <typename L, typename R = void>
    using error_t = neo::
        conditional_t<neo_is_void(R), maybe_invoke_error_t<F, L>, maybe_invoke_error_t<G, L, R>>;
};
LMNO_AUTO_CTAD_GUIDE(polyfun);

/**
 * @brief Create a function that unconditionally returns the given value. Use with CTAD.
 *
 * @tparam T The type of value that will be returned.
 *
 * @note This is created using the "high-dit"/"constant" operator spelled "˙" (\-")
 */
template <typename T>
struct const_fn {
    NEO_NO_UNIQUE_ADDRESS T _value;

    constexpr auto operator()(auto&&) NEO_RETURNS(_value);
    constexpr auto operator()(auto&&) const NEO_RETURNS(_value);
    constexpr auto operator()(auto&&, auto&& x) NEO_RETURNS(_value(NEO_FWD(x)));
    constexpr auto operator()(auto&&, auto&& x) const NEO_RETURNS(_value(NEO_FWD(x)));

    constexpr static auto monadic_name() noexcept {
        return cx_fmt_v<"(constant-func {})", render::type_v<T>>;
    }

    constexpr static auto monadic_name() noexcept
        requires typed_constant<T>
    {
        return "˙" + ast::render_constant_value_v<T>;
    }

    constexpr static auto dyadic_name() noexcept { return cx_fmt_v<"(˙ {})", render::type_v<T>>; }

    constexpr static auto dyadic_name() noexcept
        requires typed_constant<T>
    {
        return "˙" + ast::render_constant_value_v<T>;
    }
};
LMNO_AUTO_CTAD_GUIDE(const_fn);

template <typename T>
constexpr auto render::type_v<const_fn<T>> = cx_fmt_v<"˙{:'}", render::type_v<T>>;

struct left_id {
    constexpr decltype(auto) operator()(auto&& x) const noexcept { return NEO_FWD(x); }
    constexpr decltype(auto) operator()(auto&& w, auto&&) const noexcept { return NEO_FWD(w); }
};

template <>
constexpr auto define<"⊣"> = left_id{};

struct right_id {
    constexpr decltype(auto) operator()(auto&& x) const noexcept { return NEO_FWD(x); }
    constexpr decltype(auto) operator()(auto&&, auto&& x) const noexcept { return NEO_FWD(x); }
};

template <>
constexpr auto define<"⊢"> = right_id{};

// Select the function
struct valence_modifier {
    template <typename F, typename G>
    constexpr auto operator()(F&& f, G&& g) NEO_RETURNS(polyfun{NEO_FWD(f), NEO_FWD(g)});
};

template <>
constexpr auto define<"⊘"> = valence_modifier{};

struct constant_mod {
    constexpr auto operator()(auto&& x) const NEO_RETURNS(const_fn{NEO_FWD(x)});
};

template <>
constexpr auto define<"˙"> = constant_mod{};

template <typename T>
constexpr bool autoconst_v = std::is_arithmetic_v<T>;

template <>
constexpr bool autoconst_v<rational> = true;

template <auto V>
constexpr bool autoconst_v<Const<V>> = autoconst_v<typename Const<V>::type>;

template <typename... Ts>
constexpr bool autoconst_v<strand_range<Ts...>> = true;

template <>
constexpr bool autoconst_v<infinity> = true;

/**
 * @brief If given an object that is marked as "auto-const", return a const_fn that will return that
 * object. Otherwise returns the object unchanged.
 */
template <typename T>
constexpr decltype(auto) autoconst_fn(T&& arg) noexcept {
    if constexpr (autoconst_v<remove_cvref_t<T>>) {
        return const_fn{NEO_FWD(arg)};
    } else {
        return unconst(NEO_FWD(arg));
    }
}

// The "⟜" closure
template <typename After, typename Before>
struct after {
    NEO_NO_UNIQUE_ADDRESS After  _aft;
    NEO_NO_UNIQUE_ADDRESS Before _bef;

    constexpr auto operator()(auto&& x) LMNO_INVOKES(*this, x, x);
    constexpr auto operator()(auto&& x) const LMNO_INVOKES(*this, x, x);

    constexpr auto operator()(auto&& w, auto&& x)
        LMNO_INVOKES(_aft, NEO_FWD(w), lmno::invoke(_bef, NEO_FWD(x)));
    constexpr auto operator()(auto&& w, auto&& x) const
        LMNO_INVOKES(_aft, NEO_FWD(w), lmno::invoke(_bef, NEO_FWD(x)));

    template <typename X,
              bool InBefore = requires { Before::template error<X>(); },
              bool InAfter  = requires { After::template error<X, invoke_t<Before, X>>(); }>
        requires InBefore or InAfter
    constexpr static auto error() noexcept {
        if constexpr (InBefore) {
            return Before::template error<X>();
        } else {
            return After::template error<X, invoke_t<Before, X>>();
        }
    }
};
LMNO_AUTO_CTAD_GUIDE(after);

struct after_mod {
    constexpr auto operator()(auto&& f, auto&& g) const
        NEO_RETURNS(after{unconst(NEO_FWD(f)), autoconst_fn(NEO_FWD(g))});
};

template <>
constexpr auto define<"⟜"> = after_mod{};

template <typename A, typename B>
constexpr auto render::type_v<after<A, B>>
    = cx_fmt_v<"({} ⟜ {})", render::type_v<A>, render::type_v<B>>;

// The "⊸" closure
template <typename Before, typename After>
struct before {
    NEO_NO_UNIQUE_ADDRESS Before _bef;
    NEO_NO_UNIQUE_ADDRESS After  _aft;

    constexpr auto operator()(auto&& x) LMNO_INVOKES(_aft, lmno::invoke(_bef, x), x);
    constexpr auto operator()(auto&& x) const LMNO_INVOKES(_aft, lmno::invoke(_bef, x), x);

    constexpr auto operator()(auto&& w, auto&& x)
        LMNO_INVOKES(_aft, lmno::invoke(_bef, NEO_FWD(w)), NEO_FWD(x));
    constexpr auto operator()(auto&& w, auto&& x) const
        LMNO_INVOKES(_aft, lmno::invoke(_bef, NEO_FWD(w)), NEO_FWD(x));

    constexpr static auto monadic_name() noexcept {
        return cx_fmt_v<"({} ⊸ {})", ast::monadic_name<Before>(), ast::dyadic_name<After>()>;
    }
};
LMNO_AUTO_CTAD_GUIDE(before);

struct before_mod {
    constexpr auto operator()(auto&& f, auto&& g) const
        NEO_RETURNS(before{autoconst_fn(NEO_FWD(f)), NEO_FWD(g)});
};

template <>
constexpr auto define<"⊸"> = before_mod{};

// The "∘" closure
template <typename F, typename G>
struct atop {
    NEO_NO_UNIQUE_ADDRESS F _f;
    NEO_NO_UNIQUE_ADDRESS G _g;

    constexpr auto operator()(auto&& x) LMNO_INVOKES(_f, lmno::invoke(_g, NEO_FWD(x)));
    constexpr auto operator()(auto&& x) const LMNO_INVOKES(_f, lmno::invoke(_g, NEO_FWD(x)));

    constexpr auto operator()(auto&& w, auto&& x)
        LMNO_INVOKES(_f, lmno::invoke(_g, NEO_FWD(w), NEO_FWD(x)));
    constexpr auto operator()(auto&& w, auto&& x) const
        LMNO_INVOKES(_f, lmno::invoke(_g, NEO_FWD(w), NEO_FWD(x)));

    template <typename X, typename Y = void>
    using error_t = decltype([] {
        if constexpr (neo_is_same(Y, void)) {
            using f_result = invoke_t<F, X>;
            if constexpr (any_error<f_result>) {
                return err::fmt_errorex_t<
                    f_result,
                    "Invoking function {:'} with argument of type {:'} resulted in an error",
                    render::type_v<F>,
                    render::type_v<X>>{};
            } else {
                using g_result = invoke_t<G, f_result>;
                return err::fmt_errorex_t<  //
                    g_result,
                    "Invoking function {:'} with an argument of type {:'} produced\n"
                    "a value of type {:'}.\n"
                    "Invoking function {:'} with {:'} resulted in an error.",
                    render::type_v<F>,
                    render::type_v<X>,
                    render::type_v<f_result>,
                    render::type_v<G>,
                    render::type_v<f_result>>{};
            }
        } else {
            using f_result = invoke_t<F, X, Y>;
            if constexpr (any_error<f_result>) {
                return err::fmt_errorex_t<  //
                    f_result,
                    "Invoking function {:'} with arguments of type {:'} and {:'} \n"
                    "resulted in an error",
                    render::type_v<F>,
                    render::type_v<X>,
                    render::type_v<Y>>{};
            } else {
                using g_result = invoke_t<G, f_result>;
                return err::fmt_errorex_t<
                    g_result,
                    "Invoking function {:'} with an argument of types {:'} and {:'}\n"
                    "produced a value of type {:'}.\n"
                    "Invoking function {:'} with {:'} resulted in an error.",
                    render::type_v<F>,
                    render::type_v<X>,
                    render::type_v<Y>,
                    render::type_v<f_result>,
                    render::type_v<G>,
                    render::type_v<f_result>>{};
            }
        }
    }());
};
LMNO_AUTO_CTAD_GUIDE(atop);

template <typename F, typename G>
constexpr auto render::type_v<atop<F, G>>
    = cx_fmt_v<"({} ∘ {})", render::type_v<F>, render::type_v<G>>;

struct atop_mod {
    constexpr auto operator()(auto&& f, auto&& g) const NEO_RETURNS(atop{NEO_FWD(f), NEO_FWD(g)});
};

template <>
constexpr auto define<"∘"> = atop_mod{};

// The "○" closure
template <typename F, typename G>
struct over {
    NEO_NO_UNIQUE_ADDRESS F _f;
    NEO_NO_UNIQUE_ADDRESS G _g;

    constexpr auto operator()(auto&& x) LMNO_INVOKES(_f, lmno::invoke(_g, NEO_FWD(x)));
    constexpr auto operator()(auto&& x) const LMNO_INVOKES(_f, lmno::invoke(_g, NEO_FWD(x)));

    constexpr auto operator()(auto&& w, auto&& x)
        LMNO_INVOKES(_f, lmno::invoke(_g, NEO_FWD(w)), lmno::invoke(_g, NEO_FWD(x)));
    constexpr auto operator()(auto&& w, auto&& x) const
        LMNO_INVOKES(_f, lmno::invoke(_g, NEO_FWD(w)), lmno::invoke(_g, NEO_FWD(x)));
};
LMNO_AUTO_CTAD_GUIDE(over);

struct over_mod {
    constexpr auto operator()(auto&& f, auto&& g) const NEO_RETURNS(over{NEO_FWD(f), NEO_FWD(g)});
};

template <>
constexpr auto define<"○"> = over_mod{};

// The "φ" closure, equivalent to an APL fork
template <typename F, typename H, typename G>
struct phi {
    F _f;
    H _h;
    G _g;

    constexpr auto operator()(auto&& x) LMNO_INVOKES(_h, lmno::invoke(_f, x), lmno::invoke(_g, x));
    constexpr auto operator()(auto&& x) const
        LMNO_INVOKES(_h, lmno::invoke(_f, x), lmno::invoke(_g, x));

    constexpr auto operator()(auto&& w, auto&& x)
        LMNO_INVOKES(_h, lmno::invoke(_f, w, x), lmno::invoke(_g, w, x));
    constexpr auto operator()(auto&& w, auto&& x) const
        LMNO_INVOKES(_h, lmno::invoke(_f, w, x), lmno::invoke(_g, w, x));
};
LMNO_AUTO_CTAD_GUIDE(phi);

// The intermediate when creating a φ closure or "fork"
template <typename H>
struct phi_partial {
    NEO_NO_UNIQUE_ADDRESS H _h;

    constexpr auto operator()(auto&& f, auto&& g)
        NEO_RETURNS(phi{NEO_FWD(f), NEO_FWD(_h), NEO_FWD(g)});
    constexpr auto operator()(auto&& f, auto&& g) const
        NEO_RETURNS(phi{NEO_FWD(f), _h, NEO_FWD(g)});
};
LMNO_AUTO_CTAD_GUIDE(phi_partial);

// Applies to the central function of the fork:
struct phi_mod {
    constexpr auto operator()(auto&& f) const NEO_RETURNS(phi_partial{NEO_FWD(f)});
};

template <>
constexpr auto define<"φ"> = phi_mod{};

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
    template <typename L, addable<L> R>
    constexpr auto operator()(L&& l, R&& r) const NEO_RETURNS(NEO_FWD(l) + NEO_FWD(r));

    template <typename>
    static constexpr std::string_view error() noexcept {
        return "The '+' operator does not support unary prefix invocation.";
    }

    template <typename L, typename R = void>
        requires(not addable<L, R>)
    using error_t = neo::conditional_t<
        neo::void_type<R>,
        err::fmt_error_t<"The {:'} operator does not support unary prefix invocation.",
                         cx_str{"+"}>,
        err::fmt_error_t<"The operands {:'} and {:'} are not compatible for arithmetic addition",
                         render::type_v<L>,
                         render::type_v<R>>>;

    static constexpr std::string_view dyadic_name() noexcept { return "arithmetic addition"; }
};

template <>
constexpr auto define<"+"> = lmno::plus{};

struct times {
    template <typename L, multipliable<L> R>
    constexpr auto operator()(L&& l, R&& r) const NEO_RETURNS(NEO_FWD(l) * NEO_FWD(r));
};

template <>
constexpr auto define<"×"> = lmno::times{};

struct minus {
    template <typename R>
        requires requires(const R& r) {
                     { -r };
                 }
    constexpr auto operator()(R&& arg) const NEO_RETURNS(-NEO_FWD(arg));

    template <typename L, typename R>
        requires requires(const L& l, const R& r) {
                     { l - r };
                     { l + (l - r) };
                 }
    constexpr auto operator()(L&& l, R&& r) const NEO_RETURNS(NEO_FWD(l) - NEO_FWD(r));
};

template <>
constexpr auto define<"-"> = lmno::minus{};

// Unary "÷"
struct recip {
    template <std::convertible_to<rational> Num>
    constexpr auto operator()(const Num& x) const {
        return rational{x}.recip();
    }

    template <typename Arg>
    constexpr static auto error() noexcept {
        using T = unconst_t<Arg>;
        if constexpr (not std::convertible_to<T, rational>) {
            return cx_fmt_v<
                "Type {:'} does not define an implicit conversion to [lmno::rational], which is "
                "required for taking its reciprocal.",
                render::type_v<T>>;
        }
    }
};

// Binary "÷"
struct divide_by {
    template <typename Denom, dividable_by<Denom> Numer>
        requires std::constructible_from<rational, Denom> and dividable_by<rational, Denom>
    constexpr auto operator()(const Numer& num, const Denom& den) const {
        return rational{num} / den;
    }

    template <typename Numer, typename Denom>
    constexpr static auto error() noexcept {
        return cx_fmt_v<
            "Values of type {:'} cannot be divided by values of type {:'}.\n(Note: that the "
            "result of the division must be multipliable by {:'} as well!)",
            render::type_v<Numer>,
            render::type_v<Denom>,
            render::type_v<Denom>>;
    }
};

struct divide : polyfun<recip, divide_by> {};

template <>
constexpr auto define<"÷"> = divide{};

struct pow {
    template <typename I>
        requires requires(I v) {
                     { v* v } -> neo::weak_same_as<I>;
                 }
    constexpr auto operator()(const I& w, auto x) const {
        auto acc = I(1);
        while (x > 0) {
            acc = acc * w;
            x   = x - 1;
        }
        return acc;
    }

    constexpr auto operator()(auto&& x) const
        LMNO_INVOKES(*this, rational{271'801, 99'990}, NEO_FWD(x));
};

template <>
constexpr auto define<"^"> = pow{};

template <typename T, typename Operator>
constexpr auto make_no_id_elem_error_str() {
    constexpr auto M = cx_fmt_v<"No identity-element for type {:'} with binary operation {:'}",
                                render::type_v<T>,
                                render::type_v<Operator>>;
    return err::error_type<M>{};
}

template <typename Type, typename Operator>
constexpr auto identity_element = make_no_id_elem_error_str<Type, Operator>();

template <std::integral I>
constexpr I identity_element<I, plus> = I(0);

template <std::integral I>
constexpr I identity_element<I, minus> = I(0);

template <std::integral I>
constexpr I identity_element<I, times> = I(1);

template <std::integral I>
constexpr I identity_element<I, divide> = I(1);

// The "/" closure
template <typename F>
struct fold {
    NEO_NO_UNIQUE_ADDRESS F _fold;

    template <typename Init, typename T>
    using common_type_t = std::common_type_t<Init, neo::invoke_result_t<F, Init, T>>;

    template <_sr::input_range R>
        requires invocable<F, _sr::range_reference_t<R>, _sr::range_reference_t<R>>
    constexpr decltype(auto) operator()(R&& in) const {
        return (*this)(identity_element<_sr::range_value_t<R>, F>, in);
    }

    template <_sr::input_range R>
        requires invocable<F, _sr::range_reference_t<R>, _sr::range_reference_t<R>>
    constexpr decltype(auto) operator()(R&& in) {
        return (*this)(identity_element<_sr::range_value_t<R>, F>, in);
    }

    template <neo::incrementable Init, _sr::input_range R>
        requires requires { typename common_type_t<Init, _sr::range_reference_t<R>>; }
    constexpr decltype(auto) operator()(Init init, R&& in) const {
        common_type_t<Init, _sr::range_reference_t<R>> value = init;
        _sr::for_each(in, [&](auto&& elem) {
            value = lmno::invoke(_fold, NEO_MOVE(value), NEO_FWD(elem));
        });
        return value;
    }

    template <neo::incrementable Init, _sr::input_range R>
        requires requires { typename common_type_t<Init, _sr::range_reference_t<R>>; }
    constexpr decltype(auto) operator()(Init init, R&& in) {
        common_type_t<Init, _sr::range_reference_t<R>> value = init;
        _sr::for_each(in, [&](auto&& elem) {
            value = lmno::invoke(_fold, NEO_MOVE(value), NEO_FWD(elem));
        });
        return value;
    }

    constexpr static auto dyadic_name() noexcept {
        return cx_str{"fold-reduction by "} + ast::dyadic_name<F>();
    }

    constexpr static auto monadic_name() noexcept {
        return cx_str{"fold-reduction by "} + ast::dyadic_name<F>();
    }

    template <typename Arg>
    constexpr static auto error() noexcept {
        if constexpr (not _sr::input_range<Arg>) {
            return cx_str{"The argument must be an input range"};
        } else if constexpr (not invocable<F,
                                           _sr::range_reference_t<Arg>,
                                           _sr::range_reference_t<Arg>>) {
            return cx_fmt_v<
                "The range elements (of type {:'}) are not valid operands for the binary "
                "function "
                "{:'}: {}",
                render::type_v<_sr::range_reference_t<Arg>>,
                render::type_v<F>,
                invoke_t<F, _sr::range_reference_t<Arg>, _sr::range_reference_t<Arg>>::message>;
        } else {
            return cx_str{"lol"};
        }
    }

    template <typename Init, typename Arg>
    constexpr static auto error() noexcept {
        if constexpr (not _sr::input_range<Arg>) {
            return cx_fmt_v<"The right-hand argument {:'} is not an input-range",
                            render::type_v<Arg>>;
        } else if constexpr (not neo::has_common_type<Init, _sr::range_reference_t<Arg>>) {
            return cx_fmt_v<
                "There is no common type between the init-value of type {:'} and the range's "
                "element type {:'}",
                render::type_v<Init>,
                render::type_v<_sr::range_reference_t<Arg>>>;
        } else if constexpr (not invocable<F,
                                           _sr::range_reference_t<Arg>,
                                           _sr::range_reference_t<Arg>>) {
            return cx_fmt_v<
                "The range elements (of type {:'}) are not valid operands for the binary "
                "function "
                "{:'}: {}",
                render::type_v<_sr::range_reference_t<Arg>>,
                render::type_v<F>,
                invoke_t<F, _sr::range_reference_t<Arg>, _sr::range_reference_t<Arg>>::message>;
        } else {
            return cx_str{"lol"};
        }
    }
};
LMNO_AUTO_CTAD_GUIDE(fold);

template <typename F>
constexpr auto render::type_v<fold<F>> = cx_fmt_v<"(fold over {:'})", render::type_v<F>>;

struct fold_modifier {
    constexpr auto operator()(auto&& fn) const NEO_RETURNS(fold{NEO_FWD(fn)});
};

template <>
constexpr auto define<"/"> = lmno::fold_modifier{};

// The "\" closure
template <typename F>
struct scan {
    NEO_NO_UNIQUE_ADDRESS F _fn;

    template <typename Init, typename T>
    using common_type_t = std::common_type_t<Init, neo::invoke_result_t<F, Init, T>>;

    template <_sr::input_range R>
        requires neo::invocable2<F, _sr::range_reference_t<R>, _sr::range_reference_t<R>>
    constexpr decltype(auto) operator()(R&& in) const {
        return (*this)(identity_element<_sr::range_value_t<R>, F>, in);
    }

    template <_sr::input_range R>
        requires neo::invocable2<F, _sr::range_reference_t<R>, _sr::range_reference_t<R>>
    constexpr decltype(auto) operator()(R&& in) {
        return (*this)(identity_element<_sr::range_value_t<R>, F>, in);
    }

    template <typename Init, _sr::input_range R>
    constexpr decltype(auto) operator()(Init&& init, R&& in) const {
        common_type_t<Init, _sr::range_reference_t<R>> value = NEO_FWD(init);
        std::vector<decltype(value)>                   vec;
        _sr::for_each(in, [&](auto&& elem) {
            value = _fn(NEO_MOVE(value), NEO_FWD(elem));
            vec.push_back(value);
        });
        return vec;
    }

    template <typename Init, _sr::input_range R>
    constexpr decltype(auto) operator()(Init&& init, R&& in) {
        common_type_t<Init, _sr::range_reference_t<R>> value = NEO_FWD(init);
        std::vector<decltype(value)>                   vec;
        _sr::for_each(in, [&](auto&& elem) {
            value = _fn(NEO_MOVE(value), NEO_FWD(elem));
            vec.push_back(value);
        });
        return vec;
    }

    constexpr static auto dyadic_name() noexcept {
        return cx_str{"scan by "} + ast::dyadic_name<F>();
    }

    constexpr static auto monadic_name() noexcept {
        return cx_str{"scan by "} + ast::dyadic_name<F>();
    }

    template <typename Arg>
    constexpr static auto error() noexcept {
        if constexpr (not _sr::input_range<Arg>) {
            return cx_str{"The argument must be an input range"};
        } else if constexpr (not neo::invocable2<F,
                                                 _sr::range_reference_t<Arg>,
                                                 _sr::range_reference_t<Arg>>) {
            return cx_fmt_v<
                "The range of elements (of type {:'}) are not valid operands for the binary "
                "operation {:'}",
                render::type_v<_sr::range_reference_t<Arg>>,
                ast::dyadic_name<F>()>;
        }
    }
};
LMNO_AUTO_CTAD_GUIDE(scan);

struct scan_modifier {
    constexpr auto operator()(auto&& fn) const NEO_RETURNS(scan{NEO_FWD(fn)});
};

template <>
constexpr auto define<"\\"> = lmno::scan_modifier{};

template <auto Min = std::int64_t(0)>
struct infinite_iota_range : _sr::view_interface<infinite_iota_range<Min>> {
    constexpr auto begin() const noexcept { return std::views::iota(Min).begin(); }
    constexpr auto end() const noexcept { return std::views::iota(Min).end(); }
};

template <typename Min, typename Max>
struct iota_range {
    NEO_NO_UNIQUE_ADDRESS Min mn;
    NEO_NO_UNIQUE_ADDRESS Max mx;

    constexpr auto begin() const noexcept { return std::views::iota(mn, mx).begin(); }
    constexpr auto end() const noexcept { return std::views::iota(mn, mx).end(); }

    constexpr auto begin() noexcept { return std::views::iota(mn, mx).begin(); }
    constexpr auto end() noexcept { return std::views::iota(mn, mx).end(); }
};
LMNO_AUTO_CTAD_GUIDE(iota_range);

template <typename Min, typename Max, iota_range<Min, Max> R>
constexpr auto render::value_of_type_v<iota_range<Min, Max>, R>
    = cx_fmt_v<"({} ⍳ {})", render::value_v<R.mn>, render::value_v<R.mx>>;

// The type of the "⍳" function
struct iota_generator {
    enum { typed_constant_aware = true };

    template <typename T>
        requires std::incrementable<unconst_t<T>>
    constexpr auto operator()(T max) const noexcept {  //
        return iota_range{unconst(max - max), unconst(max)};
    }

    template <typename Min, typename Max>
        requires requires {
                     typename std::common_type_t<unconst_t<Min>, unconst_t<Max>>;
                     requires std::incrementable<unconst_t<Min>>;
                     requires std::incrementable<unconst_t<Max>>;
                 }
    constexpr auto operator()(Min min, Max max) const noexcept {
        return iota_range{unconst(min), unconst(max)};
    }

    constexpr auto operator()(infinity) const noexcept { return infinite_iota_range<>{}; }
    constexpr auto operator()(typed_constant auto l, infinity) const noexcept {
        return infinite_iota_range<l.value>{};
    }
    constexpr auto operator()(std::int64_t left, infinity) const noexcept {
        return iota_range{left, infinity{}};
    }
};

template <>
constexpr auto define<"⍳"> = lmno::iota_generator{};

struct max {
    template <typename W, std::totally_ordered_with<W> X>
        requires std::common_reference_with<W, X>
    constexpr std::common_reference_t<W, X> operator()(W&& w, X&& x) const noexcept {
        if (w < x) {
            return static_cast<std::common_reference_t<W, X>>(NEO_FWD(x));
        } else {
            return static_cast<std::common_reference_t<W, X>>(NEO_FWD(w));
        }
    }
};

template <>
constexpr auto define<"⌈"> = max{};  // TODO: Define ceil with rationals

// The "˜" closure
template <typename F>
struct self_swap {
    NEO_NO_UNIQUE_ADDRESS F _f;

    constexpr auto operator()(auto&& x) LMNO_INVOKES(NEO_FWD(_f), x, x);
    constexpr auto operator()(auto&& x) const LMNO_INVOKES(_f, x, x);
    constexpr auto operator()(auto&& w, auto&& x) LMNO_INVOKES(NEO_FWD(_f), NEO_FWD(x), NEO_FWD(w));
    constexpr auto operator()(auto&& w, auto&& x) const LMNO_INVOKES(_f, NEO_FWD(x), NEO_FWD(w));
};
LMNO_AUTO_CTAD_GUIDE(self_swap);

template <>
constexpr auto define<"˜"> = [](auto&& f) { return self_swap{NEO_FWD(f)}; };

// The "¨" closure
template <typename F>
struct over_each {
    NEO_NO_UNIQUE_ADDRESS F _fn;

    struct _invoke_tag;

    template <_sr::input_range R>
        requires invocable<F, _sr::range_reference_t<R>> and _sr::viewable_range<R>
    constexpr auto operator()(_invoke_tag*, _invoke_tag*, R&& r) {
        return std::views::transform(NEO_FWD(r), _fn);
    }

    template <_sr::input_range R>
        requires invocable<F, _sr::range_reference_t<R>> and _sr::viewable_range<R>
    constexpr auto operator()(_invoke_tag*, _invoke_tag*, R&& r) const {
        return std::views::transform(NEO_FWD(r), _fn);
    }

    /// XXX: This is a horrible hack to get invoke() to detect our requirements.
    /// TODO: Make it not so ugly...
    constexpr auto operator()(auto&& x)  //
        LMNO_INVOKES(*this,
                     static_cast<_invoke_tag*>(nullptr),
                     static_cast<_invoke_tag*>(nullptr),
                     NEO_FWD(x));
    constexpr auto operator()(auto&& x) const  //
        LMNO_INVOKES(*this,
                     static_cast<_invoke_tag*>(nullptr),
                     static_cast<_invoke_tag*>(nullptr),
                     NEO_FWD(x));

    constexpr auto operator()(auto&& w, auto&& x)  //
        LMNO_INVOKES(*this,
                     static_cast<_invoke_tag*>(nullptr),
                     static_cast<_invoke_tag*>(nullptr),
                     NEO_FWD(w),
                     NEO_FWD(x));
    constexpr auto operator()(auto&& w, auto&& x) const
        LMNO_INVOKES(*this,
                     static_cast<_invoke_tag*>(nullptr),
                     static_cast<_invoke_tag*>(nullptr),
                     NEO_FWD(w),
                     NEO_FWD(x));

    template <std::same_as<_invoke_tag*>, std::same_as<_invoke_tag*>, typename X>
    constexpr static auto error() noexcept {
        if constexpr (not _sr::input_range<X>) {
            return cx_str{"The right-hand argument to an over-each function must be a range"};
        } else if constexpr (not invocable<F, _sr::range_reference_t<X>>) {
            constexpr auto err = invoke_t<F, _sr::range_reference_t<X>>{};
            return cx_fmt_v<
                "The inner function {} is not\n  invocable with the range's element type "
                "({}):\n\n{}",
                quote_str_v<ast::monadic_name<F>()>,
                render::type_v<_sr::range_reference_t<X>>,
                err.message>;
        } else {
            return cx_fmt_v<"Unknown other issue trying to apply over-each {} for range {}",
                            quote_str_v<ast::monadic_name<F>()>,
                            quote_str_v<render::type_v<X>>>;
        }
    }

    constexpr static auto monadic_name() noexcept {
        return cx_fmt_v<"over-each[{}]", ast::monadic_name<F>>;
    }
};
LMNO_AUTO_CTAD_GUIDE(over_each);

template <>
constexpr auto define<"¨"> = [](auto&& f) { return over_each{NEO_FWD(f)}; };

// The "⌽" function
struct reverse {
    template <_sr::bidirectional_range R>
    // requires(not std::same_as<_sr::sentinel_t<R>, _sr::unreachable_t>)
    constexpr auto operator()(R&& r) const NEO_RETURNS(std::views::reverse(NEO_FWD(r)));
};

template <>
constexpr auto define<"⌽"> = reverse{};  /// TODO: Rotate!

// The "=" function
struct equal {
    template <typename Left, std::equality_comparable_with<Left> Right>
        requires(not typed_constant<Left> or not typed_constant<Right>)
    constexpr std::int64_t operator()(const Left& l, const Right& r) const noexcept {
        return l == r;
    }

    constexpr auto operator()(typed_constant auto left, typed_constant auto right) const noexcept {
        return ConstInt64<left.value == right.value>{};
    }
};

template <>
constexpr auto define<"="> = equal{};

}  // namespace lmno
