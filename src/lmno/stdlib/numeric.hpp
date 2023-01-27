#pragma once

#include "../define.hpp"
#include "../invoke.hpp"
#include "../render.hpp"
#include "./arithmetic.hpp"
#include "./constants.hpp"
#include "./logic.hpp"
#include "./ranges.hpp"

#include <neo/attrib.hpp>
#include <neo/returns.hpp>
#include <neo/type_traits.hpp>

#include <ranges>

namespace lmno::stdlib {

namespace _sr = std::ranges;
namespace _sv = std::views;

/*
8888888888       888      888
888              888      888
888              888      888
8888888  .d88b.  888  .d88888
888     d88""88b 888 d88" 888
888     888  888 888 888  888
888     Y88..88P 888 Y88b 888
888      "Y88P"  888  "Y88888
*/

template <typename T, typename Operator>
constexpr auto make_no_id_elem_error_str() {
    constexpr auto M = cx_fmt_v<"No identity-element for type {:'} with binary operation {:'}",
                                render::type_v<T>,
                                render::type_v<Operator>>;
    return err::error_type<M>{};
}

template <typename Type, typename Operator>
constexpr auto identity_element = make_no_id_elem_error_str<Type, Operator>();

template <neo::integral I>
constexpr I identity_element<I, stdlib::plus> = I(0);

template <neo::integral I>
constexpr I identity_element<I, stdlib::minus_or_negative> = I(0);

template <neo::integral I>
constexpr I identity_element<I, stdlib::times_or_sign> = I(1);

template <neo::integral I>
constexpr I identity_element<I, stdlib::divide> = I(1);

template <neo::integral I>
constexpr int identity_element<I, stdlib::and_> = int(1);

template <neo::integral I>
constexpr int identity_element<I, stdlib::or_> = int(0);

template <typename Func>
struct fold {
    NEO_NO_UNIQUE_ADDRESS Func _binop;

    template <typename Init, typename T>
    using common_type_t = std::common_type_t<Init, neo::invoke_result_t<Func, Init, T>>;

    LMNO_INDIRECT_INVOCABLE(fold);

    template <typename Init,
              typename Binop,
              input_range_convertible R,
              typename Ref = range_reference_t<R>>
        requires requires {
                     typename common_type_t<Init, Ref>;
                     requires invocable<Binop&, common_type_t<Init, Ref>, Ref>;
                 }
    constexpr static auto _run(Binop& binop, Init init, R&& in) {
        auto value = static_cast<common_type_t<Init, Ref>>(init);
        for (Ref el : as_range(in)) {
            value = lmno::invoke(binop, NEO_MOVE(value), NEO_FWD(el));
        }
        return value;
    }

    constexpr auto call(auto&& init, auto&& range)
        NEO_RETURNS(this->_run(this->_binop, NEO_FWD(init), NEO_FWD(range)));

    constexpr auto call(auto&& init, auto&& range) const
        NEO_RETURNS(this->_run(this->_binop, NEO_FWD(init), NEO_FWD(range)));

    template <input_range_convertible R, typename Ref = range_reference_t<R>>
        requires invocable<Func, Ref, Ref>
    constexpr auto call(R&& range)
        NEO_RETURNS(this->call(identity_element<range_value_t<R>, Func>, NEO_FWD(range)));

    template <input_range_convertible R, typename Ref = range_reference_t<R>>
        requires invocable<Func, Ref, Ref>
    constexpr auto call(R&& range) const
        NEO_RETURNS(this->call(identity_element<range_value_t<R>, Func>, NEO_FWD(range)));

    template <typename Arg>
    constexpr static auto error() noexcept {
        if constexpr (not input_range_convertible<Arg>) {
            return err::fmt_error_t<"The argument must be an input range (Got {:'})",
                                    render::type_v<Arg>>{};
        } else if constexpr (not invocable<Func, range_reference_t<Arg>, range_reference_t<Arg>>) {
            return err::fmt_errorex_t<
                invoke_error_t<Func, range_reference_t<Arg>, range_reference_t<Arg>>,
                "The range elements (of type {:'}) are not valid operands for the binary "
                "function {:'}",
                render::type_v<range_reference_t<Arg>>,
                render::type_v<Func>>{};
        }
    }

    template <typename Init, typename Arg>
    constexpr static auto error() noexcept {
        if constexpr (not input_range_convertible<Arg>) {
            return err::fmt_error_t<"The right-hand argument {:'} is not an input-range",
                                    render::type_v<Arg>>{};
        } else if constexpr (not neo::has_common_type<unconst_t<Init>, range_reference_t<Arg>>) {
            return err::fmt_error_t<
                "There is no common type between the init-value of type {:'} and the range's "
                "element type {:'}",
                render::type_v<Init>,
                render::type_v<range_reference_t<Arg>>>{};
        } else if constexpr (not invocable<Func, range_reference_t<Arg>, range_reference_t<Arg>>) {
            return err::fmt_errorex_t<
                invoke_error_t<Func, range_reference_t<Arg>, range_reference_t<Arg>>,
                "The range elements (of type {:'}) are not valid operands for the binary "
                "function {:'}",
                render::type_v<range_reference_t<Arg>>,
                render::type_v<Func>>{};
        }
    }
};
LMNO_AUTO_CTAD_GUIDE(fold);

/*
 .d8888b.
d88P  Y88b
Y88b.
 "Y888b.    .d8888b  8888b.  88888b.
    "Y88b. d88P"        "88b 888 "88b
      "888 888      .d888888 888  888
Y88b  d88P Y88b.    888  888 888  888
 "Y8888P"   "Y8888P "Y888888 888  888
*/

// The "\" closure
template <typename Func>
struct scan {
    NEO_NO_UNIQUE_ADDRESS Func _binop;

    template <typename Init, typename T>
    using common_type_t = std::common_type_t<Init, neo::invoke_result_t<Func, Init, T>>;

    LMNO_INDIRECT_INVOCABLE(scan);

    template <typename Init,
              input_range_convertible    R,
              neo::has_common_type<Init> Ref = range_reference_t<R>,
              typename Value                 = common_type_t<Init, Ref>>
        requires(invocable<Func, Value, Ref>
                 and neo::assignable_from<Value&, invoke_t<Func, Value, Ref>>)
    constexpr decltype(auto) call(Init&& init, R&& in) {
        auto value = static_cast<Value>(NEO_FWD(init));
        return this->_scan(_binop, NEO_MOVE(value), as_range(NEO_FWD(in)));
    }

    template <typename Init,
              input_range_convertible    R,
              neo::has_common_type<Init> Ref = range_reference_t<R>,
              typename Value                 = common_type_t<Init, Ref>>
        requires(invocable<Func, Value, Ref>
                 and neo::assignable_from<Value&, invoke_t<Func, Value, Ref>>)
    constexpr decltype(auto) call(Init&& init, R&& in) const {
        auto value = static_cast<Value>(NEO_FWD(init));
        return this->_scan(_binop, NEO_MOVE(value), as_range(NEO_FWD(in)));
    }

    template <typename R, input_range_convertible Ru = unconst_t<R>>
    constexpr auto call(R&& in)
        NEO_RETURNS(this->call(identity_element<range_value_t<Ru>, Func>, NEO_FWD(in)));

    template <typename R, input_range_convertible Ru = unconst_t<R>>
    constexpr auto call(R&& in) const
        NEO_RETURNS(this->call(identity_element<range_value_t<Ru>, Func>, NEO_FWD(in)));

    template <typename R, typename Value>
    constexpr static auto _scan(auto&& func, Value value, R&& in) {
        if constexpr (typed_constant<R> and (_sr::random_access_range<R> or _sr::sized_range<R>)
                      and neo::default_initializable<Value>) {
            constexpr auto          size = static_cast<std::size_t>(_sr::distance(R{}));
            std::array<Value, size> _arr;
            _scan_into(_arr.begin(), func, value, in);
            return _arr;
        } else {
            std::vector<decltype(value)> vec;
            _scan_into(std::back_inserter(vec), func, value, in);
            return vec;
        }
    }

    constexpr static void _scan_into(auto out, auto&& fn, auto& value, auto& range) {
        for (decltype(auto) el : range) {
            value  = fn(NEO_MOVE(value), NEO_FWD(el));
            *out++ = value;
        }
    }

    // Defer to 'fold', which produces the same error messages
    template <typename... Args>
    static auto error() NEO_RETURNS(fold<Func>::template error<Args...>());
};
LMNO_AUTO_CTAD_GUIDE(scan);

/*
8888888         888
  888           888
  888           888
  888   .d88b.  888888  8888b.
  888  d88""88b 888        "88b
  888  888  888 888    .d888888
  888  Y88..88P Y88b.  888  888
8888888 "Y88P"   "Y888 "Y888888
*/

struct infinite_iota_range : _sr::view_interface<infinite_iota_range> {
    constexpr auto begin() const noexcept { return _sv::iota(std::int64_t{0}).begin(); }
    constexpr auto end() const noexcept { return _sv::iota(std::int64_t{0}).end(); }
};

template <typename Max>
struct iota_range : _sr::view_interface<iota_range<Max>> {
    NEO_NO_UNIQUE_ADDRESS Max mx;

    iota_range() = default;
    constexpr explicit iota_range(Max m) noexcept
        : mx(m) {}

    constexpr auto begin() const noexcept { return _sv::iota(Max(0), mx).begin(); }
    constexpr auto end() const noexcept { return _sv::iota(Max(0), mx).end(); }

    constexpr auto begin() noexcept { return _sv::iota(Max(0), mx).begin(); }
    constexpr auto end() noexcept { return _sv::iota(Max(0), mx).end(); }
};
LMNO_AUTO_CTAD_GUIDE(iota_range);

struct iota {
    LMNO_INDIRECT_INVOCABLE(iota);

    template <std::incrementable T>
    constexpr auto call(T max) const noexcept {
        return iota_range<T>{max};
    }

    constexpr auto call(stdlib::infinity) const noexcept { return infinite_iota_range{}; }

    template <typename W, typename X>
    static auto error() {
        return err::fmt_error_t<"Iota {:'} is not infix-invocable", cx_str{"⍳"}>{};
    }

    template <typename X>
    static auto error() {
        if constexpr (not std::incrementable<X>) {
            return err::fmt_error_t<"Iota operand {:'} is not an incrementable type",
                                    render::type_v<X>>{};
        }
    }
};

/*
8888888b.
888  "Y88b
888    888
888    888 888d888 .d88b.  88888b.
888    888 888P"  d88""88b 888 "88b
888    888 888    888  888 888  888
888  .d88P 888    Y88..88P 888 d88P
8888888P"  888     "Y88P"  88888P"
                           888
                           888
                           888
*/

struct drop {
    LMNO_INDIRECT_INVOCABLE(drop);

    constexpr auto call(neo::integral auto n, viewable_range_convertible auto&& r) const noexcept {
        return _sv::drop(as_range(r), static_cast<std::int64_t>(n));
    }

    template <typename N,
              typename R,
              typename Nu = neo::decay_t<unconst_t<N>>,
              typename Ru = unconst_t<R>>
    constexpr auto error() {
        if constexpr (not neo::integral<Nu>) {
            return err::fmt_error_t<"The left-hand operand of type {:'} is not an integral value",
                                    render::type_v<N>>{};
        } else if constexpr (not viewable_range_convertible<Ru>) {
            return err::fmt_error_t<"The right-hand operand of type {:'} is not a viewable-range",
                                    render::type_v<R>>{};
        }
    }
};

/*
88888888888       888
    888           888
    888           888
    888   8888b.  888  888  .d88b.
    888      "88b 888 .88P d8P  Y8b
    888  .d888888 888888K  88888888
    888  888  888 888 "88b Y8b.
    888  "Y888888 888  888  "Y8888
*/

struct take {
    LMNO_INDIRECT_INVOCABLE(take);

    constexpr auto call(neo::integral auto n, viewable_range_convertible auto&& r) const
        NEO_RETURNS(_sv::take(as_range(NEO_FWD(r)), static_cast<std::int64_t>(n)));

    template <typename N,
              typename R,
              typename Nu = neo::decay_t<unconst_t<N>>,
              typename Ru = unconst_t<R>>
    static auto error() {
        if constexpr (not neo::integral<Nu>) {
            return err::fmt_error_t<"Left-hand operand of type {:'} is not an integral type",
                                    render::type_v<N>>{};
        } else if constexpr (not viewable_range_convertible<Ru>) {
            return err::fmt_error_t<"Right-hand operand of type {:'} is not a viewable-range",
                                    render::type_v<R>>{};
        }
    }
};

}  // namespace lmno::stdlib

namespace lmno {

template <>
constexpr inline auto define<"/"> = [](auto&& f) { return stdlib::fold{NEO_FWD(f)}; };

template <>
constexpr inline auto define<"\\"> = [](auto&& f) { return stdlib::scan{NEO_FWD(f)}; };

template <>
constexpr inline auto define<"⍳"> = stdlib::iota{};

template <>
constexpr inline auto define<"↓"> = stdlib::drop{};

template <>
constexpr inline auto define<"↑"> = stdlib::take{};

template <typename Max, stdlib::iota_range<Max> R>
constexpr auto render::value_of_type_v<stdlib::iota_range<Max>, R>
    = cx_fmt_v<"·⍳{}", render::value_v<R.mx>>;

template <typename Max, auto I>
constexpr Max meta::static_range_size_v<Const<I, stdlib::iota_range<Max>>> = I.mx;

}  // namespace lmno

template <typename N>
constexpr bool std::ranges::enable_view<lmno::stdlib::iota_range<N>> = true;
