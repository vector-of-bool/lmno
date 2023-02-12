#include "./ast.hpp"
#include "./parse.hpp"

namespace ast = lmno::ast;
using ast::name;
using ast::render_v;

namespace {

static_assert(render_v<name<"foo">> == "foo");

static_assert(render_v<name<"⍳">> == "⍳");

static_assert(render_v<ast::monad<name<"foo">, name<"bar">>> == "foo bar");

static_assert(render_v<lmno::parse_t<"foo bar baz">> == "foo bar baz");
static_assert(render_v<lmno::parse_t<"foo bar baz egg">> == "foo bar baz egg");
static_assert(render_v<lmno::parse_t<"foo bar baz quux another">> == "foo bar baz quux another");
static_assert(render_v<lmno::parse_t<"foo bar baz · quux another">>
              == "foo bar baz · quux another");

static_assert(render_v<lmno::parse_t<"foo bar baz    · quux another">>
              == "foo bar baz · quux another");

static_assert(render_v<lmno::parse_t<"foo bar : baz">> == "foo (bar baz)");

static_assert(render_v<lmno::parse_t<"foo bar : baz quux">> == "foo (bar baz) quux");
static_assert(render_v<lmno::parse_t<"foo bar baz : quux">> == "foo bar (baz quux)");

static_assert(render_v<lmno::parse_t<"foo ; bar">> == "foo ; bar");
static_assert(render_v<lmno::parse_t<"foo  ←   two ; bar">> == "foo ← two ; bar");
static_assert(render_v<lmno::parse_t<"foo  ←  {5⊸+} ; bar">> == "foo ← {5 ⊸ +} ; bar");
static_assert(ast::render_value_v<int, 12> == "12");
static_assert(ast::render_value_v<int, 0> == "0");
static_assert(ast::render_value_v<int, -210> == "¯210");

static_assert(render_v<lmno::parse_t<"⍳5">> == "⍳ 5");
static_assert(render_v<lmno::parse_t<"-5">> == "- 5");

static_assert(render_v<lmno::parse_t<"+φ:=×">> == "+ (φ =) ×");

static_assert(render_v<lmno::parse_t<"1‿2‿2‿Q‿1">> == "1‿2‿2‿Q‿1");

}  // namespace
