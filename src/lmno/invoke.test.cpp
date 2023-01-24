#include "./invoke.hpp"
#include "./rational.hpp"

#include <functional>

using namespace lmno;

using std::same_as;

template <auto V, auto W>
    requires(V == W)
void check_const(Const<W>) {}

struct with_error {
    template <typename... Args>
    static constexpr auto error() {
        return cx_str{"I am an error message"};
    }
};

struct widget {};

static_assert(any_error<lmno::invoke_t<with_error, int, int>>);

static_assert(std::same_as<lmno::unconst_t<std::plus<>>, std::plus<>>);
static_assert(neo::invocable2<unconst_t<std::plus<>>, unconst_t<int>, unconst_t<int>>);
static_assert(std::same_as<lmno::invoke_t<std::plus<>, int, int>, int>);
static_assert(std::same_as<lmno::invoke_t<std::plus<>, Const<12>, Const<4>>, Const<16>>);

static_assert(std::same_as<lmno::invoke_t<std::plus<>, Const<4>, Const<3>>, Const<7>>);
static_assert(std::same_as<lmno::invoke_t<std::divides<>, Const<rational{4}>&&, Const<3>>,
                           Const<rational{4, 3}>>);

template <typename T>
struct add {
    T rhs;

    constexpr auto operator()(auto val) const
        requires requires { val + rhs; }
    {
        return rhs + val;
    }
};
LMNO_AUTO_CTAD_GUIDE(add);

struct invocable_without_const {
    auto operator()(lmno::variate auto arg) const { return arg; }
};

struct invocable_without_const_nonempty {
    int  v;
    auto operator()(lmno::variate auto arg) const { return arg; }
};

struct invocable_with_const {
    auto operator()(auto arg) const { return arg; }
};

struct invocable_with_const_nonempty {
    int  v;
    auto operator()(auto arg) const { return arg; }
};

struct constexpr_invocable_without_const {
    constexpr auto operator()(lmno::variate auto arg) const { return arg; }
};

struct constexpr_invocable_without_const_nonempty {
    int            v;
    constexpr auto operator()(lmno::variate auto arg) const { return arg; }
};

struct constexpr_invocable_with_const {
    constexpr auto operator()(auto arg) const { return arg; }
};

struct constexpr_invocable_with_const_nonempty {
    int            v;
    constexpr auto operator()(auto arg) const { return arg; }
};

struct never_invocable {
    constexpr auto operator()(std::same_as<void> auto) const {}
};

struct generates_error_chain {
    constexpr auto operator()(auto i) const noexcept { return lmno::invoke(never_invocable{}, i); }
};

struct explains_error_not_invocable {
    constexpr auto operator()(std::same_as<void> auto) const {}

    template <typename X>
    using error_t = lmno::err::error_type<"I am an error">;
};

template <auto V, auto W>
    requires(V == W)
void check_returns_const(Const<W>) {}

template <auto V>
void check_returns_variate(variate auto val) {
    assert(val == V);
}

template <typename T, std::same_as<T> U>
void check_returns(U) {}

int main() {
    using lmno::invocable;
    using lmno::invoke_t;
    // static_assert(lmno::invoke(std::plus{}, 2, 3) == 5);
    std::plus     lval{};
    constexpr int two   = 2;
    constexpr int three = 3;
    assert(lmno::invoke(lval, 2, 3) == 5);
    assert(lmno::invoke(lval, two, 3) == 5);
    assert(lmno::invoke(lval, two, three) == 5);
    assert(lmno::invoke(lval, 2, three) == 5);
    assert(lmno::invoke(std::as_const(lval), 2, 3) == 5);
    static_assert(invocable<std::plus<>, int, int>);
    static_assert(invocable<std::plus<>, int, int&>);
    static_assert(invocable<std::plus<>, int, int&&>);
    static_assert(invocable<std::plus<>, int&, int>);
    static_assert(invocable<std::plus<>, int&&, int>);
    static_assert(invocable<std::plus<>, int&&, int&>);
    static_assert(invocable<std::plus<>, int&&, int&&>);
    static_assert(invocable<std::plus<>, int const&, int>);
    static_assert(invocable<std::plus<>, int const&&, int>);
    static_assert(invocable<std::plus<>, int, int const&>);
    static_assert(invocable<std::plus<>, int, int const&&>);
    static_assert(invocable<std::plus<>, int const&, int const&&>);
    static_assert(invocable<std::plus<>, int const&&, int const&>);
    static_assert(invocable<std::plus<>, int const&, int const&>);
    static_assert(invocable<std::plus<>, int const&&, int const&&>);

    // clang-format off

    check_returns<int>(invoke(invocable_without_const_nonempty{}, 2));
    check_returns<int>(invoke(invocable_without_const{}, 2));
    check_returns<int>(invoke(invocable_without_const_nonempty{}, Const<2>{}));
    check_returns<int>(invoke(invocable_without_const{}, Const<2>{}));
    check_returns<int>(invoke(invocable_with_const_nonempty{}, 2));
    check_returns<int>(invoke(invocable_with_const{}, 2));
    check_returns<Const<2>>(invoke(invocable_with_const_nonempty{}, Const<2>{}));
    check_returns<Const<2>>(invoke(invocable_with_const{}, Const<2>{}));

    // -

    check_returns<int>(invoke(constexpr_invocable_without_const_nonempty{}, 2));
    check_returns<int>(invoke(constexpr_invocable_without_const{}, 2));
    check_returns<int>(invoke(constexpr_invocable_without_const_nonempty{}, Const<2>{}));
    check_returns<Const<2>>(invoke(constexpr_invocable_without_const{}, Const<2>{}));
    check_returns<int>(invoke(constexpr_invocable_with_const_nonempty{}, 2));
    check_returns<int>(invoke(constexpr_invocable_with_const{}, 2));
    check_returns<Const<2>>(invoke(constexpr_invocable_with_const_nonempty{}, Const<2>{}));
    check_returns<Const<2>>(invoke(constexpr_invocable_with_const{}, Const<2>{}));

    // ========

    check_returns<int>(invoke(Const<invocable_without_const_nonempty{}>{}, 2));
    check_returns<int>(invoke(Const<invocable_without_const{}>{}, 2));
    check_returns<int>(invoke(Const<invocable_without_const_nonempty{}>{}, Const<2>{}));
    check_returns<int>(invoke(Const<invocable_without_const{}>{}, Const<2>{}));
    check_returns<int>(invoke(Const<invocable_with_const_nonempty{}>{}, 2));
    check_returns<int>(invoke(Const<invocable_with_const{}>{}, 2));
    check_returns<Const<2>>(invoke(Const<invocable_with_const_nonempty{}>{}, Const<2>{}));
    check_returns<Const<2>>(invoke(Const<invocable_with_const{}>{}, Const<2>{}));

    // -

    check_returns<int>(invoke(Const<constexpr_invocable_without_const_nonempty{}>{}, 2));
    check_returns<int>(invoke(Const<constexpr_invocable_without_const{}>{}, 2));
    check_returns<Const<2>>(invoke(Const<constexpr_invocable_without_const_nonempty{}>{}, Const<2>{}));
    check_returns<Const<2>>(invoke(Const<constexpr_invocable_without_const{}>{}, Const<2>{}));
    check_returns<int>(invoke(Const<constexpr_invocable_with_const_nonempty{}>{}, 2));
    check_returns<int>(invoke(Const<constexpr_invocable_with_const{}>{}, 2));
    check_returns<Const<2>>(invoke(Const<constexpr_invocable_with_const_nonempty{}>{}, Const<2>{}));
    check_returns<Const<2>>(invoke(Const<constexpr_invocable_with_const{}>{}, Const<2>{}));

    // ========

    static_assert(lmno::any_error<invoke_t<never_invocable, int>>);
    static_assert(lmno::any_error<invoke_t<generates_error_chain, int>>);
    static_assert(lmno::any_error<invoke_t<explains_error_not_invocable, int>>);
    // lmno::invoke(generates_error_chain{}, 12);

    // clang-format on

    // static_assert(2 == invoke(invocable_with_const{}, Const<2>{}));

    static_assert(invocable<std::plus<>, Const<2>, int>);
    static_assert(invocable<std::plus<>, Const<2>&, int>);
    static_assert(same_as<invoke_t<std::plus<>, int, int>, int>);
    static_assert(same_as<invoke_t<std::plus<>, Const<2>, int>, int>);
    static_assert(same_as<invoke_t<std::plus<>, Const<2>&, int>, int>);
    static_assert(same_as<invoke_t<std::plus<>, Const<2>, Const<3>>, Const<5>>);
    static_assert(same_as<invoke_t<std::plus<>, Const<2>&, Const<3>>, Const<5>>);
    static_assert(same_as<invoke_t<Const<std::plus<>{}>, Const<2>, Const<3>>, Const<5>>);
    static_assert(same_as<invoke_t<Const<std::plus<>{}>, Const<2>&, Const<3>>, Const<5>>);

    // // Not invocable, but won't generate a compilation error:
    any_error auto err [[maybe_unused]] = lmno::invoke(std::plus<>{}, 2, std::string("hi"));
    // Okay:
    non_error auto five [[maybe_unused]] = lmno::invoke(std::plus<>{}, 2, 3);

    std::same_as<int> auto twelve [[maybe_unused]] = lmno::invoke(std::plus<>{}, Const<7>{}, 5);
    // std::same_as<Const<12>> auto twelve2 [[maybe_unused]]
    // = lmno::invoke(std::plus<>{}, Const<7>{}, Const<5>{});

    any_error auto e1 [[maybe_unused]] = lmno::invoke(std::plus<>{}, 2, Const<widget{}>{});
    any_error auto e2 [[maybe_unused]] = lmno::invoke(std::plus<>{}, Const<2>{}, Const<widget{}>{});

    std::same_as<Const<10>> auto ten [[maybe_unused]]
    = lmno::invoke(std::plus<>{}, Const<2>{}, Const<8>{});

    std::same_as<Const<10>> auto ten2 [[maybe_unused]]
    = lmno::invoke(Const<std::plus<>{}>{}, Const<2>{}, Const<8>{});

    std::same_as<int> auto ten3 [[maybe_unused]]
    = lmno::invoke(Const<std::plus<>{}>{}, Const<2>{}, 8);

    std::same_as<int> auto ten4 [[maybe_unused]] = lmno::invoke(Const<std::plus<>{}>{}, 2, 8);

    std::same_as<Const<rational{4, 3}>> auto four_thirds [[maybe_unused]]
    = lmno::invoke(std::divides<>{}, Const<rational{4}>{}, Const<rational{3}>{});

    // Pass an lvalue
    auto div_fn = std::divides<>{};

    std::same_as<Const<rational{4, 3}>> auto four_thirds2 [[maybe_unused]]
    = lmno::invoke(div_fn, Const<rational{4}>{}, Const<rational{3}>{});
    std::same_as<Const<rational{4, 3}>> auto four_thirds3 [[maybe_unused]]
    = lmno::invoke(std::as_const(div_fn), Const<rational{4}>{}, Const<rational{3}>{});

    constexpr auto         add2                   = add{2};
    std::same_as<int> auto four1 [[maybe_unused]] = lmno::invoke(add2, 2);
    std::same_as<int> auto four2 [[maybe_unused]] = lmno::invoke(add2, Const<2>{});

    constexpr auto         add2const              = Const<add{2}>{};
    std::same_as<int> auto four3 [[maybe_unused]] = lmno::invoke(add2const, 2);
}
