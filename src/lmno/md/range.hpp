#pragma once

#include <neo/concepts.hpp>
#include <neo/declval.hpp>
#include <neo/scalar_box.hpp>
#include <neo/type_traits.hpp>

#include "./cursor.hpp"
#include "./shape.hpp"

#include <ranges>

namespace lmno::md {

/**
 * @brief Obtain a cursor pointing to the origin of a multidimensional range.
 */
inline constexpr struct origin_fn {
    template <detail::has_shape A>
    [[nodiscard]] constexpr auto operator()(A& a) const noexcept
        requires detail::non_array_range<A>
        or detail::bounded_md_carray_v<neo::remove_reference_t<A>>  //
        or requires {
               { a.origin() } -> cursor;
           }
    {
        if constexpr (detail::non_array_range<A>) {
            auto view = std::views::all(a);
            return range_cursor(view);
        } else if constexpr (detail::bounded_md_carray_v<neo::remove_reference_t<A>>) {
            return c_array_cursor(a);
        } else {
            return a.origin();
        }
    }
} origin;

/**
 * @brief Obtain the cursor type for the given mdrange
 */
template <typename A>
using cursor_t = decltype(origin(NEO_DECLVAL(A&)));

/**
 * @brief Obtain the rank of the given mdrange
 */
template <typename A>
constexpr std::size_t rank_v = shape_t<A>::rank();

/**
 * @brief Obtain the offset type used for the given mdrange's cursor
 */
template <typename T>
using offset_t = cursor_offset_t<cursor_t<T>>;

/**
 * @brief Get the reference-type returned by the given mdrange's cursor
 */
template <typename T>
using reference_t = cursor_reference_t<cursor_t<T>>;

template <typename A>
concept mdrange =  //
    requires(A& a) {
        md::origin(a);
        md::shapeof(a);
    };

/**
 * @brief Match an mdrange that has a compiled-time fixed shape
 */
template <typename A>
concept fixed_shape_mdrange = mdrange<A> and fixed_shape<shape_t<A>>;

/**
 * @brief Compute the bounds of the given mdrange, i.e. the total number of 0-cells in the array
 */
inline constexpr struct bounds_fn {
    [[nodiscard]] constexpr auto operator()(shape auto const& e) const noexcept {
        std::size_t ret = 1;
        auto        r   = static_cast<std::size_t>(e.rank());
        for (auto n : std::views::iota(r - r, r)) {
            auto x = static_cast<std::size_t>(e.extent(n));
            ret    = ret * x;
        }
        return ret;
    }
    [[nodiscard]] constexpr auto operator()(mdrange auto const& r) const noexcept {
        return (*this)(md::shapeof(r));
    }
} bounds;

/**
 * @brief Obtain the rank of the given mdrange, i.e. the number of dimensions in the array.
 */
inline constexpr struct rank_fn {
    [[nodiscard]] constexpr auto operator()(shape auto const& e) const noexcept { return e.rank(); }
    [[nodiscard]] constexpr auto operator()(mdrange auto const& r) const noexcept {
        return md::shapeof(r).rank();
    }
} rank;

template <std::ranges::forward_range R>
class range_md_accessor {
public:
    using offset_policy = range_md_accessor;
    using element_type  = std::ranges::range_value_t<R>;
    using reference     = std::ranges::range_reference_t<R>;
    using pointer       = std::ranges::iterator_t<R>;

    // nonstd: Used by Kokkos instead of 'pointer'
    using data_handle_type = pointer;

public:
    range_md_accessor() = default;

    constexpr pointer offset(pointer from, std::ptrdiff_t off) const noexcept {
        pointer to = std::ranges::next(from, off);
        return to;
    }

    constexpr reference access(pointer p, std::size_t nth) const noexcept {
        pointer   adjust = this->offset(p, static_cast<std::ptrdiff_t>(nth));
        reference v      = *adjust;
        return v;
    }
};

template <std::ranges::forward_range R, shape E>
    requires std::ranges::viewable_range<R>
constexpr auto mdspan_for_range(R&& r, E shp) noexcept {
    auto iter    = std::ranges::begin(r);
    auto mapping = std::experimental::layout_right::mapping(shp);
    auto access  = range_md_accessor<R>();
    return mdspan(iter, mapping, access);
}

template <std::ranges::forward_range R>
    requires std::ranges::viewable_range<R>
constexpr auto flat_mdspan_for_range(R&& r) noexcept {
    using shape_type = uz_dextents<1>;
    shape_type shp(std::ranges::distance(r));
    return mdspan_for_range(NEO_FWD(r), shp);
}

inline constexpr struct view_extents_fn {
    constexpr auto operator()(md::shape auto shp) const noexcept {
        auto is = std::views::iota(0, shp.rank());
        return std::views::transform(is, [shp](auto ax) { return shp.extent(ax); });
    }
} view_extents;

namespace detail {

namespace _sr = std::ranges;
namespace _sv = std::views;

constexpr void reshape_move_items(bool  reverse,
                                  auto& vec,
                                  auto  map_from,
                                  auto  map_to,
                                  std::same_as<std::size_t> auto... idx) {
    if constexpr (sizeof...(idx) == decltype(map_to)::extents_type::rank()) {
        // We're acting on the 0-cells
        // Get the previous index of the item and its new destination index
        const std::size_t from_idx = map_from(idx...);
        const std::size_t to_idx   = map_to(idx...);
        // The iterators for those positions:
        const auto it_from = _sr::next(_sr::begin(vec), from_idx);
        const auto it_to   = _sr::next(_sr::begin(vec), to_idx);
        // Do the move:
        auto&  dest  = *it_to;
        auto&& value = _sr::iter_move(it_from);
        dest         = static_cast<decltype(value)&&>(value);
    } else {
        // Iterate over the K-cells, recusing into the inner cells
        const std::size_t tox   = map_to.extents().extent(sizeof...(idx));
        const std::size_t fromx = map_from.extents().extent(sizeof...(idx));
        // Don't iterate over K-cells that weren't in the prior shape, nor
        // K-cells that won't be present in the new shape:
        const std::size_t len = (std::min)(tox, fromx);
        for (std::size_t nth : _sv::iota(0u, len)) {
            if (reverse) {
                // Caller wants us to reverse the iteration order
                nth = len - nth - 1;
            }
            reshape_move_items(reverse, vec, map_from, map_to, idx..., nth);
        }
    }
}

}  // namespace detail

namespace _sr = std::ranges;

template <typename C, shape From, shape To>
    requires _sr::forward_range<C>                       //
    and _sr::output_range<C, _sr::range_reference_t<C>>  //
    and requires(C& c) { c.resize(42); }
constexpr void reshape(C& vec, From from_shape, To to_shape) noexcept {
    const std::size_t old_bounds = md::bounds(from_shape);
    const std::size_t new_bounds = md::bounds(to_shape);
    if (old_bounds > new_bounds) {
        // We need to move elements around before resizing, since items will be dropped
        detail::reshape_move_items(false,
                                   vec,
                                   layout_right::mapping(from_shape),
                                   layout_right::mapping(to_shape));
        vec.resize(new_bounds);
    } else {
        // We added more room, so now we shift *after* resizing
        vec.resize(new_bounds);
        detail::reshape_move_items(true,
                                   vec,
                                   layout_right::mapping(from_shape),
                                   layout_right::mapping(to_shape));
    }
}

}  // namespace lmno::md
