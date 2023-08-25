#include "./cursor.hpp"

#include <vector>

namespace md = lmno::md;

int main() {}

static_assert(md::cursor<md::range_cursor<std::views::all_t<std::vector<int>&>>>);
