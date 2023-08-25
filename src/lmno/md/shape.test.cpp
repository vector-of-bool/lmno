#include "./shape.hpp"

#include "./kokkos-mdspan.hpp"

int main() {}

static_assert(lmno::md::weak_shape<std::experimental::extents<int, 2>>);
static_assert(lmno::md::shape<std::experimental::extents<int, 2>>);
static_assert(lmno::md::fixed_shape<std::experimental::extents<int, 2>>);
static_assert(lmno::md::shape<std::experimental::extents<int, std::experimental::dynamic_extent>>);
static_assert(
    not lmno::md::fixed_shape<std::experimental::extents<int, std::experimental::dynamic_extent>>);
