#include "./strand.hpp"

#include <ranges>

static_assert(std::ranges::random_access_range<lmno::strand_range<int, int, int, int>>);

