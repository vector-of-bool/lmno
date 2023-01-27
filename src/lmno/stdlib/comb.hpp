#pragma once

#include "../invoke.hpp"
#include "../rational.hpp"
#include "../render.hpp"
#include "../strand.hpp"
#include "./constants.hpp"

#include <neo/attrib.hpp>
#include <neo/returns.hpp>

namespace lmno::stdlib {

/*
  888
  888
  888
  888

  888  888888
  888
  888
  888
*/

struct left_id {
    template <typename X>
    constexpr X&& operator()(X&& x) const noexcept {
        return NEO_FWD(x);
    }
    template <typename W>
    constexpr W&& operator()(W&& w, auto&&) const noexcept {
        return NEO_FWD(w);
    }
};

/*
        888
        888
        888
        888

888888  888
        888
        888
        888
*/

struct right_id {
    template <typename X>
    constexpr X&& operator()(X&& x) const noexcept {
        return NEO_FWD(x);
    }
    template <typename X>
    constexpr X&& operator()(auto&&, X&& x) const noexcept {
        return NEO_FWD(x);
    }
};

/*
 .d8888b.                             888                      888
d88P  Y88b                            888                      888
888    888                            888                      888
888         .d88b.  88888b.  .d8888b  888888  8888b.  88888b.  888888
888        d88""88b 888 "88b 88K      888        "88b 888 "88b 888
888    888 888  888 888  888 "Y8888b. 888    .d888888 888  888 888
Y88b  d88P Y88..88P 888  888      X88 Y88b.  888  888 888  888 Y88b.
 "Y8888P"   "Y88P"  888  888  88888P'  "Y888 "Y888888 888  888  "Y888
*/

/**
 * @brief Create a function that unconditionally returns the given value. Use with CTAD.
 *
 * @tparam T The type of value that will be returned.
 *
 * @note This is created using the "high-dot"/"constant" operator spelled "˙" (\-")
 */
template <typename T>
struct const_fn {
    NEO_NO_UNIQUE_ADDRESS T _value;

    constexpr auto operator()(auto&&) NEO_RETURNS(_value);
    constexpr auto operator()(auto&&) const NEO_RETURNS(_value);
    constexpr auto operator()(auto&&, auto&& x) NEO_RETURNS(_value(NEO_FWD(x)));
    constexpr auto operator()(auto&&, auto&& x) const NEO_RETURNS(_value(NEO_FWD(x)));
};
LMNO_AUTO_CTAD_GUIDE(const_fn);

/**
 * @brief Variable template that determines when 'before' and 'after' will automatically
 * wrap an object in a const_fn
 */
template <typename T>
constexpr bool autoconst_v = std::is_arithmetic_v<T>;

template <>
constexpr bool autoconst_v<rational> = true;

template <auto V, typename T>
constexpr bool autoconst_v<Const<V, T>> = autoconst_v<T>;

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
    if constexpr (stdlib::autoconst_v<remove_cvref_t<T>>) {
        return stdlib::const_fn{NEO_FWD(arg)};
    } else {
        return unconst(NEO_FWD(arg));
    }
}

/*
       d8888  .d888 888
      d88888 d88P"  888
     d88P888 888    888
    d88P 888 888888 888888 .d88b.  888d888
   d88P  888 888    888   d8P  Y8b 888P"
  d88P   888 888    888   88888888 888
 d8888888888 888    Y88b. Y8b.     888
d88P     888 888     "Y888 "Y8888  888
*/

// The "⟜" closure
template <typename After, typename Before>
struct after {
    NEO_NO_UNIQUE_ADDRESS After  _after;
    NEO_NO_UNIQUE_ADDRESS Before _before;

    LMNO_INDIRECT_INVOCABLE(after);

    constexpr auto call(auto&& x) NEO_RETURNS(invoke(*this, x, x));
    constexpr auto call(auto&& x) const NEO_RETURNS(invoke(*this, x, x));

    constexpr auto call(auto&& w, auto&& x)
        NEO_RETURNS(invoke(_after, NEO_FWD(w), invoke(_before, NEO_FWD(x))));
    constexpr auto call(auto&& w, auto&& x) const
        NEO_RETURNS(invoke(_after, NEO_FWD(w), invoke(_before, NEO_FWD(x))));

    template <typename X>
    static auto error() {
        using render::type_v;
        if constexpr (not invocable<Before, X>) {
            return err::fmt_errorex_t<
                maybe_invoke_error_t<Before, X>,
                "Before-function {:'} is not invocable with an argument of type {:'}",
                type_v<Before>,
                type_v<X>>{};
        } else {
            using bef_t = invoke_t<Before, X>;
            if constexpr (not invocable<After, bef_t, X>) {
                return err::fmt_errorex_t<  //
                    maybe_invoke_error_t<After, bef_t, X>,
                    "Invoking left-hand before-function {:'} with value of\n"
                    "type {:'} results in a value of type {:'}.\n"
                    "\n"
                    "The after-function of type {:'} is not invocable with \n"
                    "arguments α of type {:'} and ω of type {:'}.",
                    type_v<Before>,
                    type_v<X>,
                    type_v<bef_t>,
                    type_v<After>,
                    type_v<bef_t>,
                    type_v<X>>{};
            }
        }
    }
};
LMNO_AUTO_CTAD_GUIDE(after);

/*
888888b.             .d888
888  "88b           d88P"
888  .88P           888
8888888K.   .d88b.  888888 .d88b.  888d888 .d88b.
888  "Y88b d8P  Y8b 888   d88""88b 888P"  d8P  Y8b
888    888 88888888 888   888  888 888    88888888
888   d88P Y8b.     888   Y88..88P 888    Y8b.
8888888P"   "Y8888  888    "Y88P"  888     "Y8888
*/

// The "⊸" closure
template <typename Before, typename After>
struct before {
    NEO_NO_UNIQUE_ADDRESS Before _before;
    NEO_NO_UNIQUE_ADDRESS After  _after;

    LMNO_INDIRECT_INVOCABLE(before);

    constexpr auto call(auto&& x) NEO_RETURNS(invoke(_after, invoke(_before, x), x));
    constexpr auto call(auto&& x) const NEO_RETURNS(invoke(_after, invoke(_before, x), x));

    constexpr auto call(auto&& w, auto&& x)
        NEO_RETURNS(invoke(_after, invoke(_before, NEO_FWD(w)), NEO_FWD(x)));
    constexpr auto call(auto&& w, auto&& x) const
        NEO_RETURNS(invoke(_after, invoke(_before, NEO_FWD(w)), NEO_FWD(x)));

    template <typename X>
    static auto error() {
        using render::type_v;
        if constexpr (not invocable<Before, X>) {
            return err::fmt_errorex_t<
                maybe_invoke_error_t<Before, X>,
                "Before-function {:'} is not invocable with an argument of type {:'}",
                type_v<Before>,
                type_v<X>>{};
        } else {
            using bef_t = invoke_t<Before, X>;
            if constexpr (not invocable<After, bef_t, X>) {
                return err::fmt_errorex_t<  //
                    maybe_invoke_error_t<After, bef_t, X>,
                    "Invoking left-hand before-function {:'} with value of\n"
                    "type {:'} results in a value of type {:'}.\n"
                    "\n"
                    "The after-function of type {:'} is not invocable with \n"
                    "arguments α of type {:'} and ω of type {:'}.",
                    type_v<Before>,
                    type_v<X>,
                    type_v<bef_t>,
                    type_v<After>,
                    type_v<bef_t>,
                    type_v<X>>{};
            }
        }
    }
};
LMNO_AUTO_CTAD_GUIDE(before);

/*
       d8888 888
      d88888 888
     d88P888 888
    d88P 888 888888 .d88b.  88888b.
   d88P  888 888   d88""88b 888 "88b
  d88P   888 888   888  888 888  888
 d8888888888 Y88b. Y88..88P 888 d88P
d88P     888  "Y888 "Y88P"  88888P"
                            888
                            888
                            888
*/

// The "∘" closure
template <typename F, typename G>
struct atop {
    NEO_NO_UNIQUE_ADDRESS F _f;
    NEO_NO_UNIQUE_ADDRESS G _g;

    LMNO_INDIRECT_INVOCABLE(atop);

    constexpr auto call(auto&& x) NEO_RETURNS(invoke(_f, invoke(_g, NEO_FWD(x))));
    constexpr auto call(auto&& x) const NEO_RETURNS(invoke(_f, invoke(_g, NEO_FWD(x))));

    constexpr auto call(auto&& w, auto&& x)
        NEO_RETURNS(invoke(_f, invoke(_g, NEO_FWD(w), NEO_FWD(x))));
    constexpr auto call(auto&& w, auto&& x) const
        NEO_RETURNS(invoke(_f, invoke(_g, NEO_FWD(w), NEO_FWD(x))));

    template <typename X>
    static auto error() {
        using render::type_v;
        using g_type = invoke_t<G, X>;
        if constexpr (any_error<g_type>) {
            return err::fmt_errorex_t<
                maybe_invoke_error_t<G, X>,
                "Right-hand function of type {:'} is not unary-invocable with ω of type {:'}",
                type_v<G>,
                type_v<X>>{};
        } else {
            if constexpr (not invocable<F, g_type>) {
                return err::fmt_errorex_t<  //
                    maybe_invoke_error_t<F, g_type>,
                    "Unary-invoking function {:'} with ω of type {:'} produces an intermediate\n"
                    "of type {:'}.\n\n"
                    "The left-hand function of type {:'} is not unary-invocable with ω of the\n"
                    "intermediate type {:'}",
                    type_v<G>,
                    type_v<X>,
                    type_v<g_type>,
                    type_v<F>,
                    type_v<g_type>>{};
            }
        }
    }

    template <typename W, typename X>
    static auto error() {
        using render::type_v;
        if constexpr (not invocable<G, W, X>) {
            return err::fmt_errorex_t<  //
                maybe_invoke_error_t<G, W, X>,
                "Right-hand function of type {:'} is not binary-invocable with\n"
                "arguments of type {:'} and {:'}.",
                type_v<G>,
                type_v<W>,
                type_v<X>>{};
        } else {
            using g_result = invoke_t<G, W, X>;
            if constexpr (not invocable<F, g_result>) {
                return err::fmt_errorex_t<  //
                    maybe_invoke_error_t<F, g_result>,
                    "Infix-invoking function {:'} with α of type {:'} and ω of \n"
                    "type {:'} produces an intermediate of type {:'}.\n\n"
                    "The left-hand function of type {:'} is not unary-invocable with ω of the\n"
                    "intermediate type {:'}",
                    type_v<G>,
                    type_v<W>,
                    type_v<X>,
                    type_v<g_result>,
                    type_v<F>,
                    type_v<g_result>>{};
            }
        }
    }
};
LMNO_AUTO_CTAD_GUIDE(atop);

/*
 .d88888b.
d88P" "Y88b
888     888
888     888 888  888  .d88b.  888d888
888     888 888  888 d8P  Y8b 888P"
888     888 Y88  88P 88888888 888
Y88b. .d88P  Y8bd8P  Y8b.     888
 "Y88888P"    Y88P    "Y8888  888
*/

// The "○" closure
template <typename F, typename G>
struct over {
    NEO_NO_UNIQUE_ADDRESS F _f;
    NEO_NO_UNIQUE_ADDRESS G _g;

    LMNO_INDIRECT_INVOCABLE(over);

    constexpr auto call(auto&& x) LMNO_INVOKES(_f, invoke(_g, NEO_FWD(x)));
    constexpr auto call(auto&& x) const LMNO_INVOKES(_f, invoke(_g, NEO_FWD(x)));

    constexpr auto call(auto&& w, auto&& x)
        LMNO_INVOKES(_f, invoke(_g, NEO_FWD(w)), invoke(_g, NEO_FWD(x)));
    constexpr auto call(auto&& w, auto&& x) const
        LMNO_INVOKES(_f, invoke(_g, NEO_FWD(w)), invoke(_g, NEO_FWD(x)));

    template <typename X>
    static auto error() {
        // Equivalent error from Atop
        return atop<F, G>::template error<X>();
    }
};
LMNO_AUTO_CTAD_GUIDE(over);

/*
8888888b.  888      d8b
888   Y88b 888      Y8P
888    888 888
888   d88P 88888b.  888
8888888P"  888 "88b 888
888        888  888 888
888        888  888 888
888        888  888 888
*/

// The "φ" closure, equivalent to an APL fork
template <typename F, typename H, typename G>
struct phi {
    NEO_NO_UNIQUE_ADDRESS F _f;
    NEO_NO_UNIQUE_ADDRESS H _h;
    NEO_NO_UNIQUE_ADDRESS G _g;

    LMNO_INDIRECT_INVOCABLE(phi);

    constexpr auto call(auto&& x) NEO_RETURNS(invoke(_h, invoke(_f, x), invoke(_g, x)));
    constexpr auto call(auto&& x) const NEO_RETURNS(invoke(_h, invoke(_f, x), invoke(_g, x)));

    constexpr auto call(auto&& w, auto&& x)
        NEO_RETURNS(invoke(_h, invoke(_f, w, x), invoke(_g, w, x)));
    constexpr auto call(auto&& w, auto&& x) const
        NEO_RETURNS(invoke(_h, invoke(_f, w, x), invoke(_g, w, x)));

    template <typename X>
    static auto error() {
        using render::type_v;
        if constexpr (not invocable<F, X>) {
            return err::fmt_errorex_t<
                maybe_invoke_error_t<F, X>,
                "Error while unary-invoking left-tine function {:'} with ω of type {:'}",
                type_v<F>,
                type_v<X>>{};
        } else if constexpr (not invocable<G, X>) {
            return err::fmt_errorex_t<
                maybe_invoke_error_t<G, X>,
                "Error while unary-invoking right-tine function {:'} with ω of type {:'}",
                type_v<G>,
                type_v<X>>{};
        } else {
            using f_result = invoke_t<F, X>;
            using g_result = invoke_t<G, X>;
            if constexpr (not invocable<H, f_result, g_result>) {
                return err::fmt_errorex_t<  //
                    maybe_invoke_error_t<H, f_result, g_result>,
                    "φ: Unary-invoking left-tine {:'} with ω {:'} produced an\n"
                    "   intermediate α' of type {:'},\n"
                    "\n"
                    "   Unary-invoking right-tine {:'} with ω {:'} produced an\n"
                    "   intermediate ω' of type {:'}.\n"
                    "\n"
                    "   Binary-invoking center-tine {:'} with\n"
                    "   α' of type {:'} and ω' of type {:'} fails.",
                    type_v<F>,
                    type_v<X>,
                    type_v<f_result>,
                    type_v<G>,
                    type_v<X>,
                    type_v<g_result>,
                    type_v<H>,
                    type_v<f_result>,
                    type_v<g_result>>{};
            }
        }
    }
};
LMNO_AUTO_CTAD_GUIDE(phi);

// The intermediate when creating a φ closure or "fork"
template <typename H>
struct phi_partial {
    NEO_NO_UNIQUE_ADDRESS H _h;

    constexpr auto operator()(auto&& f, auto&& g) NEO_RETURNS(
        phi{stdlib::autoconst_fn(NEO_FWD(f)), NEO_FWD(_h), stdlib::autoconst_fn(NEO_FWD(g))});
    constexpr auto operator()(auto&& f, auto&& g) const
        NEO_RETURNS(phi{stdlib::autoconst_fn(NEO_FWD(f)), _h, stdlib::autoconst_fn(NEO_FWD(g))});
};
LMNO_AUTO_CTAD_GUIDE(phi_partial);

/*
 .d8888b.           888  .d888      d88P  .d8888b.
d88P  Y88b          888 d88P"      d88P  d88P  Y88b
Y88b.               888 888       d88P   Y88b.
 "Y888b.    .d88b.  888 888888   d88P     "Y888b.   888  888  888  8888b.  88888b.
    "Y88b. d8P  Y8b 888 888     d88P         "Y88b. 888  888  888     "88b 888 "88b
      "888 88888888 888 888    d88P            "888 888  888  888 .d888888 888  888
Y88b  d88P Y8b.     888 888   d88P       Y88b  d88P Y88b 888 d88P 888  888 888 d88P
 "Y8888P"   "Y8888  888 888  d88P         "Y8888P"   "Y8888888P"  "Y888888 88888P"
                                                                           888
                                                                           888
                                                                           888
*/
template <typename F>
struct self_swap {
    NEO_NO_UNIQUE_ADDRESS F _f;

    LMNO_INDIRECT_INVOCABLE(self_swap);

    constexpr auto call(auto&& x) LMNO_INVOKES(_f, x, x);
    constexpr auto call(auto&& x) const LMNO_INVOKES(_f, x, x);
    constexpr auto call(auto&& w, auto&& x) LMNO_INVOKES(_f, NEO_FWD(x), NEO_FWD(w));
    constexpr auto call(auto&& w, auto&& x) const LMNO_INVOKES(_f, NEO_FWD(x), NEO_FWD(w));

    template <typename X>
    static auto error() {
        using render::type_v;
        if constexpr (not invocable<F, X, X>) {
            return err::fmt_errorex_t<  //
                maybe_invoke_error_t<F, X, X>,
                "Self-operator ˜ will binary-invoke function {:'} with left-hand\n"
                "and right-hand both of type {:'}.\n\n"
                "Function {:'} is not binary-invocable with α and ω both of\n"
                "type {:'}",
                type_v<F>,
                type_v<X>,
                type_v<X>>{};
        }
    }
};
LMNO_AUTO_CTAD_GUIDE(self_swap);

}  // namespace lmno::stdlib

namespace lmno {

/// ############################################################################
/// Name definitions

template <>
constexpr inline auto define<"˙"> = [](auto&& x) NEO_RETURNS_L(stdlib::const_fn{NEO_FWD(x)});

template <>
constexpr inline auto define<"⊢"> = stdlib::right_id{};

template <>
constexpr inline auto define<"⊣"> = stdlib::left_id{};

template <>
constexpr inline auto define<"⟜"> = [](auto&& f, auto&& g)
    NEO_RETURNS_L(stdlib::after{NEO_FWD(f), stdlib::autoconst_fn(NEO_FWD(g))});

template <>
constexpr inline auto define<"⊸"> = [](auto&& f, auto&& g)
    NEO_RETURNS_L(stdlib::before{stdlib::autoconst_fn(NEO_FWD(f)), NEO_FWD(g)});

template <>
constexpr inline auto define<"∘"> =
    [](auto&& f, auto&& g) NEO_RETURNS_L(stdlib::atop{NEO_FWD(f), NEO_FWD(g)});

template <>
constexpr inline auto define<"○"> =
    [](auto&& f, auto&& g) NEO_RETURNS_L(stdlib::over{NEO_FWD(f), NEO_FWD(g)});

template <>
constexpr inline auto define<"φ"> = [](auto&& f) NEO_RETURNS_L(stdlib::phi_partial{NEO_FWD(f)});

template <>
constexpr inline auto define<"˜"> = [](auto&& f) NEO_RETURNS_L(stdlib::self_swap{NEO_FWD(f)});

/// ############################################################################
/// Rendering

template <typename T>
constexpr auto render::type_v<stdlib::const_fn<T>> = cx_fmt_v<"˙{:'}", render::type_v<T>>;

template <typename F, typename G>
constexpr auto render::type_v<stdlib::after<F, G>>
    = cx_fmt_v<"({} ⟜ {})", render::type_v<F>, render::type_v<G>>;

template <typename F, typename G>
constexpr auto render::type_v<stdlib::before<F, G>>
    = cx_fmt_v<"({} ⊸ {})", render::type_v<F>, render::type_v<G>>;

template <typename F, typename G>
constexpr auto render::type_v<stdlib::atop<F, G>>
    = cx_fmt_v<"({} ∘ {})", render::type_v<F>, render::type_v<G>>;

template <typename F, typename G>
constexpr auto render::type_v<stdlib::over<F, G>>
    = cx_fmt_v<"({} ○ {})", render::type_v<F>, render::type_v<G>>;

template <typename F, typename H, typename G>
constexpr auto render::type_v<stdlib::phi<F, H, G>>
    = cx_fmt_v<"({} .φ:{} {})", render::type_v<F>, render::type_v<H>, render::type_v<G>>;

template <typename F>
constexpr auto render::type_v<stdlib::self_swap<F>> = cx_fmt_v<"˜:{}", render::type_v<F>>;

}  // namespace lmno
