#pragma once

#include "./meta.hpp"
#include "./string.hpp"

#include <bit>
#include <cassert>
#include <string_view>
#include <utility>

namespace lmno::lex {

constexpr static std::size_t max_token_length = 32;
using meta::vlist;

/**
 * @brief Represents a LMNO token.
 *
 * Structural and valid as a non-type template parameter
 */
struct token {
    char str[max_token_length] = {};

    constexpr token() = default;

    constexpr token(const char* s) {
        auto dst = str;
        while (*s) {
            *dst++ = *s++;
        }
    }

    constexpr explicit operator std::string_view() const noexcept { return std::string_view(str); }

    constexpr std::size_t size() const noexcept { return std::char_traits<char>::length(str); }

    bool operator==(const token&) const = default;
};

template <token... Tokens>
struct token_list {};

template <>
struct token_list<> {
    static constexpr bool starts_with(auto) noexcept { return false; }
    constexpr static bool empty = true;
};

template <token First, token... Tail>
struct token_list<First, Tail...> {
    static constexpr token head = First;
    static constexpr auto  tail = token_list<Tail...>{};

    static constexpr bool starts_with(token t) noexcept { return t == head; }

    constexpr static bool empty = false;
};

constexpr bool is_digit(char c) { return c >= '0' and c <= '9'; }
constexpr bool is_alpha(char c) { return (c >= 'A' and c <= 'Z') or (c >= 'a' and c <= 'z'); }
constexpr bool is_alnum(char c) { return is_digit(c) or is_alpha(c); }
constexpr bool is_ident(char c) { return is_alnum(c) or c == '_'; }

namespace detail {

using meta::list;

constexpr auto fin_token(const char* s, std::size_t len) {
    token ret;
    for (auto i = 0u; i < len; ++i) {
        ret.str[i] = s[i];
    }
    return ret;
}

/**
 * @brief Represents the position of a token within a larger string
 *
 */
struct token_range {
    /// The beginning offset of the token
    std::size_t pos;
    /// The length of the token (in char)
    std::size_t len;
    /// The number of EOFs we have accumulated during an unrolled loop iteration
    int eof_counter = 0;
};

/**
 * @brief Obtain the next token_range from the given string at the specified offset
 *
 * @param begin A pointer to the very beginning of the source string
 * @param begin_pos The offset within the string to begin searching for another token
 * @param in_len
 * @param eof_counter
 * @return constexpr token_range
 */
constexpr token_range
next_token(const char* const begin, const std::size_t begin_pos, int eof_counter) {
    auto it = begin + begin_pos;
    // The beginning index of the token within the string view:
    std::size_t pos = begin_pos;
    // Skip leading whitespace
    while (*it == ' ' or *it == '\n') {
        ++it;
        ++pos;
    }

    // Create an EOF token at the end.
    if (*it == 0) {
        return {pos, 0, eof_counter + 1};
    }

    const char first = *it;
    if (first >= ' ' and first <= '~') {
        // Regular ASCII char
        if (first >= 'A') {
            if (is_ident(first)) {
                // Ident
                auto id_begin = it;
                ++it;
                while (is_ident(*it)) {
                    ++it;
                }
                auto len = static_cast<std::size_t>(it - id_begin);
                return {pos, len};
            } else {
                return {pos, 1};
            }
        } else {
            if (is_digit(first)) {
                auto num_begin = it;
                ++it;
                while (is_digit(*it)) {
                    ++it;
                }
                auto len = static_cast<std::size_t>(it - num_begin);
                return {pos, len};
            } else {
                if (first == ':') {
                    auto col_begin = it;
                    while (*it == ':') {
                        ++it;
                    }
                    auto len = static_cast<std::size_t>(it - col_begin);
                    return {pos, len};
                }
                return {pos, 1};
            }
        }
    } else {
        auto b = static_cast<std::uint8_t>(first);
        if (it[1] == char(0xaf) and it[0] == char(0xc2)) {
            // A leading hi-bar '¯', which introduces a negative literal
            token_range peek = next_token(begin, begin_pos + 2, eof_counter);
            peek.pos -= 2;
            peek.len += 2;
            return peek;
        } else if (b >= 0b1111'0000) {
            return {pos, 4};
        } else if (b >= 0b1110'0000) {
            return {pos, 3};
        } else if (b >= 0b1100'0000) {
            return {pos, 2};
        } else {
            throw "bogus";
        }
    }
}

template <token... Tokens>
auto unpack_tokens(list<vlist<Tokens>...>) -> token_list<Tokens...>;

template <std::size_t NumEOFs, typename Toks>
using eof_trim_t = decltype(unpack_tokens(meta::reverse<meta::remove_prefix<Toks, NumEOFs>>{}));

struct tokenizer {
    template <auto String, std::size_t Pos, token_range... Acc>
    constexpr static auto clang_tokenize_more(meta::listptr<vlist<String, Pos>, vlist<Acc...>>) {
        constexpr auto tok1 = detail::next_token(String.data(), Pos, 0);
#define GET_TOKEN(Prev, N)                                                                         \
    constexpr auto tok##N                                                                          \
        = detail::next_token(String.data(), tok##Prev.pos + tok##Prev.len, tok##Prev.eof_counter)
        GET_TOKEN(1, 2);
        GET_TOKEN(2, 3);
        GET_TOKEN(3, 4);
        GET_TOKEN(4, 5);
        GET_TOKEN(5, 6);
        GET_TOKEN(6, 7);
        GET_TOKEN(7, 8);
        GET_TOKEN(8, 9);
        GET_TOKEN(9, 10);
        GET_TOKEN(10, 11);
        GET_TOKEN(11, 12);
        GET_TOKEN(12, 13);
        GET_TOKEN(13, 14);
        GET_TOKEN(14, 15);
        GET_TOKEN(15, 16);
        GET_TOKEN(16, 17);
        GET_TOKEN(17, 18);
        GET_TOKEN(18, 19);
        GET_TOKEN(19, 20);
        GET_TOKEN(20, 21);
        GET_TOKEN(21, 22);
        GET_TOKEN(22, 23);
        GET_TOKEN(23, 24);
        GET_TOKEN(24, 25);
        GET_TOKEN(25, 26);
        GET_TOKEN(26, 27);
        GET_TOKEN(27, 28);
        GET_TOKEN(28, 29);
        GET_TOKEN(29, 30);
#undef GET_TOKEN
        using NewList = vlist<tok30,
                              tok29,
                              tok28,
                              tok27,
                              tok26,
                              tok25,
                              tok24,
                              tok23,
                              tok22,
                              tok21,
                              tok20,
                              tok19,
                              tok18,
                              tok17,
                              tok16,
                              tok15,
                              tok14,
                              tok13,
                              tok12,
                              tok11,
                              tok10,
                              tok9,
                              tok8,
                              tok7,
                              tok6,
                              tok5,
                              tok4,
                              tok3,
                              tok2,
                              tok1,
                              Acc...>;
        if constexpr (tok30.len == 0) {
            return meta::listptr<vlist<String, std::size_t(tok30.eof_counter)>, NewList>{};
        } else {
            return tokenizer::clang_tokenize_more(
                meta::listptr<vlist<String, tok30.pos + tok30.len>, NewList>{});
        }
    }

    template <auto String, auto NumEOFs, token_range... Token>
    constexpr static auto
        clang_finalize_tokens(meta::listptr<vlist<String, NumEOFs>, vlist<Token...>>)
            -> eof_trim_t<NumEOFs, list<vlist<fin_token(String.data() + Token.pos, Token.len)>...>>;

    template <auto S>
    static auto clang_tokenize() -> decltype(tokenizer::clang_finalize_tokens(
        tokenizer::clang_tokenize_more(meta::listptr<vlist<S, std::size_t(0)>, vlist<>>{})));

    template <auto String, std::size_t Pos, std::size_t NumEOFs, token_range... Tokens>
    static auto gcc_tokenize_more(
        meta::listptr<vlist<true, String, Pos, NumEOFs>, vlist<Tokens...>>)
        -> eof_trim_t<NumEOFs, list<vlist<fin_token(String.data() + Tokens.pos, Tokens.len)>...>>;

#define PARSE_TOKEN(Prev, N)                                                                         \
    token_range             Token##N = detail::next_token(String.data(), Pos##Prev, EOFCount##Prev), \
                std::size_t Pos##N   = Token##N.pos + Token##N.len,                                  \
                int         EOFCount##N = Token##N.eof_counter

    template <auto        String,
              std::size_t Pos0,
              int         EOFCount0 = 0,
              token_range... Tokens,
              typename Self = tokenizer,
              PARSE_TOKEN(0, 1),
              PARSE_TOKEN(1, 2),
              PARSE_TOKEN(2, 3),
              PARSE_TOKEN(3, 4),
              PARSE_TOKEN(4, 5),
              PARSE_TOKEN(5, 6)>
    static auto gcc_tokenize_more(
        meta::listptr<vlist<false, String, Pos0, std::size_t(0)>, vlist<Tokens...>>)
        -> decltype(Self::gcc_tokenize_more(
            meta::listptr<vlist<EOFCount6 != 0, String, Pos6, std::size_t(EOFCount6)>,
                          vlist<Token6, Token5, Token4, Token3, Token2, Token1, Tokens...>>{}));

#undef PARSE_TOKEN

    template <auto S, std::size_t Len = S.size(), std::size_t Zero = 0>
    static auto gcc_tokenize() -> decltype(tokenizer::gcc_tokenize_more(
        meta::listptr<vlist<false, S, Zero, Zero>, vlist<>>{}));
};

}  // namespace detail

// Both Clang and GCC can tokenize using either implementation, but one
// is significantly faster for each.
template <cx_str Str>
using tokenize_t =
#if defined __INTELLISENSE__
    token_list<token{"IDE Disables Tokenizing"}>
#elif defined __clang__
    decltype(detail::tokenizer::clang_tokenize<Str>())
#elif defined __GNUC__
    decltype(detail::tokenizer::gcc_tokenize<Str>())
#endif
    ;

}  // namespace lmno::lex
