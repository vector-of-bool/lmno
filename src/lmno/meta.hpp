#pragma once

#include <neo/meta.hpp>
#include <neo/type_traits.hpp>

namespace lmno::meta {

/**
 * @brief A list of values
 *
 * @tparam Vs
 */
template <auto... Vs>
struct vlist;

using neo::meta::append;
using neo::meta::head;
using neo::meta::list;
using neo::meta::rebind;
using neo::meta::second;
using neo::meta::tail;

template <template <class...> class L, typename... Ts, typename V>
auto push_front_fn(L<Ts...> const&, V const&) -> L<V, Ts...>;

template <typename L, typename V>
using push_front = decltype(push_front_fn(NEO_DECLVAL(L const&), NEO_DECLVAL(V const&)));

template <typename T, typename...>
using just_t = T;

namespace detail {

template <typename T, std::size_t N>
struct type_reverser_base {};

template <typename L, typename Seq = std::make_index_sequence<neo::meta::len_v<L>>>
struct reverser_derived;

template <template <class...> class L, typename... Ts, std::size_t... Ns>
struct reverser_derived<L<Ts...>, std::index_sequence<Ns...>> : type_reverser_base<Ts, Ns>... {};

template <std::size_t N>
struct reverser_ns {
    template <typename T>
    static auto apply(type_reverser_base<T, N>*...) -> T;
};

}  // namespace detail

template <typename L,
          std::size_t Len = neo::meta::len_v<L>,
          typename Seq    = std::make_index_sequence<Len>,
          typename D      = detail::reverser_derived<L, Seq>>
struct reverser;

template <template <class...> class L,
          typename... Ts,
          std::size_t Len,
          std::size_t... Ns,
          typename D>
struct reverser<L<Ts...>, Len, std::index_sequence<Ns...>, D> {
    using type = L<decltype(detail::reverser_ns<sizeof...(Ns) - Ns - 1>  //
                            ::apply(static_cast<D*>(nullptr)))...>;
};

/**
 * @brief Reverse the elements of a type-list
 */
template <typename L>
using reverse = reverser<L>::type;

// Declare a simple CTAD guide that performs aggregate-style CTAD.
// (Clang does not yet implement that C++20 feature)
#define LMNO_AUTO_CTAD_GUIDE(Type)                                                                 \
    template <typename... Args>                                                                    \
    explicit Type(const Args&...)->Type<Args...>

template <typename Func, typename... Ts>
using apply_f = typename Func::template f<Ts...>;

// Declare a type as a pointer
template <typename T>
using ptr = T*;

}  // namespace lmno::meta
