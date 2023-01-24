#pragma once

#include "./stateless.hpp"
#include "./structural.hpp"

#include <neo/concepts.hpp>

namespace lmno {

struct typed_constant_base {};

template <typename T>
constexpr bool enable_typed_constant_v
    = requires { requires static_cast<bool>(T::enable_typed_constant); };

/**
 * @brief Match any object which declares itself to be a typed constant and
 * satisfies certain constraints
 *
 * A typed-consteant must inherit from typed_constant_base, or specialize
 * enable_typed_constant_v to `true`. It must also expose a static member variable
 * `::value` and a member type `type`. The nested `::type` must be a structural type.
 *
 * The type itself must be stateless.
 */
template <typename T>
concept typed_constant =                                                       //
    (neo::derived_from<T, typed_constant_base> or enable_typed_constant_v<T>)  //
    and stateless<T>                                                           //
    and requires(T c) {
            // It must declare its type
            typename T::type;
            // That type must be valid as a non-type template parameter
            requires structural<typename T::type>;
            // ::value must actually be of that type
            { T::value } -> std::same_as<const typename T::type&>;
            // We should be able to static_cast to the underlying type:
            static_cast<typename T::type>(c);
        };

/**
 * @brief Match any type that isn't a typed_constant
 *
 * @tparam T
 */
template <typename T>
concept variate = (not typed_constant<T>);

namespace detail {

template <bool IsTypedConstant>
struct unconst {
    template <typename T>
    using f = T;
};

template <>
struct unconst<true> {
    template <typename T>
    using f = neo::remove_cvref_t<T>::type const&;
};

}  // namespace detail

template <typename T>
using unconst_t = detail::unconst<typed_constant<neo::remove_cvref_t<T>>>::template f<T>;

template <typename T>
constexpr unconst_t<T> unconst(T&& arg) noexcept {
    return static_cast<unconst_t<T&&>>(arg);
}

/**
 * @brief Match a typed_constant type that has an integral value
 */
template <typename T>
concept integral_typed_constant = typed_constant<T> and neo::integral<typename T::type>;

}  // namespace lmno
