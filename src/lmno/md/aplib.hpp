#pragma once

#include "./range.hpp"
#include "cursor.hpp"

#include <neo/attrib.hpp>
#include <ranges>

namespace lmno::md {

// clang-format off
template <typename C>
concept mdrange_adaptable_container =
        std::ranges::forward_range<C>
    and std::ranges::sized_range<C>
    and std::ranges::output_range<C, std::ranges::range_value_t<C>>
    and requires(C& mref,
                 const std::remove_reference_t<C>& cref,
                 std::ranges::range_size_t<C> size,
                 std::ranges::range_difference_t<C> offset,
                 std::ranges::range_reference_t<C> ref,
                 std::ranges::range_value_t<C> const& value) {
            // clang-format on
            { mref.resize(size) };
            { mref.resize(size, value) };
            // clang-format off
        };
// clang-format on

template <mdrange_adaptable_container Container, typename Extents>
class mdarray_adaptor {
public:
    using container_type  = Container;
    using extents_type    = Extents;
    using reference       = std::ranges::range_reference_t<Container>;
    using const_reference = std::ranges::range_reference_t<const Container>;
    using index_type      = std::array<std::size_t, extents_type::rank()>;

private:
    NEO_NO_UNIQUE_ADDRESS extents_type   _shape     = extents_type();
    NEO_NO_UNIQUE_ADDRESS container_type _container = container_type();

public:
    constexpr mdarray_adaptor() { _container.resize(md::bounds(_shape)); }

    template <typename Shape>
        requires std::constructible_from<extents_type, Shape const&>
    explicit(not std::convertible_to<Shape, extents_type>)  //
        constexpr mdarray_adaptor(Shape&& shape)
        : _shape(NEO_FWD(shape)) {
        _container.resize(md::bounds(shape));
    }
    constexpr const extents_type& extents() const noexcept { return _shape; }

    constexpr reference operator[](index_type const& idx) noexcept {
        const auto span = md::mdspan_for_range(_container, _shape);
        reference  el   = span[idx];
        return el;
    }

    constexpr const_reference operator[](index_type const& idx) const noexcept {
        const auto      span = md::mdspan_for_range(_container, _shape);
        const_reference el   = span[idx];
        return el;
    }

    template <typename Shape>
        requires std::constructible_from<extents_type, Shape const&>
    constexpr void reshape(Shape&& ext) noexcept {
        reshape(extents_type(ext));
    }

    constexpr void reshape(index_type new_shape) noexcept {
        this->reshape(extents_type(new_shape));
    }

    constexpr void reshape(extents_type new_shape) noexcept {
        md::reshape(_container, extents(), new_shape);
        _shape = new_shape;
    }

    constexpr auto zero_cells() noexcept { return std::ranges::subrange(_container); }
    constexpr auto zero_cells() const noexcept { return std::ranges::subrange(_container); }

    // TODO: Define an origin() and a cursor type for this adaptor.
    //* almost: ...
    // constexpr auto origin() noexcept {
    //     return md::origin(md::mdspan_for_range(_container, _shape));
    // }
    // constexpr auto origin() const noexcept {
    //     return md::origin(md::mdspan_for_range(_container, _shape));
    // }
    //* (but not quite...)
};

template <typename T, typename Shape>
using mdvector = mdarray_adaptor<std::vector<T>, Shape>;

template <typename T, std::size_t Rank>
using mdvector_of_rank = mdvector<T, uz_dextents<Rank>>;

}  // namespace lmno::md
