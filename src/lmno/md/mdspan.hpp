#pragma once

#include "./kokkos-mdspan.hpp"

namespace std::experimental {

// Kokkos does not include a deduction guide, but we want that
template <std::integral... Is>
explicit extents(Is...) -> extents<std::size_t, (Is(0), dynamic_extent)...>;

}  // namespace std::experimental

namespace lmno::md {

using std::experimental::dextents;
using std::experimental::dynamic_extent;
using std::experimental::extents;
using std::experimental::full_extent;
using std::experimental::full_extent_t;
using std::experimental::layout_left;
using std::experimental::layout_right;
using std::experimental::layout_stride;
using std::experimental::mdspan;
using std::experimental::submdspan;

template <std::size_t... Ns>
using uz_extents = extents<std::size_t, Ns...>;

template <std::size_t N>
using uz_dextents = dextents<std::size_t, N>;

}  // namespace lmno::md

namespace lmno {

using lmno::md::mdspan;
using lmno::md::submdspan;

}  // namespace lmno
