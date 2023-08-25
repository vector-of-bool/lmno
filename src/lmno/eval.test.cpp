#include "./eval.hpp"

#include "./stdlib.hpp"

#include <catch2/catch.hpp>

using lmno::Const;
using lmno::ConstInt64;
using lmno::eval;
using lmno::eval_v;
using lmno::rational;

static_assert(eval<"4">() == 4);
static_assert(eval<"4">() == 4u);
static_assert(eval<"17">() == Const<17>{});
static_assert(eval<"174">() == Const<174>{});
static_assert(eval<"¯174">() == Const<-174>{});

static_assert(eval_v<"3"> == 3);
static_assert(eval_v<"3+4"> == 7);

static_assert(eval<"4+3">() == 7);
static_assert(eval<"- 4+3">() == -7);

static_assert(eval<"4+3">() == 7);
static_assert(eval<"4×3">() == 12);

static_assert(eval<"5 - 3">() == 2);

static_assert(eval<"-4">() == -4);

static_assert(eval<"4÷3">() == rational{4, 3});

static_assert(std::same_as<lmno::eval_t<"4+3">, ConstInt64<7>>);
static_assert(std::same_as<lmno::eval_t<"4÷3">, Const<rational{4, 3}>>);

// Hi-dot "const" operator
static_assert(eval<"˙5">()(4) == 5);
// Just-left
static_assert(eval<"⊣">()(4) == 4);
static_assert(eval<"⊣">()(4, 5) == 4);
// Just-right
static_assert(eval<"⊢">()(4) == 4);
static_assert(eval<"⊢">()(4, 8) == 8);
// minus-atop-minus
static_assert(eval<"-∘-">()(4) == 4);
// minus atop 2×
static_assert(eval<"(2⊸×)∘-">()(4) == -8);
static_assert(eval<"(×⟜2)∘-">()(4) == -8);
// φ combinator over ⌈
static_assert(eval<"-φ:⌈+">()(8, -12) == 20);
// Swap ˜
static_assert(eval<"-φ:⌈˜:-">()(8, 12) == 4);
static_assert(eval<"-φ:⌈˜:-">()(12, 8) == 4);
// Fold
static_assert(eval<"/:+∘⍳">()(6) == 15);
static_assert(eval<"/:+∘⍳">()(Const<6>{}) == 15);
static_assert(eval<"3⊸÷">()(4, 10) == rational{3, 10});

// Concept checks
static_assert(lmno::stateless<lmno::eval_t<"2">>);
static_assert(lmno::stateless<lmno::eval_t<"{ω}">>);
static_assert(lmno::stateless<lmno::eval_t<"{{ω}}">>);
static_assert(lmno::stateless<lmno::eval_t<"{{ω}2}">>);
static_assert(lmno::stateless<lmno::eval_t<"{{ω}2}0">>);

static_assert(lmno::stateless<lmno::default_sema>);
static_assert(lmno::stateless<lmno::ast::name<"ω">>);
static_assert(lmno::stateless<lmno::default_context<>>);
static_assert(
    lmno::stateless<lmno::default_context<lmno::scope<lmno::named_value<"α", lmno::ast::nothing>,
                                                      lmno::named_value<"ω", lmno::Const<0>>>>>);
static_assert(
    lmno::stateless<
        lmno::closure<lmno::ast::name<"ω">,
                      lmno::default_sema,
                      lmno::default_context<lmno::scope<lmno::named_value<"α", lmno::ast::nothing>,
                                                        lmno::named_value<"ω", lmno::Const<0>>>>>>);

static_assert(
    not lmno::stateless<
        lmno::closure<lmno::ast::name<"ω">,
                      lmno::default_sema,
                      lmno::default_context<lmno::scope<lmno::named_value<"α", std::string>,
                                                        lmno::named_value<"ω", lmno::Const<0>>>>>>);

// Simply requires that its argument be a typed-constant of value V
template <auto V, auto U>
    requires(V == U)
constexpr void is_constant(Const<U>) {}

// An operator func that is purposefully non-constexpr:
struct not_constexpr {
    auto operator()(int a) const NEO_RETURNS(a * 2);
};

TEST_CASE("Cases") {
    constexpr auto pow2 = eval<"2⊸^">();
    static_assert(pow2(8) == 256);

    constexpr auto drop = eval<"2↓·⍳5">();
    CHECK(drop.size() == 3);

    // Simple runtime closure:
    constexpr auto f2 = eval<"{2}">();
    static_assert(f2(1) == 2);

    // Closures must support both Const<> and runtime values:
    constexpr auto f3                    = eval<"{2+ω}">();
    ConstInt64<4>  four [[maybe_unused]] = f3(Const<2>{});
    static_assert(f3(4) == 6);

    // Miscellaneous:
    is_constant<6>(eval<"3+3">());
    is_constant<6>(eval<"{3+ω}">()(Const<3>{}));
    is_constant<6>(eval<"/+$⍳4">());
    is_constant<4>(eval<"{0+ω}">()(Const<4>{}));
    is_constant<4>(eval<"{ω}">()(Const<4>{}));
    is_constant<2>(eval<"{{ω}2}0">());
    is_constant<2>(eval<"{{ω}2}">()(Const<0>{}));
    is_constant<6>(eval<"/:+∘⍳">()(Const<4>{}));
    is_constant<6>(eval<"{/+$⍳ω}">()(Const<4>{}));
    is_constant<6>(eval<"{0⊸·/+$⍳ω}">()(Const<4>{}));
    is_constant<6>(eval<"{0⊸·/{α+ω}$⍳ω}">()(Const<4>{}));

    // Pass a non-constexpr, then call it:
    lmno::non_error auto v = eval<"⊢">()(not_constexpr{})(7);
    CHECK(v == 14);

    // Create a closure returning a function:
    constexpr auto f5   = eval<"{+⟜ω}">();
    constexpr auto add5 = f5(5);
    static_assert(add5(4) == 9);

    // Sequence expressions:
    static_assert(eval<"foo ← 5 ; foo + 2">() == 7);

    // Assignment as the only statement: The name goes nowhere, but doesn't matter
    static_assert(eval<"foo ← 8">() == 8);

    // Compute the Nth inverse of a power of two:
    constexpr lmno::non_error auto fn [[maybe_unused]] = eval<"1⊸·/+⟜÷∘2⊸^ $ ∘ $ 1φ:↓⍳">();
    constexpr lmno::non_error auto quo = fn(63);
    static_assert(quo == rational{9'223'372'036'854'775'807, 4'611'686'018'427'387'904});
    // Written using closures:
    constexpr auto f4 = eval<"{ 0 /:{α + 1÷2^ω} ·⍳ ω }">();
    is_constant<rational{9'223'372'036'854'775'807, 4'611'686'018'427'387'904}>(f4(Const<63>{}));

    // Imperative style:
    constexpr auto imper = eval<R"(
        inverseSquare ← {1÷2^ω};
        sumInverseSquares ← 0⊸/:{α + inverseSquare:ω};
        {sumInverseSquares ·⍳ ω}
    )">();

    static_assert(f4(63) == fn(63));
    static_assert(f4(63) == imper(63));

    // Even more imperative:
    constexpr auto imper2 = eval<R"({
        firstNNumbers ← ⍳ ω;
        inverseSquareOf ← {1÷2^ω};
        addInverseSquare ← {α + (inverseSquareOf ω)};
        sumInverseSquares ← 0⊸·/addInverseSquare;
        sumInverseSquares firstNNumbers
    })">();

    static_assert(
        std::same_as<decltype(imper2(Const<63>{})),
                     Const<rational{9'223'372'036'854'775'807, 4'611'686'018'427'387'904}>>);

    // Create some errors:
    lmno::any_error auto e [[maybe_unused]]  = eval<"/:+⟜⍳7">();
    lmno::any_error auto e1 [[maybe_unused]] = eval<"÷">()(std::string("yo"));
    lmno::any_error auto e2 [[maybe_unused]] = eval<"2⊸÷">()(std::string("yo"));

    // Mapping:
    constexpr lmno::non_error auto add_four = eval<"¨{4+ω}">();
    constexpr auto                 nums     = std::array{2, 1, 4, -3, 1, -44};
    lmno::non_error auto           added    = add_four(nums);
    CHECK(*added.as_range().begin() == 6);

    using namespace std::literals;

    // Attempt to add a number to a string is invalid:
    // A function that adds 2 to every element in a range
    auto add_two_to_each = eval<"¨2⊸+">();
    // An array of strings
    auto string_array = {"foo"s, "bar"s, "baz"s};
    // That's nonsense.
    lmno::any_error auto result [[maybe_unused]] = add_two_to_each(string_array);
    // Attempt to "use" `result` to see the error message:
    // result.foo;
}
