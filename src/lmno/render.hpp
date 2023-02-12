#pragma once

#include "./const.hpp"
#include "./rational.hpp"
#include "./string.hpp"

#if __has_include(<nameof.hpp>)
#include <nameof.hpp>
#endif

#include <concepts>
#include <initializer_list>
#include <vector>

#define cx constexpr

namespace lmno {

namespace render {

template <typename I>
cx cx_str literal_suffix_v = "";

template <>
cx cx_str literal_suffix_v<unsigned> = "u";

template <>
cx cx_str literal_suffix_v<unsigned long> = "ul";

template <>
cx cx_str literal_suffix_v<unsigned long long> = "ull";

template <>
cx cx_str literal_suffix_v<long> = "l";

template <>
cx cx_str literal_suffix_v<long long> = "ll";

namespace detail {

// Calculate the number of digits required to represent an integer
template <typename I>
constexpr std::size_t n_chars_required(I v) {
    if (v == 0) {
        return 1;
    }
    if (v < 0) {
        // We need to char (UTF code units) to add the negative high-bar
        return 2 + n_chars_required(-v);
    }
    std::size_t r = 0;
    while (v) {
        ++r;
        v /= 10;
    }
    return r;
}

// Render an integral value. Negative numbers are rendered with a high-bar "¯"
template <auto Value>
constexpr auto render_integral() noexcept {
    constexpr auto    NumDigits = n_chars_required(Value);
    cx_str<NumDigits> string;
    auto              out = string.data() + NumDigits - 1;
    if (Value == 0) {
        *out = '0';
    }
    ++out;
    for (auto val = Value < 0 ? -Value : Value; val; val /= 10) {
        --out;
        auto mod = val % 10;
        *out     = char('0' + mod);
    }
    if (Value < 0) {
        // Add "¯" to the front
        *--out = char(0xaf);
        *--out = char(0xc2);
    }
    return string;
}

// Use nameof to extract the name of the type, if possible
template <typename T>
cx auto type_default() noexcept {
#ifdef NAMEOF_TYPE
    cx auto name = NAMEOF_TYPE(T);
    return cx_str<name.size()>(name);
#else
    return cx_str{"[no-nameof]"};
#endif
}

// Obtains the default rendering of a type, without any type-specific customizations
template <typename T>
cx cx_str type_default_v = type_default<T>();

template <typename T>
cx auto get_template_name() {
    cx auto const& n        = type_default_v<T>;
    cx std::string_view sv  = n;
    cx auto             pos = sv.find("<");
    if cx (pos == sv.npos) {
        return n;
    } else {
        return cx_str<pos>(sv.substr(0, pos));
    }
}

}  // namespace detail

/**
 * @brief Obtain a compile-time string rendering of the given type.
 *
 * By default, renders an implementation-defined string representation of the
 * type, if nameof is available. Types can specialize this to override the
 * default appearance of their rendering.
 */
template <typename T>
cx cx_str type_v = detail::type_default_v<T>;

/**
 * @brief Obtain a rendering of the template-ID of a template specialization type
 */
template <typename T>
constexpr cx_str template_of_v = detail::get_template_name<T>();

/**
 * @brief Obtain a rendering of the value of a type.
 *
 * @tparam T The type of the value being rendered
 * @tparam Value The actual value to render
 *
 * @note Specialize this template variable for your own types.
 */
template <typename T, T Value>
constexpr cx_str value_of_type_v = cx_fmt_v<"[Unsupported value-render for type {:'}]", type_v<T>>;

/**
 * @brief Obtain a rendering of teh given integral value (without a literal suffix)
 */
template <auto V>
    requires std::integral<decltype(V)>
cx cx_str integer_v = detail::render_integral<V>();

// Specialize rendering of rational numbers
template <rational R>
cx cx_str value_of_type_v<rational, R>
    = cx_fmt_v<"({}÷{}):ℚ", integer_v<R.numerator()>, integer_v<R.denominator()>>;

// Specialize rendering of integers to show the value as well as a literal
// suffix for its type
template <std::integral I, I Value>
constexpr cx_str value_of_type_v<I, Value>
    = cx_fmt_v<"{}{}", integer_v<Value>, literal_suffix_v<I>>;

/**
 * @brief Render a constant value
 *
 * @tparam V The value that will be rendered.
 *
 * @note Do not specialize this variable template. Prefer to specialize value_of_type_v<T, V>
 * instead.
 */
template <auto V>
constexpr cx_str value_v = value_of_type_v<std::remove_const_t<decltype(V)>, V>;

// Specialize for const:
template <typename T>
cx cx_str type_v<const T> = cx_fmt_v<"{} const", type_v<T>>;

// Specialize for lvalue-ref
template <typename T>
cx cx_str type_v<T&> = cx_fmt_v<"{}&", type_v<T>>;

// Specialize for rvalue-ref
template <typename T>
cx cx_str type_v<T&&> = cx_fmt_v<"{}&&", type_v<T>>;

// Specialize for pointers
template <typename T>
cx cx_str type_v<T*> = cx_fmt_v<"{}*", type_v<T>>;

// Normalize the spellings of the builtins
#define DECL_RENDERING(T)                                                                          \
    template <>                                                                                    \
    constexpr cx_str type_v<T> = #T
DECL_RENDERING(int);
DECL_RENDERING(unsigned int);
DECL_RENDERING(short);
DECL_RENDERING(unsigned short);
DECL_RENDERING(long);
DECL_RENDERING(unsigned long);
DECL_RENDERING(long long);
DECL_RENDERING(unsigned long long);
DECL_RENDERING(bool);
DECL_RENDERING(float);
DECL_RENDERING(double);
DECL_RENDERING(void);
DECL_RENDERING(signed char);
DECL_RENDERING(unsigned char);
DECL_RENDERING(char);
DECL_RENDERING(char8_t);
DECL_RENDERING(char16_t);
DECL_RENDERING(char32_t);
DECL_RENDERING(wchar_t);
#undef DECL_RENDERING

// Grab any rendering of a class-template specialization:
template <template <class...> class L, typename... Args>
constexpr auto type_v<L<Args...>>
    = cx_fmt_v<"{}<{}>", template_of_v<L<Args...>>, cx_str_join_v<", ", type_v<Args>...>>;

// Grab class-templates that are specialized on values:
template <template <auto...> class L, auto... V>
constexpr auto type_v<L<V...>>
    = cx_fmt_v<"{}<{}>", template_of_v<L<V...>>, cx_str_join_v<", ", value_v<V>...>>;

// Hide the default allocator in a std::vector
template <typename T>
cx cx_str type_v<std::vector<T, std::allocator<T>>> = cx_fmt_v<"std::vector<{}>", type_v<T>>;

// Pretty-ify std::string
template <>
cx cx_str type_v<std::string> = "std::string";

// Pretty-ify std::wstring
template <>
cx cx_str type_v<std::wstring> = "std::wstring";

// Pretty-ify std::u8string
template <>
cx cx_str type_v<std::u8string> = "std::u8string";

// Pretty-ify std::u16string
template <>
cx cx_str type_v<std::u16string> = "std::u16string";

// Pretty-ify std::u32string
template <>
cx cx_str type_v<std::u32string> = "std::u32string";

template <typename C>
cx cx_str type_v<std::basic_string<C, std::char_traits<C>, std::allocator<C>>>
    = cx_fmt_v<"std::basic_string<{}>", type_v<C>>;

// Grab initializer_list
template <typename T>
cx cx_str type_v<std::initializer_list<T>> = cx_fmt_v<"init-list<{}>", type_v<T>>;

// Prettier Const<> to display both the value and the type
template <auto V>
cx cx_str type_v<Const<V>>
    = cx_fmt_v<"(Constant {:'}: {})", type_v<typename Const<V>::type>, value_v<V>>;

}  // namespace render

}  // namespace lmno

#undef cx
