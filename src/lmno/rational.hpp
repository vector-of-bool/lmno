#pragma once

#include <cstdint>
#include <numeric>

namespace lmno {

/**
 * @brief A rational number, represented using two integers
 */
class rational {
public:
    // Public to permit use in non-type template parameters, but theoretically private
    std::int64_t _priv_numerator;
    // Public to permit use in non-type template parameters, but theoretically private
    std::int64_t _priv_denominator;

private:
    // Delegated constructor that normalizes the values
    constexpr rational(std::int64_t gcd, std::int64_t n, std::int64_t d) noexcept
        : _priv_numerator(n / gcd)
        , _priv_denominator(d / gcd) {}

public:
    /**
     * @brief Construct a rational number as the ratio of the two given integers.
     *
     * Will be normalized into it's most-reduces form, and the denominator will
     * always be positive, i.e. rational{2, -4} == rational{-9, 18}
     */
    [[nodiscard]] constexpr explicit rational(std::int64_t n, std::int64_t d) noexcept
        : rational(std::gcd(n, d), n, d) {}

    /**
     * @brief Create a rational number equivalent to the given integer.
     *
     * The denominator will be 1
     */
    [[nodiscard]] constexpr rational(std::integral auto n) noexcept
        : _priv_numerator{static_cast<std::int64_t>(n)}
        , _priv_denominator{1} {}

    /// The numerator of the rational number
    [[nodiscard]] constexpr std::int64_t numerator() const noexcept { return _priv_numerator; }
    /// The denominator of the rational number
    [[nodiscard]] constexpr std::int64_t denominator() const noexcept { return _priv_denominator; }

    /// Compare two rationals
    [[nodiscard]] bool operator==(const rational&) const = default;

    // Add two rational numbers together
    [[nodiscard]] constexpr rational operator+(rational other) const noexcept {
        const auto new_d = std::lcm(_priv_denominator, other._priv_denominator);
        const auto lfac  = new_d / _priv_denominator;
        const auto rfac  = new_d / other._priv_denominator;
        const auto lnum  = _priv_numerator * lfac;
        const auto rnum  = other._priv_numerator * rfac;
        return rational{static_cast<std::int64_t>(lnum + rnum), new_d};
    }

    // Swap the sign of the numerator
    [[nodiscard]] constexpr rational operator-() const noexcept {
        return rational{-_priv_numerator, _priv_denominator};
    }

    // Subtract two rationals
    [[nodiscard]] constexpr rational operator-(rational other) const noexcept {
        return *this + -(other);
    }

    // Multiply two rationals together
    [[nodiscard]] constexpr rational operator*(rational other) const noexcept {
        const auto n = _priv_numerator * other._priv_numerator;
        const auto d = _priv_denominator * other._priv_denominator;
        // The constructor will reduce the terms, if necessary:
        return rational{n, d};
    }

    // Divide two rational numbers
    [[nodiscard]] constexpr rational operator/(rational other) const {
        return *this * other.recip();
    }

    /**
     * @brief Obtain the reciprocal of two rational numbers
     *
     * @return constexpr rational
     */
    [[nodiscard]] constexpr rational recip() const {
        return rational{_priv_denominator, _priv_numerator};
    }

    // Math with other types will promote
    // +
    [[nodiscard]] friend constexpr rational operator+(rational q, std::integral auto n) noexcept {
        return q + rational{n};
    }
    [[nodiscard]] friend constexpr rational operator+(std::integral auto n, rational q) noexcept {
        return rational{n} + q;
    }
    // -
    [[nodiscard]] friend constexpr rational operator-(rational q, std::integral auto n) noexcept {
        return q - rational{n};
    }
    [[nodiscard]] friend constexpr rational operator-(std::integral auto n, rational q) noexcept {
        return rational{n} - q;
    }
    // ×
    [[nodiscard]] friend constexpr rational operator*(rational q, std::integral auto n) noexcept {
        return q * rational{n};
    }
    [[nodiscard]] friend constexpr rational operator*(std::integral auto n, rational q) noexcept {
        return rational{n} * q;
    }
    // ÷
    [[nodiscard]] friend constexpr rational operator/(rational q, std::integral auto n) noexcept {
        return q / rational{n};
    }
    [[nodiscard]] friend constexpr rational operator/(std::integral auto n, rational q) noexcept {
        return rational{n} / q;
    }
};

constexpr rational operator""_Q(unsigned long long n) noexcept { return rational{n}; }
constexpr rational operator""_Qr(unsigned long long n) noexcept { return rational{n}.recip(); }

}  // namespace lmno

template <std::integral I>
struct std::common_type<lmno::rational, I> {
    using type = lmno::rational;
};

template <std::integral I>
struct std::common_type<I, lmno::rational> {
    using type = lmno::rational;
};