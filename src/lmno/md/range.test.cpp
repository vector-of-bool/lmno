#include "./range.hpp"

#include <vector>

#include <catch2/catch.hpp>

namespace md = lmno::md;

static_assert(md::mdrange<std::vector<int>>);
static_assert(md::mdrange<int[5]>);
static_assert(md::fixed_shape_mdrange<int[5]>);
static_assert(md::mdrange<int[5][7]>);
static_assert(md::mdrange<int (&)[5][7]>);
static_assert(not md::mdrange<int[][7]>);

static_assert(md::rank_v<int[4]> == 1);
static_assert(md::rank_v<int[5][4]> == 2);
static_assert(md::rank_v<std::array<int, 4>> == 1);

TEST_CASE("Scan a range") {
    std::vector<int> vec = {1, 2, 3};
    auto             cur = md::augmented_cursor{md::origin(vec)};
    CHECK(*cur == 1);
    cur = cur + md::basic_offset{1};
    CHECK(*cur == 2);
}

TEST_CASE("Scan a multidimentional array") {
    int             arr[2][3] = {{1, 2, 3}, {4, 5, 6}};
    md::cursor auto orig      = md::origin(arr);
    auto            cur       = md::augmented_cursor{orig};
    auto            six       = cur[{1, 2}];
    CHECK(*cur == 1);
    CHECK(six == 6);
    CHECK(md::bounds(arr) == 6);
    CHECK(md::rank(arr) == 2);
}
