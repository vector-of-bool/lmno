#include "./stateless.hpp"

#include <functional>

struct empty {};

template <typename Elem>
struct aggregate {
    Elem p;
};

struct inherits : empty {
    empty e;
};

template <>
constexpr bool lmno::enable_stateless_v<inherits> = false;

static_assert(lmno::stateless<std::plus<>>);
static_assert(not lmno::stateless<int>);
static_assert(lmno::stateless<std::plus<int>>);
static_assert(not lmno::stateless<aggregate<int>>);
static_assert(not lmno::stateless<inherits>);
