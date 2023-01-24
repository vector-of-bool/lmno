#pragma once

#include <type_traits>

namespace lmno {

/**
 * @brief Match a type that is "structural". Such a type can be used as a non-type template
 * argument.
 */
template <typename T>
concept structural = requires { []<T V>(std::integral_constant<T, V>*) {}; };

}  // namespace lmno
