#pragma once

#include "./ast.hpp"
#include "./lex.hpp"

namespace lmno {

namespace detail {

// Sentinel representing the EOF token
constexpr inline lex::token EOF_TOKEN = lex::token{"<eof>"};

// Holds the result of a successful parse
template <typename T, typename Tail>
struct parse_result {
    // The actual node that was generated
    static constexpr T result{};
    // The remaining tokens following the parse
    static constexpr Tail tail{};

    constexpr parse_result(T, Tail) {}

    // Always 'true' for parse_result
    constexpr static bool okay = true;
};
LMNO_AUTO_CTAD_GUIDE(parse_result);

// Holds an error of a failed parse
template <cx_str Error>
struct parse_error {
    // Always 'false' for parse_error
    constexpr static bool okay = false;
};

// Genreate a parse_error of the given string
template <cx_str E>
constexpr auto parse_error_v = parse_error<E>{};

// Regroup a set of expressions according to the associativity rules
template <typename T>
struct parse_regroup;

template <typename T>
using parse_regroup_t = parse_regroup<std::remove_cvref_t<T>>::type;

// If we have three expressions, that's a dyadic invocation (Note the reversal):
template <typename Z, typename Y, typename X, typename... Tail>
struct parse_regroup<meta::list<Z, Y, X, Tail...>>
    : parse_regroup<meta::list<ast::dyad<X, Y, Z>, Tail...>> {};

// Two expressions is a monadic invocation. It can only appear at the end of an expression sequence
// (also reversed)
template <typename Y, typename X>
struct parse_regroup<meta::list<Y, X>> {
    using type = ast::monad<X, Y>;
};

// A single expression is just that expression:
template <typename X>
struct parse_regroup<meta::list<X>> {
    using type = X;
};

struct parse_impl {
    // Convert an integer token into the std::int64_t value
    constexpr static std::int64_t parse_int(std::string_view sv) {
        int fac = 1;
        if (sv.starts_with("¯")) {
            fac = -1;
            sv  = sv.substr(2);  // drop two bytes for the prefix
        }
        std::int64_t ret = 0;
        for (auto it = sv.begin(); it != sv.end(); ++it) {
            ret *= 10;
            ret += *it - '0';
        }
        return ret * fac;
    }

    // Regroup a sequence of function applications
    auto regroup_result(auto res) {
        return parse_result{parse_regroup_t<decltype(res.result)>(), res.tail};
    }

    // Expression-edge detection
    constexpr static bool is_edge(auto r) {
        return r.empty or r.starts_with(")") or r.starts_with("}") or r.starts_with(EOF_TOKEN)
            or r.starts_with("⋄") or r.starts_with(".");
    }

    auto parse_primary(auto toks) {
        constexpr auto first = std::string_view(toks.head);
        constexpr auto c     = first[0];
        // Integer literals:
        if constexpr (lex::is_digit(c) or first.starts_with("¯")) {
            return parse_result{Const<parse_int(std::string_view(toks.head))>{}, toks.tail};

        }
        // Identifier names:
        else if constexpr (lex::is_alpha(c)) {
            return parse_result{ast::name<toks.head>{}, toks.tail};
        }
        // A sentinel for an invalid token:
        else if constexpr (c == '\x1b') {
            return parse_error_v<"An invalid token appears within the source">;
        }
        // The "nothing" token:
        else if constexpr (std::string_view(toks.head) == "·") {
            return parse_result{ast::nothing{}, toks.tail};
        }
        // The beginning of a function block:
        else if constexpr (c == '{') {
            auto blk = parse_stmt_seq(toks.tail);
            if constexpr (not blk.okay) {
                return blk;
            } else {
                if constexpr (not blk.tail.starts_with("}")) {
                    return parse_error_v<"Missing closing brace '}' for a block">;
                } else {
                    return parse_result{ast::block<LMNO_TYPEOF(blk.result)>{}, blk.tail.tail};
                }
            }
        }
        // The beginning of a grouped subexpression:
        else if constexpr (c == '(') {
            auto inner = parse_stmt_seq(toks.tail);
            if constexpr (not inner.okay) {
                return inner;
            } else {
                if constexpr (inner.tail.starts_with(")")) {
                    return parse_result{inner.result, inner.tail.tail};
                } else {
                    return parse_error_v<"Missing closing parenthesis in expression">;
                }
            }
        }
        // Detect a missing expression:
        else if constexpr (is_edge(toks) or toks.starts_with(":") or toks.starts_with("‿")
                           or toks.starts_with("←") or toks.starts_with(".")) {
            return parse_error_v<
                cx_fmt_v<"Expected an expression, but got {:'}.", LMNO_CX_STR(toks.head)>>;
        }
        // Treat anything else as a name:
        else {
            return parse_result{ast::name<toks.head>{}, toks.tail};
        }
    }

    template <typename... Elems>
    auto parse_strand_seq(auto toks, meta::list<Elems...> l = {}) {
        auto first = parse_primary(toks);
        if constexpr (not first.okay) {
            return first;
        } else if constexpr (not first.tail.starts_with("‿")) {
            return parse_result{fin_strand_seq(l, first.result), first.tail};
        } else {
            return parse_strand_seq(first.tail.tail, l.push_back(first.result));
        }
    }

    template <typename... Elems, typename Fin>
    auto fin_strand_seq(meta::list<Elems...>, Fin) {
        return ast::strand<Elems..., Fin>{};
    }

    auto fin_strand_seq(meta::list<>, auto fin) { return fin; }

    auto parse_col_seq(auto toks) {
        auto first = parse_strand_seq(toks, meta::list<>{});
        if constexpr (not first.okay) {
            return first;
        } else if constexpr (not first.tail.starts_with(":")) {
            return first;
        } else {
            auto next = parse_col_seq(first.tail.tail);
            if constexpr (not next.okay) {
                return next;
            } else {
                return parse_result{ast::monad<neo::remove_cvref_t<decltype(first.result)>,
                                               neo::remove_cvref_t<decltype(next.result)>>{},
                                    next.tail};
            }
        }
    }

    auto parse_fn_seq(auto toks) {
        auto r = do_parse_fn_seq(toks, meta::list<>{});
        if constexpr (not r.okay) {
            return r;
        } else {
            return regroup_result(r);
        }
    }

    auto do_parse_fn_seq(auto toks, auto acc) {
        auto first = parse_col_seq(toks);
        if constexpr (not first.okay) {
            return first;
        } else {
            auto here = acc.push_front(first.result);
            if constexpr (is_edge(first.tail) or first.tail.starts_with(":")) {
                return parse_result{here, first.tail};
            } else {
                return do_parse_fn_seq(first.tail, here);
            }
        }
    }

    auto parse_dot_seq(auto toks) {
        auto r = do_parse_dot_seq(toks, meta::list<>{});
        if constexpr (not r.okay) {
            return r;
        } else {
            return regroup_result(r);
        }
    }

    auto do_parse_dot_seq(auto toks, auto acc) {
        auto first = parse_fn_seq(toks);
        if constexpr (not first.okay) {
            return first;
        } else {
            auto here = acc.push_front(first.result);
            if constexpr (not first.tail.starts_with(".")) {
                return parse_result{here, first.tail};
            } else {
                // The tokens after the dot:
                auto tail = first.tail.tail;
                if constexpr (tail.starts_with(".")) {
                    // Two subsequent dots:
                    return do_parse_dot_seq(tail.tail, here);
                } else {
                    // Parse the next element as a tight expression:
                    auto mid = parse_col_seq(tail);
                    if constexpr (not mid.okay) {
                        return mid;
                    } else {
                        auto then = here.push_front(mid.result);
                        return do_parse_dot_seq(mid.tail, then);
                    }
                }
            }
        }
    }

    auto parse_assignment(auto toks) {
        auto id = parse_primary(toks);
        if constexpr (not id.okay) {
            return id;
        } else {
            if constexpr (not id.tail.starts_with("←")) {
                // Backtrack, parse again
                return parse_dot_seq(toks);
            } else {
                auto rhs = parse_dot_seq(id.tail.tail);
                if constexpr (not rhs.okay) {
                    return rhs;
                } else {
                    return parse_result{ast::assignment<LMNO_TYPEOF(id.result),
                                                        LMNO_TYPEOF(rhs.result)>{},
                                        rhs.tail};
                }
            }
        }
    }

    template <typename... Stmts>
    auto parse_stmt_seq(auto toks, meta::list<Stmts...> l = {}) {
        auto asn = parse_assignment(toks);
        if constexpr (not asn.okay) {
            return asn;
        } else if constexpr (not asn.tail.starts_with("⋄")) {
            return parse_result{fin_stmt_seq(l, asn.result), asn.tail};
        } else {
            return parse_stmt_seq(asn.tail.tail, l.push_back(asn.result));
        }
    }

    template <typename... Stmts, typename Fin>
    auto fin_stmt_seq(meta::list<Stmts...>, Fin) {
        return ast::stmt_seq<Stmts..., Fin>{};
    }

    auto fin_stmt_seq(meta::list<>, auto n) { return n; }

    template <lex::token... Toks>
    auto parse_root(lex::token_list<Toks...>) {
        auto r = parse_stmt_seq(lex::token_list<Toks..., EOF_TOKEN>{});
        if constexpr (r.okay) {
            return r.result;
        } else {
            return r;
        }
    }
};

}  // namespace detail

template <typename Tokens>
using parse_tokens_t = decltype(detail::parse_impl{}.parse_root(Tokens{}));

template <cx_str S>
using parse_t = parse_tokens_t<lex::tokenize_t<S>>;

}  // namespace lmno
