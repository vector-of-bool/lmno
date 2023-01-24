#include "./meta.hpp"

int main() {}

using r = lmno::meta::reverse<lmno::meta::list<int, double, char, double>>;
static_assert(std::same_as<r, lmno::meta::list<double, char, double, int>>);
