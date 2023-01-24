#pragma once

#include "./string.hpp"

#include <neo/concepts.hpp>

#include <concepts>
#include <type_traits>

namespace lmno::err {

/// Base class of all errors, used to detect whether a type is an error
struct error_base {};

/**
 * @brief Class template that encloses an error message, as well as an optional
 * child error
 *
 * @tparam Message The message string that represents the error
 * @tparam Child A child error that provides additional context
 */
template <cx_str Message, typename Child = void>
struct [[nodiscard]] error_type : error_base {
    // The child type associated with this error, or void
    using child = Child;

    // The error message string associated with this error
    static constexpr const auto& message = Message;
};

/**
 * @brief Construct an error object using a string format message
 */
template <cx_str Fmt, auto... Items>
    requires(lmno::cx_sized_string<decltype(Items)> and ...)
constexpr auto make_error() {
    constexpr auto s = lmno::cx_fmt_v<Fmt, Items...>;
    return error_type<s>{};
}

/**
 * @brief Construct an error object using a string format message, with the given
 * child error type
 */
template <typename Child, cx_str Fmt, auto... Items>
    requires(lmno::cx_sized_string<decltype(Items)> and ...)
constexpr auto make_error() {
    constexpr auto s = lmno::cx_fmt_v<Fmt, Items...>;
    return error_type<s, Child>{};
}

template <cx_str Fmt, auto... Items>
    requires(lmno::cx_sized_string<decltype(Items)> and ...)
using fmt_error_t = decltype(make_error<Fmt, Items...>());

template <typename Child, cx_str Fmt, auto... Items>
    requires(lmno::cx_sized_string<decltype(Items)> and ...)
using fmt_errorex_t = decltype(make_error<Child, Fmt, Items...>());

}  // namespace lmno::err

namespace lmno {

/**
 * @brief Match any cvr-qualified type that represents a compile-time error
 */
template <typename T>
concept any_error = neo::derived_from<neo::remove_cvref_t<T>, err::error_base>;

/**
 * @brief Determinate whether the given named value designates an error
 */
#define LMNO_IS_ERROR(...) (::lmno::any_error<decltype(__VA_ARGS__)>)

/**
 * @brief Match any type that is not a cvr-qualified compile-time error
 */
template <typename T>
concept non_error = (not any_error<T>);

}  // namespace lmno
