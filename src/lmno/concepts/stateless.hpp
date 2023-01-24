#pragma once

#include <boost/pfr/ops_fields.hpp>
#include <neo/concepts.hpp>

#include <tuple>

namespace lmno {

/**
 * @brief Specialize for a type to force LMNO to consider it "stateless"
 */
template <typename T>
constexpr int enable_stateless_v = 0;

namespace detail {

template <typename T,
          std::size_t Size = boost::pfr::tuple_size_v<T>,
          typename Seq     = std::make_index_sequence<Size>>
constexpr bool all_elems_stateless_v = false;

}  // namespace detail

/**
 * @brief Match a type that we can prove has no state.
 *
 * @tparam T
 */
template <typename T>
concept stateless =                   //
    enable_stateless_v<T> == true or  //
    requires {
        requires std::same_as<decltype(enable_stateless_v<T>), const int>;
        requires enable_stateless_v<T> == 0;
        // We can't use "is_literal", but being trivially destructible
        // is a reasonable substitute in most cases.
        requires neo::trivially_destructible<T>;
        requires neo::default_initializable<T>;
        // Check for constexpr-constructor-ness
        requires(static_cast<void>(T{}), true);
        // Finally, it must be either empty:
        requires std::is_empty_v<T>  //
            or requires {
                   // Or all of its public elements must be empty:
                   requires std::is_aggregate_v<T>;
                   requires detail::all_elems_stateless_v<T>;
               };
    };

template <typename T, std::size_t Size, auto... Is>
constexpr bool detail::all_elems_stateless_v<T, Size, std::index_sequence<Is...>> =  //
    requires {
        requires(stateless<neo::remove_cvref_t<boost::pfr::tuple_element_t<Is, T>>> and ...);
    };

// Enable for tuples that are themselves stateless
template <typename... Ts>
constexpr bool enable_stateless_v<std::tuple<Ts...>> = (stateless<Ts> and ...);

}  // namespace lmno
