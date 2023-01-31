#pragma once

#include <string_view>

namespace lmno {

/**
 * @brief A compile-time string of length N
 *
 * Use with CTAD on a string literal to deduce the string length. Otherwise, the
 * length must be given explicitly as a constexpr expression.
 *
 * @tparam N The number of characters in the string (NOT including a null terminator)
 */
template <std::size_t N>
struct cx_str {
    char chars[N + 1] = {0};

    cx_str()              = default;
    cx_str(const cx_str&) = default;

    // Copy from an array
    constexpr cx_str(const char (&arr)[N + 1]) noexcept {
        auto src = arr;
        auto dst = chars;
        for (auto i = 0u; i < N; ++i) {
            dst[i] = src[i];
        }
    }

    // Copy from a string_view
    constexpr explicit cx_str(std::string_view s) noexcept {
        auto dst = chars;
        auto src = s.data();
        for (auto i = 0u; i < N; ++i) {
            dst[i] = src[i];
        }
    }

    // Implicit convert to a string_view
    constexpr operator std::string_view() const noexcept { return std::string_view(chars, N); }

    // Obtain a pointer to the string data. (null terminated)
    constexpr char*       data() noexcept { return chars; }
    constexpr const char* data() const noexcept { return chars; }
    // Get the size of the string
    constexpr std::size_t size() const noexcept { return N; }

    // Default-compare
    constexpr bool operator==(const cx_str&) const noexcept = default;

    // Compare with anything else that is convertible to a string_view
    template <std::convertible_to<std::string_view> S>
    constexpr bool operator==(S&& s) const noexcept {
        return std::string_view(*this) == std::string_view(s);
    }

    template <std::size_t O>
    constexpr cx_str<O + N> operator+(const cx_str<O>& other) const noexcept {
        cx_str<O + N> ret;
        auto          o = ret.chars;
        for (auto i = 0u; i < N; ++i, ++o) {
            *o = this->chars[i];
        }
        for (auto i = 0u; i < O; ++i, ++o) {
            *o = other.chars[i];
        }
        return ret;
    }

    template <std::size_t O>
    constexpr cx_str<N + O - 1> operator+(const char (&arr)[O]) const noexcept {
        return *this + lmno::cx_str{arr};
    }

    template <std::size_t O>
    constexpr friend cx_str<N + O - 1> operator+(const char (&arr)[O],
                                                 const cx_str& self) noexcept {
        return lmno::cx_str{arr} + self;
    }

    constexpr char operator[](std::size_t off) const noexcept { return chars[off]; }
};

template <std::size_t N>
cx_str(const char (&)[N]) -> cx_str<N - 1>;

template <std::size_t N>
constexpr std::size_t cx_string_size(const char (&)[N]) {
    return N - 1;
}

template <std::size_t N>
constexpr std::size_t cx_string_size(cx_str<N> const&) {
    return N;
}

template <typename T>
concept cx_sized_string = std::default_initializable<T>  //
    and requires {
            { cx_string_size(T{}) } -> std::integral;
        };

namespace detail {

template <cx_str>
struct err {};

template <cx_str S, template <auto> class E = err>
auto gen_error() {
    return err<S>();
}

constexpr char* str_copy(char* dest, const char* in, int len = -1) {
    while (*in && len != 0) {
        *dest++ = *in++;
        --len;
    }
    return dest;
}

constexpr std::size_t calc_fmt_size(const char* in, const auto&... nitems) {
    std::size_t l = 0;
    while (*in) {
        if (in[0] == '{') {
            if (in[1] == '{') {
                ++l;
                in += 2;
            } else {
                // A format item.
                ++in;
                if (*in == '}') {
                    // Simple item
                    ++in;
                } else if (*in != ':') {
                    gen_error<
                        "Invalid format string (Expected ':' or '}' following opening brace)">();
                } else {
                    // There are format specs
                    ++in;
                    while (*in != '}') {
                        if (*in == '\'') {
                            // We're quoting this item. That's six 'char'
                            l += 6;
                            ++in;
                        } else {
                            gen_error<"Unsupported special character in format specifier\n\n">();
                        }
                    }
                    ++in;
                }
            }
        } else if (*in == '}') {
            if (in[1] != '}') {
                gen_error<"Closing curly brace without an opening brace. (Did you mean '}}'?)">();
            }
            ++l;
            in += 2;
        } else {
            ++l;
            ++in;
        }
    }

    return l + (cx_string_size(nitems) + ... + 0);
}

constexpr void format_another(char*& out, const char*& in, const auto& item) {
    for (;;) {
        if (*in == '}') {
            *out++ = '}';
            in += 2;
        } else if (*in != '{') {
            *out++ = *in++;
        } else {
            // A format specifier here
            ++in;
            if (*in == '{') {
                // Actually, its two braces. False alarm
                *out++ = '{';
                ++in;
            } else if (*in == '}') {
                // A simple format specifier
                out = detail::str_copy(out,
                                       std::data(item),
                                       static_cast<int>(cx_string_size(item)));
                ++in;
                return;
            } else {
                // A specifier with options
                ++in;
                bool enquote = false;
                while (*in != '}') {
                    if (*in == '\'') {
                        // Wrap quotes around the item
                        enquote = true;
                        ++in;
                    } else {
                        gen_error<"Invalid format specifier??">();
                    }
                }
                if (enquote) {
                    out = detail::str_copy(out, "‘");
                }
                out = detail::str_copy(out,
                                       std::data(item),
                                       static_cast<int>(cx_string_size(item)));
                if (enquote) {
                    out = detail::str_copy(out, "’");
                }
                ++in;
                return;
            }
        }
    }
}

constexpr std::size_t
calc_replace_size(std::string_view in, std::string_view find, std::size_t replace_size) {
    if (find.size() == 0) {
        gen_error<"The 'find' argument cannot be an empty string">();
    }
    std::size_t pos = 0;
    std::size_t acc = 0;
    while (true) {
        auto new_pos = in.find(find, pos);
        if (new_pos == in.npos) {
            // Add the remainder:
            acc += in.size() - pos;
            break;
        } else {
            // Add the amount we skipped to find it:
            acc += new_pos - pos;
            // Add the number of characters that we are inserting:
            acc += replace_size;
        }
        // Move the search pos to past the replaced string:
        pos = new_pos + find.size();
    }
    return acc;
}

}  // namespace detail

template <cx_str Fmt, cx_sized_string... SizedStrings>
constexpr auto cx_fmt(const SizedStrings&... strings) {
    cx_str<detail::calc_fmt_size(Fmt.data(), SizedStrings{}...)> ret;
    const char*                                                  in  = Fmt.data();
    char*                                                        out = ret.data();
    (detail::format_another(out, in, strings), ...);
    while (*in) {
        *out++ = *in++;
        if (*in == '{' or *in == '}') {
            ++in;
        }
    }
    return ret;
}

template <cx_str Fmt, auto... SizedStrings>
    requires(cx_sized_string<decltype(SizedStrings)> and ...)
constexpr auto cx_fmt_v = cx_fmt<Fmt>(SizedStrings...);

template <cx_str Joiner, auto... SizedStrings>
constexpr auto cx_str_join_v = nullptr;

template <cx_str Joiner>
constexpr auto cx_str_join_v<Joiner> = cx_str{""};

template <cx_str Joiner, auto First, auto... SizedStrings>
    requires(cx_sized_string<decltype(First)>) and (cx_sized_string<decltype(SizedStrings)> and ...)
constexpr auto cx_str_join_v<Joiner, First, SizedStrings...>
    = cx_fmt_v<"{}{}", First, (cx_fmt_v<"{}{}", Joiner, SizedStrings> + ... + cx_str{""})>;

template <auto S>
    requires cx_sized_string<decltype(S)>
constexpr auto quote_str_v = cx_fmt_v<"‘{}’", S>;

template <cx_str S, auto Find, auto Replace>
    requires(cx_sized_string<decltype(Find)> and cx_sized_string<decltype(Replace)>)
constexpr auto cx_str_replace() {
    constexpr auto newlen = detail::calc_replace_size(std::string_view(S),
                                                      std::string_view(Find),
                                                      cx_string_size(Replace));
    cx_str<newlen> ret;
    std::size_t    pos = 0;
    auto           in  = std::string_view(S);
    char*          out = ret.data();
    while (1) {
        auto new_pos = in.find(std::string_view(Find), pos);
        if (new_pos == in.npos) {
            // Copy the remainder
            out = detail::str_copy(out, in.data() + pos, -1);
            break;
        } else {
            // Copy up to the found item:
            out = detail::str_copy(out, in.data() + pos, static_cast<int>(new_pos - pos));
            // Copy the replacment item
            out = detail::str_copy(out, std::string_view(Replace).data(), cx_string_size(Replace));
        }
        pos = new_pos + cx_string_size(Find);
    }
    return ret;
}

#define LMNO_CX_STR(S)                                                                             \
    ::lmno::cx_str<(S).size()> { (S) }

}  // namespace lmno
