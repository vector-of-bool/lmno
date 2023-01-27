#pragma once

#include "../define.hpp"
#include "../func_wrap.hpp"
#include "../invoke.hpp"
#include "../render.hpp"

#include <neo/concepts.hpp>

namespace lmno::stdlib {

constexpr inline auto _and
    = [](neo::integral auto w, neo::integral auto x) -> int { return w and x; };

constexpr inline auto _or
    = [](neo::integral auto w, neo::integral auto x) -> int { return w or x; };

constexpr inline auto _not = [](neo::integral auto x) -> int { return not x; };

struct and_ : func_wrap<_and> {
    LMNO_INDIRECT_INVOCABLE(and_);
};
struct or_ : func_wrap<_or> {
    LMNO_INDIRECT_INVOCABLE(or_);
};
struct not_ : func_wrap<_not> {
    LMNO_INDIRECT_INVOCABLE(not_);
};

}  // namespace lmno::stdlib

namespace lmno {

template <>
constexpr inline auto define<"∧"> = stdlib::and_{};

template <>
constexpr inline auto render::type_v<stdlib::and_> = cx_fmt_v<"∧ (logical-and)">;

template <>
constexpr inline auto define<"∨"> = stdlib::or_{};

template <>
constexpr inline auto render::type_v<stdlib::or_> = cx_fmt_v<"∨ (logical-or)">;

template <>
constexpr inline auto define<"¬"> = stdlib::not_{};

template <>
constexpr inline auto render::type_v<stdlib::not_> = cx_fmt_v<"¬ (logical-not)">;

}  // namespace lmno
