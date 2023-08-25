#pragma once

#include <neo/concepts.hpp>

#include <ranges>

#include "./shape.hpp"

namespace lmno::md {

namespace detail {

template <typename Off, std::ptrdiff_t... Ns>
    requires requires(void (&fn)(const Off&)) {
                 // Check: We can construct it using N offsets
                 Off{Ns...};
                 fn({Ns...});
                 // Check: Default-construction is equivalent to initializing it with all zero
                 requires(Off{} == Off{(Ns - Ns)...});
             }
void check_cursor_construct(std::integer_sequence<std::ptrdiff_t, Ns...>);

}  // namespace detail

/**
 * @brief A type that can be used as the offset for a multi-dimensional cursor.
 *
 * Requires a static rank(), and must be constructible using rank()-count of std::ptrdiff_t, or
 * default-constructed..
 *
 * @tparam T
 */
template <typename T>
concept offset = neo::regular<T>  //
    and requires(T off, std::ptrdiff_t pos) {
            requires(T::rank() >= 0);
            off[1] = pos;
            /**
             * Require that, for a pack Ns... of rank() ptrdiff_t:
             *  • Valid: T{Ns...}
             *  • Valid: T{}
             *  • Valid: T o = {Ns...};
             *  • Require: T{} == T{(Ns-Ns)...}
             */
            detail::check_cursor_construct<T>(
                std::make_integer_sequence<std::ptrdiff_t, T::rank()>{});
        };

template <typename C>
using cursor_offset_t = decltype(NEO_DECLVAL(neo::const_reference_t<C>)
                                     .difference(NEO_DECLVAL(neo::const_reference_t<C>)));

template <typename C>
using cursor_reference_t = decltype(NEO_DECLVAL(neo::const_reference_t<C>).get());

// clang-format off
template <typename C>
concept cursor =
        neo::semiregular<C>
    and requires {
            typename cursor_offset_t<C>;
            requires offset<cursor_offset_t<C>>;
        }
    and requires(neo::const_reference_t<C> c, cursor_offset_t<C> off) {
            { c.adjust(off) } -> neo::weak_same_as<C>;
            requires neo_is_referencable(cursor_reference_t<C>);
        };

template <typename C, typename T>
concept output_cursor =
        cursor<C>
    and neo::assignable_from<cursor_reference_t<C>, T>;
// clang-format on

template <std::size_t R>
struct basic_offset {
    std::ptrdiff_t _coords[R] = {};

    static constexpr std::size_t rank() noexcept { return R; }

    inline constexpr std::ptrdiff_t const& operator[](std::size_t nth) const noexcept {
        return _coords[nth];
    }

    inline constexpr std::ptrdiff_t& operator[](std::size_t nth) noexcept { return _coords[nth]; }

    bool operator==(const basic_offset&) const = default;
};

template <std::integral... Is>
explicit basic_offset(Is...) -> basic_offset<sizeof...(Is)>;

template <std::ranges::view T>
struct range_cursor {
    using _iterator = std::ranges::iterator_t<T>;

    NEO_NO_UNIQUE_ADDRESS _iterator _iter;

    range_cursor() = default;

    constexpr explicit range_cursor(T view) noexcept
        : _iter(std::ranges::begin(view)) {}

    constexpr explicit range_cursor(_iterator i) noexcept
        : _iter(i) {}

    constexpr decltype(auto) get() const noexcept { return *_iter; }

    constexpr range_cursor adjust(basic_offset<1> off) const noexcept {
        return range_cursor{_iter + off[0]};
    }

    constexpr basic_offset<1> difference(range_cursor other) const noexcept {
        return basic_offset<1>{other._iter - _iter};
    }
};

template <std::ranges::view V>
explicit range_cursor(V) -> range_cursor<V>;

template <typename Array>
class c_array_cursor {
public:
    static constexpr std::size_t rank() noexcept { return std::rank_v<Array>; }

private:
    using _offset = basic_offset<rank()>;
    using _iseq   = std::make_index_sequence<rank()>*;
    using _ref    = std::remove_all_extents_t<Array>&;

public:
    c_array_cursor() = default;

    constexpr explicit c_array_cursor(Array& arr) noexcept
        : _ptr(arr) {}

    constexpr _offset difference(c_array_cursor const& other) const noexcept {
        return _make_diff(_pos, other._pos, _iseq{});
    }

    constexpr _ref get() const noexcept { return _get(_ptr, _pos, _iseq{}); }

    constexpr c_array_cursor adjust(_offset by) const noexcept { return _adjust(by, _iseq{}); }

private:
    using _pointer = neo::decay_t<Array>;

    constexpr explicit c_array_cursor(_pointer p, _offset o) noexcept
        : _ptr(p)
        , _pos(o) {}

    template <std::size_t... Idx>
    constexpr static _offset
    _make_diff(_offset const& self, _offset const& other, std::index_sequence<Idx...>*) noexcept {
        return _offset{(self[Idx] - other[Idx])...};
    }

    template <std::size_t... Idx>
    constexpr static _ref _get(_pointer ptr, _offset pos, std::index_sequence<Idx...>*) noexcept {
        return _deref(ptr, pos[Idx]...);
    }

    template <std::size_t... Idx>
    constexpr c_array_cursor _adjust(_offset by, std::index_sequence<Idx...>*) const noexcept {
        return c_array_cursor{_ptr, _offset{_pos[Idx] + by[Idx]...}};
    }

    constexpr static _ref _deref(auto ptr, auto off, auto... more) noexcept {
        return _deref(ptr[off], more...);
    }

    constexpr static _ref _deref(_ref r) noexcept { return r; }

public:  // Public allows us to be a structural type
    _pointer _ptr = nullptr;
    _offset  _pos;
};

template <typename Array>
    requires detail::bounded_md_carray_v<Array>
explicit c_array_cursor(const Array&) -> c_array_cursor<Array>;

/**
 * @brief Wraps a cursor object with a convenience API
 *
 * @tparam T A cursor type to wrap
 */
template <cursor T>
struct augmented_cursor {
    using offset_type = cursor_offset_t<T>;

    T _cursor;

    /**
     * @brief Obtain the element referenced by the cursor's current location
     *
     * @return constexpr decltype(auto)
     */
    inline constexpr cursor_reference_t<T> operator*() const noexcept { return _cursor.get(); }

    /**
     * @brief Obtain a new augmented cursor adjusted by the given offset
     */
    inline constexpr augmented_cursor operator+(offset_type off) const noexcept {
        return augmented_cursor{_cursor.adjust(off)};
    }

    /**
     * @brief Adjust the cursor with the given offset
     */
    inline constexpr augmented_cursor& operator+=(offset_type off) const noexcept {
        return *this = *this + off;
    }

    inline constexpr cursor_reference_t<T> operator[](offset_type off) const noexcept {
        return *(*this + off);
    }
};

template <typename T>
explicit augmented_cursor(T) -> augmented_cursor<T>;

}  // namespace lmno::md
