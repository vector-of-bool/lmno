#pragma once

#include "./meta.hpp"
#include "./string.hpp"

#include <array>
#include <cassert>
#include <string_view>
#include <utility>

namespace lmno::lex {

constexpr static std::size_t max_token_length = 23;

/**
 * @brief Represents a LMNO token.
 *
 * Structural and valid as a non-type template parameter
 */
struct token {
    char _chars[max_token_length + 1] = {};

    constexpr token() = default;

    constexpr token(const char* s) {
        auto        dst = _chars;
        std::size_t n   = 0;
        while (*s) {
            ++n;
            assert(n <= max_token_length);
            *dst++ = *s++;
        }
    }

    constexpr operator std::string_view() const noexcept { return std::string_view(_chars); }

    constexpr const char* data() const noexcept { return _chars; }
    constexpr std::size_t size() const noexcept { return std::char_traits<char>::length(_chars); }
    constexpr char        operator[](std::size_t n) const noexcept {
        assert(n < max_token_length);
        return _chars[n];
    }

    bool operator==(const token&) const = default;
};

template <token... Tokens>
struct token_list;

constexpr bool is_digit(char c) { return c <= '9' and c >= '0'; }
constexpr bool is_alpha(char c) { return (c >= 'a' and c <= 'z') or (c >= 'A' and c <= 'Z'); }
constexpr bool is_ident(char c) { return is_alpha(c) or is_digit(c); }

namespace detail {

using meta::list;

constexpr auto fin_token(const char* s, std::size_t len) {
    token ret;
    for (auto i = 0u; i < len; ++i) {
        ret._chars[i] = s[i];
    }
    return ret;
}

/**
 * @brief Represents the position of a token within a larger string
 *
 */
struct token_range {
    /// The beginning offset of the token
    std::uint32_t pos;
    /// The length of the token (in char)
    std::uint8_t len;
};

/**
 * @brief Obtain the next token_range from the given string at the specified offset
 *
 * @param begin A pointer to the very beginning of the source string
 * @param begin_pos The offset within the string to begin searching for another token
 * @return constexpr token_range
 */
constexpr token_range next_token(const char* const begin, std::uint32_t start_pos) {
    auto it = begin + start_pos;
    // The beginning index of the token within the string view:
    std::uint32_t pos = start_pos;
    // Skip leading whitespace
    while (*it == ' ' or *it == '\n') {
        ++it;
        ++pos;
    }

    const char first = *it;
    if (first & 0b1000'0000) {
        if (*it) {
            auto b = static_cast<std::uint8_t>(first);
            switch ((b >> 4) & 0xf) {
            case 0b1100:
                if (it[1] == char(0xaf) and it[0] == char(0xc2)) {
                    // A leading hi-bar '¯', which introduces a negative literal
                    token_range peek = next_token(begin, pos + 2);
                    peek.pos         = pos;
                    peek.len += 2;
                    return peek;
                }
                [[fallthrough]];
            case 0b1101:
                return {pos, 2};
            case 0b1110:
                return {pos, 3};
            default:
                return {pos, 4};
            }
        }
    }

    // Regular ASCII char
    if (first >= 'A') {
        if (is_ident(first)) {
            // Ident
            ++it;
            std::uint8_t len = 1;
            while (is_ident(*it)) {
                ++it;
                ++len;
            }
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
            auto len = static_cast<std::uint8_t>(it - num_begin);
            return {pos, len};
        } else {
            // End case: Return an EOF
            if (not *it) {
                return {pos, 0};
            }
            if (first == '(' and it[1] == ':') {
                // We're in a comment
                it += 2;
                pos += 2;
                while (*it and (it[0] != ':' or it[1] != ')')) {
                    ++it;
                    ++pos;
                }
                if (not *it) {
                    throw "Unterminated comment in source string";
                }
                it += 2;
                pos += 2;
                return next_token(begin, pos);
            }
            // Other:
            return {pos, 1};
        }
    }
}

template <std::size_t N>
struct tokenize_result {
    std::array<token_range, N> tokens;
    int                        num_tokens;
};

template <std::size_t N>
constexpr tokenize_result<N> tokenize(const char* _chars) {
    tokenize_result<N> ret = {};
    ret.num_tokens         = tokenize(ret.tokens.data(), _chars);
    return ret;
}

constexpr int tokenize(token_range* into, char const* const cptr) {
    int           n_tokens = 0;
    std::uint32_t off      = 0;
    while (1) {
        auto& tr = *into++ = detail::next_token(cptr, off);
        if (tr.len == 0) {
            break;
        }
        ++n_tokens;
        off = tr.pos + tr.len;
    }
    return n_tokens;
}

constexpr token take_token(const char* str_begin, token_range const& r) {
    return detail::fin_token(str_begin + r.pos, r.len);
}

template <cx_str String, tokenize_result Result, std::size_t... I>
auto prune_f(std::index_sequence<I...>*)
    -> token_list<take_token(String.data(), Result.tokens[I])...>;

template <cx_str String>
auto tokenize() {
    constexpr auto res = tokenize<String.size() + 1>(String.data());
    return meta::ptr<decltype(detail::prune_f<String, res>(
        (std::make_index_sequence<res.num_tokens>*)(nullptr)))>{};
}

}  // namespace detail

template <cx_str String>
using tokenize_t = neo::remove_pointer_t<decltype(detail::tokenize<String>())>;

}  // namespace lmno::lex
