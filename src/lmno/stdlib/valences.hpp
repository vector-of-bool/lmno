#pragma once

#include "../define.hpp"
#include "../func_wrap.hpp"
#include "../invoke.hpp"
#include "../meta.hpp"

#include <neo/attrib.hpp>
#include <neo/concepts.hpp>

namespace lmno::stdlib {

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

    template <typename X>
        requires requires { F::template error<X>(); }
    static auto error() {
        return F::template error<X>();
    }

    template <typename W, typename X>
        requires requires { G::template error<W, X>(); }
    static auto error() {
        return G::template error<W, X>();
    }
};
LMNO_AUTO_CTAD_GUIDE(polyfun);

constexpr inline auto _valences =
    [](auto&& f, auto&& g) NEO_RETURNS_L(polyfun{NEO_FWD(f), NEO_FWD(g)});

struct valence_modifier : func_wrap<_valences> {};

}  // namespace lmno::stdlib

namespace lmno {

template <>
constexpr auto define<"⊘"> = stdlib::valence_modifier{};

}  // namespace lmno