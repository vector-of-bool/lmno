#include "./parse.hpp"

namespace {

using lmno::Const;
using lmno::ConstInt64;
using lmno::parse_t;
using lmno::ast::dyad;
using lmno::ast::monad;
using lmno::ast::name;
using lmno::ast::strand;

using a = lmno::parse_t<"hello">;
static_assert(std::same_as<a, name<"hello">>);

using b = lmno::parse_t<"hello gov">;
static_assert(std::same_as<b, monad<name<"hello">, name<"gov">>>);

using c = lmno::parse_t<"hello new world">;
static_assert(std::same_as<c, dyad<name<"hello">, name<"new">, name<"world">>>);

using d = lmno::parse_t<"yo hello new world">;
static_assert(std::same_as<d, monad<name<"yo">, dyad<name<"hello">, name<"new">, name<"world">>>>);

using e = lmno::parse_t<"foo : bar baz">;
static_assert(std::same_as<e, monad<monad<name<"foo">, name<"bar">>, name<"baz">>>);

static_assert(std::same_as<parse_t<"foo .bar baz">, dyad<name<"foo">, name<"bar">, name<"baz">>>);
static_assert(std::same_as<parse_t<"foo bar .baz quux">,
                           dyad<monad<name<"foo">, name<"bar">>, name<"baz">, name<"quux">>>);
static_assert(
    std::same_as<
        parse_t<"foo bar .baz:quux foo">,
        dyad<monad<name<"foo">, name<"bar">>, monad<name<"baz">, name<"quux">>, name<"foo">>>);

static_assert(
    std::same_as<parse_t<"foo bar .. baz quux">,
                 monad<monad<name<"foo">, name<"bar">>, monad<name<"baz">, name<"quux">>>>);

// using big1 [[maybe_unused]] = lmno::parse_t<
//     "·÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞··÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞··÷√π∞·÷√π∞·÷√π∞·÷√π∞··÷√π∞·÷√π∞·÷√π∞·÷√π∞·"
//     "÷√π∞··÷√π∞·÷√π∞·÷√π∞·÷√π∞··÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞··÷√π∞·÷√π∞·÷√π∞·÷√π∞··÷√π∞·÷√π∞·÷√π∞·÷√π∞·"
//     "÷√π∞··÷√π∞·÷√π∞·÷√π∞·÷√π∞··÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞··÷√π∞·÷√π∞·÷√π∞·÷√π∞··÷√π∞·÷√π∞·÷√π∞·÷√π∞·"
//     "÷√π∞··÷√π∞·÷√π∞·÷√π∞·÷√π∞··÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞··÷√π∞·÷√π∞·÷√π∞·÷√π∞··÷√π∞·÷√π∞·÷√π∞·÷√π∞·"
//     "÷√π∞··÷√π∞·÷√π∞·÷√π∞·">;

using f = lmno::parse_t<"1‿2‿3‿4">;
static_assert(std::same_as<f, strand<ConstInt64<1>, ConstInt64<2>, ConstInt64<3>, ConstInt64<4>>>);

}  // namespace

int main() {}
