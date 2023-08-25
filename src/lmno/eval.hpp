#pragma once

#include "./context.hpp"
#include "./define.hpp"
#include "./error.hpp"
#include "./invoke.hpp"
#include "./parse.hpp"
#include "./render.hpp"
#include "./strand.hpp"

#include <neo/fwd.hpp>
#include <neo/invoke.hpp>

namespace lmno {

/**
 * @brief Function that evaluates an AST with the given semantics and name lookup context
 *
 * @param code The AST to evaluate.
 * @param sema The semantics of evaluation.
 * @param ctx The name lookup context.
 *
 * For a simpler use cases, use lmno::eval()
 */
inline constexpr struct evaluate_fn {
    template <typename Code, typename Sema, typename Context>
    constexpr decltype(auto) operator()(Code&& code, Sema&& sema, Context&& ctx) const {
        return sema.evaluate(ctx, code);
    }
} evaluate;

/**
 * @brief A closure object generated from a function block, capturing the names
 * and semantics of the encosing scope
 *
 * @tparam Code The code within the block
 * @tparam Sema The semantics of evaluation
 * @tparam BoundContext The name context captured when the block was evaluated
 */
template <typename Code, typename Sema, typename BoundContext>
struct closure {
    NEO_NO_UNIQUE_ADDRESS Sema         _sema;
    NEO_NO_UNIQUE_ADDRESS BoundContext _bound;

    constexpr decltype(auto) operator()(auto&& x) const {
        auto inner = _bound.bind(lmno::make_named<"α">(ast::nothing{}),  //
                                 lmno::make_named<"ω">(NEO_FWD(x)));
        return invoke(evaluate, Code{}, _sema, inner);
    }

    constexpr decltype(auto) operator()(auto&& w, auto&& x) const {
        auto inner = _bound.bind(lmno::make_named<"α">(NEO_FWD(w)),  //
                                 lmno::make_named<"ω">(NEO_FWD(x)));
        return invoke(evaluate, Code{}, _sema, inner);
    }
};
LMNO_AUTO_CTAD_GUIDE(closure);

template <typename Code, typename S, typename B>
constexpr auto render::type_v<closure<Code, S, B>>
    = cx_fmt_v<"(closure {{{}}})", ast::render_v<Code>>;

/**
 * @brief The default language evaluator semantics
 */
struct default_sema {
    // A typed constant evaluates to itself
    template <typed_constant C>
    constexpr C evaluate(auto&&, C v) const noexcept {
        return v;
    }

    // A dyadic invocation with an ast::nothing "·" on the left-hand is a monadic invocation
    template <typename Mid, typename Right>
    constexpr decltype(auto) evaluate(const auto& context,
                                      ast::dyad<ast::nothing, Mid, Right>) const {
        return this->evaluate(context, ast::monad<Mid, Right>{});
    }

    // Dyadic "infix" function application:
    template <typename Left, typename Mid, typename Right>
    constexpr auto evaluate(const auto& context, ast::dyad<Left, Mid, Right>) const {
        decltype(auto) left  = this->evaluate(context, Left{});
        decltype(auto) fn    = this->evaluate(context, Mid{});
        decltype(auto) right = this->evaluate(context, Right{});
        if constexpr (LMNO_IS_ERROR(left)) {
            return left;
        } else if constexpr (LMNO_IS_ERROR(right)) {
            return right;
        } else if constexpr (LMNO_IS_ERROR(fn)) {
            return fn;
        } else {
            return invoke(NEO_FWD(fn), NEO_FWD(left), NEO_FWD(right));
        }
    }

    // A name resolves using the current context:
    template <lex::token Name>
    constexpr decltype(auto) evaluate(const auto& context, ast::name<Name>) const noexcept {
        return context.template get<Name>();
    }

    // A monadic "prefix" function application:
    template <typename Right, typename Fn>
    constexpr auto evaluate(const auto& context, ast::monad<Fn, Right>) const {
        decltype(auto) fn    = this->evaluate(context, Fn{});
        decltype(auto) right = this->evaluate(context, Right{});
        if constexpr (LMNO_IS_ERROR(fn)) {
            return fn;
        } else if constexpr (LMNO_IS_ERROR(right)) {
            return right;
        } else {
            return invoke(NEO_FWD(fn), NEO_FWD(right));
        }
    }

    // Strand evaluation is a bit trickier:
    template <typename... Elems>
    constexpr decltype(auto) evaluate(const auto& context, ast::strand<Elems...> s) const {
        // Capture the types of each element's evaluation:
        using eval_type = meta::list<decltype(this->evaluate(context, Elems{}))...>;
        return this->eval_strand(context, s, meta::ptr<eval_type>{});
    }

    template <typename... Elems, typename... ElemEvals>
    constexpr auto
    eval_strand(const auto& context, ast::strand<Elems...>, meta::list<ElemEvals...>*) const {
        // Check that we will have a common reference
        if constexpr (not(neo::has_common_reference<unconst_t<ElemEvals>> and ...)) {
            return err::make_error<
                "Cannot form a strand for {:'}: There is no common reference type between the "
                "evaluated element types ({})",
                ast::render_v<ast::strand<Elems...>>,
                cx_str_join_v<", ", cx_fmt_v<"{:'}", render::type_v<ElemEvals>...>>>();
        } else {
            // Evaluate each element and construct the strand range:
            return strand_range{strand_range_construct_tag_t{},
                                this->evaluate(context, Elems{})...};
        }
    }

    // Evaluation of a function block generates a closure:
    template <typename Code, typename Ctx>
    constexpr auto evaluate(const Ctx& context, ast::block<Code>) const {
        auto c = lmno::closure<Code, default_sema, Ctx>{*this, context};
        return c;
    }

    // This one only handles the case of a lone assignment with no subsequent statements. The name
    // goes nowhere, but that's still semantically valid:
    template <typename ID, typename Code>
    constexpr decltype(auto) evaluate(const auto& context, ast::assignment<ID, Code>) const {
        return this->evaluate(context, Code{});
    }

    // Evaluation of statements:
    template <typename... Stmts>
    constexpr decltype(auto) evaluate(const auto& context, ast::stmt_seq<Stmts...> seq) const {
        return this->evaluate_stmts(context, seq);
    }

    template <typename Final>
    constexpr decltype(auto) evaluate_stmts(const auto& context, ast::stmt_seq<Final>) const {
        // Case: The final statement in a statement sequence. May be an assignment,
        // but the name will go nowhere
        return this->evaluate(context, Final{});
    }

    template <typename Head, typename... Tail>
    constexpr auto evaluate_stmts(const auto& context, ast::stmt_seq<Head, Tail...>) const {
        // Case: Head is a non-assignment expression
        auto&& el = this->evaluate(context, Head{});
        if constexpr (LMNO_IS_ERROR(el)) {
            return el;
        } else {
            return this->evaluate_stmts(context, ast::stmt_seq<Tail...>{});
        }
    }

    template <typename ID, typename RHS, typename Peek, typename... Tail>
    constexpr auto evaluate_stmts(const auto& context,
                                  ast::stmt_seq<ast::assignment<ID, RHS>, Peek, Tail...>) const {
        // Case: Head is an assignment expression
        auto&& value = this->evaluate(context, RHS{});
        if constexpr (LMNO_IS_ERROR(value)) {
            return value;
        } else {
            auto ctx = this->bind_assignment(context, ID{}, NEO_FWD(value));
            return this->evaluate_stmts(ctx, ast::stmt_seq<Peek, Tail...>{});
        }
    }

    template <lex::token Name>
    constexpr decltype(auto)
    bind_assignment(const auto& context, ast::name<Name>, auto&& value) const {
        return context.bind(lmno::make_named<Name>(NEO_FWD(value)));
    }
};

template <typename Code>
constexpr auto eval() -> invoke_t<evaluate_fn const&, Code, default_sema, default_context<>> {
    return invoke(evaluate, Code{}, default_sema{}, default_context{});
}

template <cx_str CodeStr, typename Parsed = parse_t<CodeStr>>
constexpr auto eval() -> decltype(eval<Parsed>()) {
    return eval<Parsed>();
}

template <cx_str CodeStr>
constexpr auto eval_v = lmno::eval<CodeStr>();

template <cx_str S, typename Sema = default_sema>
using eval_t = decltype(NEO_DECLVAL(Sema).evaluate(default_context{}, parse_t<S>{}));

template <typename AST, typename Sema>
using eval_ast_t = decltype(eval<AST>(NEO_DECLVAL(Sema)));

}  // namespace lmno
