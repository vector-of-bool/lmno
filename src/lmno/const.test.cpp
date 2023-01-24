#include "./const.hpp"
#include "./rational.hpp"

using namespace lmno;

static_assert(typed_constant<Const<12>>);
static_assert(structural<int>);
static_assert(typed_constant<Const<rational{4, 3}>>);
static_assert(typed_constant<Const<5>>);

static_assert(std::equality_comparable<Const<22>>);
static_assert(std::totally_ordered<Const<5>>);

static_assert(Const<12>{} == Const<12>{});
static_assert(Const<42>{} != Const<12>{});
static_assert(Const<42>{} != Const<12u>{});
static_assert(Const<42>{} > Const<12>{});

static_assert(structural<rational>);
