#pragma once

#include "./concepts/typed_constant.hpp"

#include <neo/fwd.hpp>
#include <neo/iterator_facade.hpp>

#include <concepts>
#include <tuple>

namespace lmno {

struct strand_range_construct_tag_t {};

/**
 * @brief A range constructed from‿a‿strand‿expression
 *
 * @tparam Ts The types of the strand elements. Must share a common type
 */
template <typename... Ts>
    requires requires { typename std::common_type_t<unconst_t<Ts>...>; }
class strand_range {
    using tuple_type = std::tuple<Ts...>;
    NEO_NO_UNIQUE_ADDRESS tuple_type _tpl;

    using _ref = std::common_reference_t<neo::const_reference_t<unconst_t<Ts>>...>;

public:
    strand_range() = default;

    template <std::convertible_to<Ts>... Us>
    constexpr strand_range(strand_range_construct_tag_t, Us&&... us)
        : _tpl(NEO_FWD(us)...) {}

    class _iter : public neo::iterator_facade<_iter> {
        friend strand_range;
        const tuple_type* _tuple = nullptr;
        std::ptrdiff_t    _idx   = 0;

        constexpr explicit _iter(tuple_type const& t, std::size_t offset) noexcept
            : _tuple(&t)
            , _idx(static_cast<std::ptrdiff_t>(offset)) {}

        template <std::size_t N>
        constexpr _ref _get_nth() const noexcept {
            if constexpr (N == sizeof...(Ts)) {
                std::terminate();
            } else if (N == _idx) {
                return static_cast<_ref>(std::get<N>(*_tuple));
            } else {
                return this->_get_nth<N + 1>();
            }
        }

    public:
        _iter() = default;

        constexpr _ref dereference() const noexcept { return _get_nth<0>(); }

        constexpr std::ptrdiff_t distance_to(_iter other) const noexcept {
            return other._idx - _idx;
        }

        constexpr void advance(std::ptrdiff_t off) noexcept { _idx += off; }

        constexpr bool operator==(const _iter& o) const noexcept {
            return _tuple == o._tuple and _idx == o._idx;
        }
    };

    constexpr auto begin() const noexcept { return _iter(_tpl, 0); }
    constexpr auto end() const noexcept { return _iter(_tpl, sizeof...(Ts)); }

    constexpr _ref operator[](std::size_t pos) const noexcept { return begin()[pos]; }
};

template <typename... Ts>
strand_range(strand_range_construct_tag_t, const Ts&...) -> strand_range<Ts...>;

namespace detail {

template <typename S>
constexpr bool is_constant_strand_range = false;

template <typed_constant... Cs>
constexpr bool is_constant_strand_range<strand_range<Cs...>> = true;

}  // namespace detail

template <typename S>
concept constant_strand_range = detail::is_constant_strand_range<std::remove_cvref_t<S>>;

}  // namespace lmno
