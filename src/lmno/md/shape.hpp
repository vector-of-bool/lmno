#pragma once

#include "../concepts/stateless.hpp"
#include "./mdspan.hpp"

#include <neo/concepts.hpp>
#include <neo/declval.hpp>

namespace lmno::md {

/**
 * @brief The rank_type of the given shape type
 */
template <typename S>
using shape_rank_t = neo::remove_reference_t<S>::rank_type;

/**
 * @brief The size_type of the given shape type
 */
template <typename S>
using shape_size_t = neo::remove_reference_t<S>::size_type;

// clang-format off
/**
 * @brief Match a shape type that has a rank which can be determined at runtime
 */
template <typename S>
concept weak_shape =
        neo::regular<S>
    and requires(const S& s) {
            { s.rank() } noexcept;
            { s.rank_dynamic() } noexcept;
            typename shape_rank_t<S>;
            typename shape_size_t<S>;
            requires neo::integral<shape_rank_t<S>>;
            requires neo::integral<shape_size_t<S>>;
        }
    and requires(const S& s, shape_rank_t<S> const rank) {
            { s.extent(rank) } noexcept -> neo::convertible_to<shape_size_t<S>>;
        };

/**
 * @brief Match a shape type that has a compile-time fixed rank, and can be queried
 * for its static-length axes
 */
template <typename S>
concept shape =
        weak_shape<S>
    and requires(const shape_rank_t<S> rank) {
        // rank, static_extent, and rank_dynamic must all be static constexpr
        requires S::rank() >= 0;
        requires S::rank_dynamic() >= 0;
        { S::static_extent(rank) } -> neo::weak_same_as<std::size_t>;
    };

/**
 * @brief Check that shape `S` is has a static extent on the Nth axis
 */
template <typename S, shape_rank_t<S> N>
concept static_on_axis =
        shape<S>
    and neo::remove_reference_t<S>::static_extent(N) < ~(std::size_t(0));

/**
 * @brief Match a shape type that has no dynamic extents, and has a compile-time
 * fixed shape.
 */
template <typename S>
concept fixed_shape =
        shape<S>
    and requires {
        // There must be zero dynamic extents:
        requires S::rank_dynamic() == 0;
        requires stateless<neo::remove_cvref_t<S>>;
    };

template <typename S>
concept unit_shape = fixed_shape<S> and S::rank() == 0;
// clang-format on

namespace detail {

template <typename T>
concept non_array_range =  //
    std::ranges::forward_range<T> and (not neo::array_type<neo::remove_cvref_t<T>>);

template <typename T>
constexpr bool bounded_md_carray_v = false;

template <typename T, std::size_t N>
constexpr bool bounded_md_carray_v<T[N]> = (not neo::array_type<T>) or bounded_md_carray_v<T>;

// clang-format off
template <typename T>
concept has_shape =
        non_array_range<T>
     or bounded_md_carray_v<neo::remove_cvref_t<T>>
     or requires(T const& a) { { a.extents() } -> shape; }
     ;
// clang-format on

template <typename A, std::size_t R = std::rank_v<A>, std::size_t... Sizes>
constexpr auto carray_shape_v = nullptr;

template <typename A, std::size_t N, std::size_t Rank, std::size_t... Acc>
constexpr auto carray_shape_v<A[N], Rank, Acc...> = carray_shape_v<A, Rank - 1, Acc..., N>;

template <typename A, std::size_t... Acc>
constexpr auto carray_shape_v<A, 0, Acc...> = md::uz_extents<Acc...>{};

}  // namespace detail

/**
 * @brief Obtain the shape of an mdrange
 */
inline constexpr struct shapeof_fn {
    template <detail::has_shape A>
    [[nodiscard]] constexpr decltype(auto) operator()(A&& arr) const noexcept {
        if constexpr (detail::non_array_range<A>) {
            return md::uz_dextents<1>{std::ranges::distance(arr)};
        } else if constexpr (detail::bounded_md_carray_v<neo::remove_cvref_t<A>>) {
            // It's a bounded multidim C-array
            return detail::carray_shape_v<neo::remove_cvref_t<A>>;
        } else {
            return arr.extents();
        }
    }
} shapeof;

template <typename A>
using shape_t = neo::remove_cvref_t<decltype(shapeof(NEO_DECLVAL(A)))>;

template <typename Arr>
using rank_t = shape_rank_t<shape_t<Arr>>;

}  // namespace lmno::md
