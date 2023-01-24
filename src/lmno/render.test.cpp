#include "./render.hpp"

#include <vector>

namespace test_ns {

struct nosupp {};

template <typename T>
struct my_type {};

}  // namespace test_ns

namespace {

template <lmno::cx_str V>
struct C {};

namespace R = lmno::render;

static_assert(R::type_v<unsigned int> == "unsigned int");

template <typename T>
constexpr bool are_same(T, T) {
    return true;
}

static_assert(are_same(C<R::template_of_v<test_ns::my_type<int>>>{}, C<"test_ns::my_type">{}));

// Default/debug info are erased
C<"std::vector<int>"> _1 [[maybe_unused]] = C<R::type_v<std::vector<int>>>{};
C<"std::string">      _2 [[maybe_unused]] = C<R::type_v<std::string>>{};

C<"42">   _3 [[maybe_unused]] = C<R::value_v<42>>{};
C<"42ul"> _4 [[maybe_unused]] = C<R::value_v<42ul>>{};

constexpr auto a1 [[maybe_unused]] = R::value_v<test_ns::nosupp{}>;
static_assert(are_same(C<a1>{}, C<"[Unsupported value-render for type ‘test_ns::nosupp’]">{}));

C<"(Constant ‘unsigned long’: 42ul)"> _5 [[maybe_unused]] = C<R::type_v<lmno::Const<42ul>>>{};

C<"(Constant ‘int’: 42)"> _6 [[maybe_unused]] = C<R::type_v<lmno::Const<42>>>{};

C<"(80÷3):ℚ"> _7 [[maybe_unused]] = C<R::value_v<lmno::rational{400, 15}>>{};

C<"¯42"> _8 [[maybe_unused]] = C<R::value_v<(-42)>>{};

}  // namespace
