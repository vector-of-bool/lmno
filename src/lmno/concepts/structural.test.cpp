#include "./structural.hpp"

struct empty {};

struct simple {
    int a;
    int b;

    constexpr simple();
};

struct non_structural {
private:
    int e;

public:
    constexpr non_structural();
    int a;
};

static_assert(lmno::structural<empty>);
static_assert(lmno::structural<simple>);
static_assert(not lmno::structural<non_structural>);
