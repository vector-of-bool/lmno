#pragma once

#include "../const.hpp"
#include "../define.hpp"
#include "../rational.hpp"

namespace lmno::stdlib {

struct infinity {};

}  // namespace lmno::stdlib

namespace lmno {

// ∞ is it's own thing...
template <>
constexpr inline auto define<"∞"> = stdlib::infinity{};

// π is rational 'round these parts :)
template <>
constexpr inline auto define<"π"> = Const<rational{104348 / 33215}>{};

}  // namespace lmno
