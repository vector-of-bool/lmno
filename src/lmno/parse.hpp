#pragma once

#include "./ast.hpp"
#include "./lex.hpp"

// Expands to "typename", because I'm lazy
#define tn typename

namespace lmno {

namespace parse_detail {

using lex::token;
using lex::token_list;
using namespace ast;
using namespace meta;
using neo::meta::split_at;
using u64 = std::uint64_t;

enum ikind {
    k_done = 0,
    k_name,
    k_train,
    k_block,
    k_assign,
    k_seq,
    k_strand,
    k_int,
    k_nothing,
};

// A node in the tree construction
struct node {
    // What kind of node?
    ikind kind;
    // Value associated with the token. Meaning on the kind.
    u64 n;
};

// Takes a list of expression nodes and collapse into a single prefix/infix expression
// NOTE that the node list comes from the expression stack, which is in reverse order.
template <tn Nodes>
struct collapse_chain;

// Handle three or more items:
template <tn X, tn F, tn W, tn... More>
struct collapse_chain<list<X, F, W, More...>>
    // Bind the first three into a diadic item, and recurse:
    : collapse_chain<list<dyad<W, F, X>, More...>> {};

// Handle many items:
template <tn X, tn F, tn W, tn F2, tn W2, tn F3, tn W3, tn F4, tn W4, tn... More>
struct collapse_chain<list<X, F, W, F2, W2, F3, W3, F4, W4, More...>>
    // Bind the first several into a chain of infixes:
    : collapse_chain<list<dyad<W4, F4, dyad<W3, F3, dyad<W2, F2, dyad<W, F, X>>>>, More...>> {};

// Base case: Two items:
template <tn X, tn F>
struct collapse_chain<list<X, F>> {
    using type = monad<F, X>;
};

// Base case: One item:
template <tn X>
struct collapse_chain<list<X>> {
    using type = X;
};

// Convert the array of node instructions into the AST node tree
template <auto Tokens>
struct executor {
    // The unused "void" is to work around unimplemented CWG 727 fixes.
    template <ikind K, tn = void>
    struct step;

    template <ikind K, tn = void>
    struct parse;

    /**
     * @brief Run the parse:
     *
     * @tparam Nodes The array of nodes that will be used to construct the tree
     * @tparam Idx The index into the array of nodes to execute. Begins at zero
     * @tparam Stack An accumulator of ast:: nodes that we are building. When finished, should have
     * one element.
     */
    template <auto Nodes, u64 Idx = 0, tn Stack = list<>>
    using exec_parse_t = parse<Nodes[Idx].kind, void>::template f<Nodes, Idx, Stack>;

    template <tn Void>
    struct parse<k_done, Void> {
        // Final state: Return the one node:
        template <auto, auto, tn Stack>
        using f = head<Stack>;
    };

    template <ikind K, tn Void>
    struct parse {
        // Get the step based on the kind of the node:
        using MyStep = step<K>;
        // Recursive case:
        template <auto Nodes,
                  auto Idx,
                  // The stack that we receive:
                  tn StackIn,
                  // Apply the step function to the node and receive the transformed
                  // stack:
                  tn StackOut = MyStep::template f<Nodes[Idx].n, StackIn>>
        // Recurse into the parser, to the next node instruction, with the new stack:
        using f = exec_parse_t<Nodes, Idx + 1, StackOut>;
    };

    // A "·" node:
    template <tn Void>
    struct step<k_nothing, Void> {
        template <u64, tn Stack>
        using f = meta::push_front<Stack, nothing>;
    };

    // An integer literal:
    template <tn Void>
    struct step<k_int, Void> {
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

        // Parse the Nth token as an integer:
        template <u64 N, tn Stack>
        using f = meta::push_front<Stack, Const<parse_int(std::string_view(Tokens[N]))>>;
    };

    template <tn Void>
    struct step<k_name, Void> {
        // Bind the Nth token as a name:
        template <u64 N, tn Stack>
        using f = meta::push_front<Stack, name<Tokens[N]>>;
    };

    template <tn Void>
    struct step<k_block, Void> {
        // Wrap the top AST node in an ast::block
        template <u64,
                  tn Stack,
                  tn Head   = head<Stack>,
                  tn Blk    = block<Head>,
                  tn Tail   = tail<Stack>,
                  tn Stack2 = push_front<Tail, Blk>>
        using f = Stack2;
    };

    template <tn Void>
    struct step<k_train, Void> {
        // Collapse the prior 'Count' nodes into a single prefix/infix node:
        template <u64 Count,
                  tn  Stack,
                  tn  Split = split_at<Stack, Count>,
                  tn  Head  = head<Split>,
                  tn  Tail  = second<Split>,
                  tn AST    = tn collapse_chain<Head>::type>
        using f = push_front<Tail, AST>;
    };

    template <tn Void>
    struct step<k_strand, Void> {
        // Collapse the prior 'Count' nodes into a strand node:
        template <u64 Count,
                  tn  Stack,
                  tn  Split = split_at<Stack, Count>,
                  tn  Head  = head<Split>,
                  tn  Tail  = tail<Split>,
                  tn  AST   = rebind<reverse<Head>, strand>>
        using f = push_front<Tail, AST>;
    };

    template <tn Void>
    struct step<k_assign, Void> {
        // Create an assignment from the prior two nodes:
        template <u64,
                  tn Stack,
                  tn Val = head<Stack>,
                  tn ID  = second<Stack>,
                  tn Asn = assignment<ID, Val>>
        using f = push_front<tail<tail<Stack>>, Asn>;
    };

    template <tn Void>
    struct step<k_seq, Void> {
        // Wrap the prior 'Count' nodes in an ast::stmt_seq
        template <u64 Count,
                  tn  Stack,
                  tn  Split = split_at<Stack, Count>,
                  tn  Head  = head<Split>,
                  tn  Tail  = tail<Split>,
                  tn  Seq   = rebind<reverse<Head>, stmt_seq>>
        using f = push_front<Tail, Seq>;
    };
};

struct token_iter {
    const token* _base;
    // The current position in the token array
    u64 pos = 0;

    constexpr const token& get() const noexcept { return _base[pos]; }
};

struct parser3 {
    constexpr static auto parse_primary(node*& into, token_iter& it) {
        auto       tk = std::string_view(it.get());
        const char c  = tk.front();
        if (lex::is_alpha(c)) {
            *into++ = {k_name, static_cast<u64>(it.pos)};
            it.pos++;
        } else if (lex::is_digit(c) or tk.starts_with("¯")) {
            *into++ = {k_int, static_cast<u64>(it.pos)};
            it.pos++;
        } else if (c == '(') {
            it.pos++;
            parse_top(into, it);
            if (it.get()[0] != ')') {
                throw "Imbalanced parentheses";
            }
            it.pos++;
        } else if (c == '{') {
            it.pos++;
            parse_top(into, it);
            *into++ = {k_block, 0};
            if (it.get()[0] != '}') {
                throw "Imbalanced braces";
            }
            it.pos++;
        } else if (tk == "·") {
            *into++ = {k_nothing, 0};
            it.pos++;
        } else {
            *into++ = {k_name, it.pos};
            it.pos++;
        }
    }

    constexpr static void parse_strand(node*& into, token_iter& it) {
        parse_primary(into, it);
        u64 n_exprs = 1;
        while (it.get() == "‿") {
            it.pos++;
            parse_primary(into, it);
            ++n_exprs;
        }
        if (n_exprs > 1) {
            *into++ = {k_strand, n_exprs};
        }
    }

    constexpr static auto parse_colon(node*& into, token_iter& it) {
        parse_strand(into, it);
        while (it.get()[0] == ':') {
            it.pos++;
            parse_strand(into, it);
            *into++ = {k_train, 2};
        }
    }

    constexpr static void parse_main(node*& into, token_iter& it) {
        u64 n_exprs = 0;
        while (1) {
            parse_colon(into, it);
            ++n_exprs;
            const token& tk = it.get();
            char         c  = tk[0];
            if (c == ':' or c == ')' or c == '.' or c == '}' or c == 0 or c == ';' or tk == "←") {
                break;
            }
        }
        if (n_exprs > 1) {
            *into++ = {k_train, n_exprs};
        }
    }

    constexpr static void parse_dots(node*& into, token_iter& it) {
        u64 n_exprs = 0;
        while (1) {
            parse_main(into, it);
            n_exprs++;
            if (it.get()[0] != '.') {
                break;
            }
            it.pos++;
            if (it.get()[0] == '.') {
                it.pos++;
                continue;
            }
            parse_colon(into, it);
            ++n_exprs;
        }
        if (n_exprs > 1) {
            *into++ = {k_train, n_exprs};
        }
    }

    constexpr static void parse_assign(node*& into, token_iter& it) {
        parse_dots(into, it);
        if (it.get() == "←") {
            it.pos++;
            parse_dots(into, it);
            *into++ = {k_assign, 0};
        }
    }

    constexpr static void parse_seq(node*& into, token_iter& it) {
        u64 n_exprs = 0;
        while (1) {
            parse_assign(into, it);
            ++n_exprs;
            if (it.get()[0] != ';') {
                break;
            }
            it.pos++;
        }
        if (n_exprs > 1) {
            *into++ = {k_seq, n_exprs};
        }
    }

    static constexpr void parse_top(node*& into, token_iter& it) { parse_seq(into, it); }

    template <auto Arr>
    static constexpr auto make_parser() {
        std::array<node, Arr.size()* 2> ret = {};
        token_iter                      iter{Arr.data()};
        auto                            into = ret.data();
        parse_top(into, iter);
        *into = {k_done, 0};
        return ret;
    }

    template <auto Arr, std::size_t... I>
    static constexpr auto as_vlist(std::index_sequence<I...>) -> vlist<Arr[I]...>;

    template <token... Tokens>
    auto parse(token_list<Tokens...>*) {
        // Create a constexpr array of the tokens:
        constexpr std::array<token, sizeof...(Tokens) + 1> arr = {Tokens...};
        // Calculate an array that will instruct the expression regrouper
        constexpr auto nodes = make_parser<arr>();
        using ast            = executor<arr>::template exec_parse_t<nodes>;
        return ptr<ast>{};
    }
};

}  // namespace parse_detail

template <typename AST>
using parse_tokens_t
    = neo::remove_pointer_t<decltype(parse_detail::parser3{}.parse(meta::ptr<AST>{}))>;

template <cx_str Code>
using parse_t = parse_tokens_t<lex::tokenize_t<Code>>;

}  // namespace lmno

#undef tn
