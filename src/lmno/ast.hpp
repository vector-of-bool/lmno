#pragma once

#include "./lex.hpp"
#include "./render.hpp"

#include <concepts>

namespace lmno::ast {

using lex::token;

/**
 * @brief An infix application expression
 *
 * @tparam W The left-hand operand
 * @tparam F The invocable in the middle
 * @tparam X The right-hand operand
 */
template <typename W, typename F, typename X>
struct dyad {};

/**
 * @brief A prefix application
 *
 * @tparam F The invocable on the left
 * @tparam X The operand
 */
template <typename F, typename X>
struct monad {};

/**
 * @brief A name
 */
template <token N>
struct name {};

/**
 * @brief The special 'nothing' node
 */
struct nothing {};

/**
 * @brief A block-expression (function)
 *
 * @tparam Inner The code in between the braces
 */
template <typename Inner>
struct block {};

/**
 * @brief An assignment expression
 *
 * @tparam Name The name being assigned to
 * @tparam Expr The expression of the assignment
 */
template <typename Name, typename Expr>
struct assignment {};

/**
 * @brief A strand‿expression
 *
 * @tparam Elems The elements being stranded together
 */
template <typename... Elems>
struct strand {};

/**
 * @brief A sequence of expressions separated by diamonds "⋄"
 *
 * @tparam Stmts The individual nodes
 */
template <typename... Stmts>
struct stmt_seq {};

template <typename AST>
struct render_not_implemented {};

/**
 * @brief Render an AST element type
 */
template <typename AST>
constexpr auto render_v = render::type_v<render_not_implemented<AST>>;

template <typename T, T Value>
constexpr auto render_value_v = render::value_of_type_v<T, Value>;

template <std::integral T, T Value>
constexpr auto render_value_v<T, Value> = render::integer_v<Value>;

namespace detail {

template <typename AST>
constexpr auto render_operand_v = cx_fmt_v<"({})", render_v<AST>>;

template <token N>
constexpr auto render_operand_v<name<N>> = cx_str<N.size()>{N.str};

template <typed_constant C>
constexpr auto render_operand_v<C> = render_v<C>;

template <>
constexpr auto render_operand_v<nothing> = cx_str{"·"};

template <typename AST>
constexpr auto render_monad_rhs_v = render_operand_v<AST>;

template <typename L, typename F, typename R>
constexpr auto render_monad_rhs_v<dyad<L, F, R>> = render_v<dyad<L, F, R>>;

template <typename AST>
constexpr auto render_dyad_rhs_v = render_v<AST>;

template <typename L, typename R>
constexpr auto render_dyad_rhs_v<monad<L, R>> = render_operand_v<monad<L, R>>;

constexpr std::size_t n_chars_required(auto v) {
    if (v == 0) {
        return 1;
    }
    if (v < 0) {
        return 2 + n_chars_required(-v);
    }
    std::size_t r = 0;
    while (v) {
        ++r;
        v /= 10;
    }
    return r;
}

template <auto Value>
constexpr auto render_integral(Const<Value>) noexcept {
    constexpr auto    NumDigits = n_chars_required(Value);
    cx_str<NumDigits> string;
    auto              out = string.chars + NumDigits - 1;
    if (Value == 0) {
        *out = '0';
    }
    for (auto val = Value < 0 ? -Value : Value; val; val /= 10, --out) {
        auto mod = val % 10;
        *out     = char('0' + mod);
    }
    if (Value < 0) {
        *out-- = char(0xaf);
        *out   = char(0xc2);
    }
    return string;
}

template <typename L>
constexpr auto render_template() {
    constexpr std::string_view t         = NAMEOF_TYPE(L);
    constexpr auto             angle_pos = t.find("<");
    return cx_str<angle_pos>(t.substr(0, angle_pos));
}

}  // namespace detail

template <token N>
constexpr auto render_v<name<N>> = cx_str<N.size()>(N.str);

template <typename T>
constexpr auto render_constant_value_v = nullptr;

template <auto Value>
constexpr auto render_constant_value_v<Const<Value>> = render::integer_v<Value>;

template <>
constexpr auto render_v<nothing> = cx_str{"·"};

template <typename F, typename R>
constexpr auto render_v<monad<F, R>>
    = cx_fmt_v<"{} {}", detail::render_operand_v<F>, detail::render_monad_rhs_v<R>>;

template <typename L, typename F, typename R>
constexpr auto render_v<dyad<L, F, R>> = cx_fmt_v<"{} {} {}",
                                                  detail::render_operand_v<L>,
                                                  detail::render_operand_v<F>,
                                                  detail::render_dyad_rhs_v<R>>;

template <typename... Stmts>
constexpr auto render_v<stmt_seq<Stmts...>> = cx_str_join_v<" ⋄ ", render_v<Stmts>...>;

template <typename L>
constexpr auto render_v<stmt_seq<L>> = render_v<L>;

template <typename ID, typename RHS>
constexpr auto render_v<assignment<ID, RHS>> = cx_fmt_v<"{} ← {}", render_v<ID>, render_v<RHS>>;

template <typename Code>
constexpr auto render_v<block<Code>> = cx_fmt_v<"{{{}}}", render_v<Code>>;

template <auto Value>
constexpr auto render_v<Const<Value>> = render::integer_v<Value>;

template <typename... Elems>
constexpr auto render_v<strand<Elems...>> = cx_str_join_v<"‿", detail::render_operand_v<Elems>...>;

template <typename T>
constexpr auto render() noexcept {
    return render_v<T>;
}

template <typename F>
    requires requires { F::dyadic_name(); }
constexpr auto dyadic_name() {
    constexpr auto n = F::dyadic_name();
    return cx_str<n.size()>(n);
}

template <typename F>
constexpr auto dyadic_name() {
    return render::type_v<F>;
}

template <typename F>
    requires requires { F::monadic_name(); }
constexpr auto monadic_name() {
    constexpr auto n = F::monadic_name();
    return cx_str<n.size()>(n);
}

template <typename F>
constexpr auto monadic_name() {
    return render::type_v<F>;
}

}  // namespace lmno::ast

namespace lmno::render {

template <typename L, typename F, typename R>
constexpr auto type_v<ast::dyad<L, F, R>>
    = cx_fmt_v<"<AST of ({:'})>", ast::render_v<ast::dyad<L, F, R>>>;

template <typename F, typename R>
constexpr auto type_v<ast::monad<F, R>>
    = cx_fmt_v<"<AST of ({:'})>", ast::render_v<ast::monad<F, R>>>;

}  // namespace lmno::render
