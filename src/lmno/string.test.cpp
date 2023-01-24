
#include "./string.hpp"

#include "./lex.hpp"

using lmno::cx_fmt_v;
using lmno::cx_str;

namespace {

template <lmno::cx_str S>
struct C {};

static_assert(lmno::cx_string_size("hello") == 5);
static_assert(lmno::cx_string_size(lmno::cx_str{"Hello"}) == 5);

static_assert(std::same_as<C<lmno::cx_fmt_v<"Hi">>, C<"Hi">>);
static_assert(std::same_as<C<lmno::cx_fmt<"{}">("Hi")>, C<"Hi">>);
static_assert(std::same_as<C<lmno::cx_fmt<"{{}}">()>, C<"{}">>);
constexpr auto in = lmno::cx_fmt<"{:}">("Hi");
static_assert(std::same_as<C<in>, C<"Hi">>);
constexpr auto in2 = lmno::cx_fmt<"{{}}">();
static_assert(std::same_as<C<in2>, C<"{}">>);

static_assert(lmno::cx_fmt<"Hello {}!">("user") == "Hello user!");
static_assert(lmno::cx_fmt<"Hello {:} {}!">("Joe", "Armstrong") == "Hello Joe Armstrong!");
static_assert(lmno::cx_fmt<"Hello {:'} {}!">("Joe", "Armstrong") == "Hello ‘Joe’ Armstrong!");
static_assert(lmno::cx_fmt<"Hello {:'}">("Joe") == "Hello ‘Joe’");

C<"Hello, world!"> _1 [[maybe_unused]]
= C<lmno::cx_str_replace<"Hello, wrold?!", cx_str{"wrold?"}, cx_str{"world"}>()>{};

C<"foo    baz  foo baz"> _2 [[maybe_unused]]
= C<lmno::cx_str_replace<"foo bar bar bar baz bar foobar baz", cx_str{"bar"}, cx_str{""}>()>{};

}  // namespace

int main() {}
