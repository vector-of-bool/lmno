#pragma once

#include "./concepts/stateless.hpp"
#include "./concepts/structural.hpp"
#include "./const.hpp"
#include "./error.hpp"
#include "./meta.hpp"
#include "./render.hpp"

#include <neo/fwd.hpp>
#include <neo/invoke.hpp>
#include <neo/returns.hpp>
#include <neo/type_traits.hpp>

#include <concepts>

namespace lmno {

namespace invoke_detail {

template <bool JustFalse>
struct pick_invoker;

template <typename F, typename... Args>
concept check_invocable_without_error =  //
    requires {
        requires neo::invocable2<unconst_t<F>, Args...>;
        requires non_error<neo::invoke_result_t<unconst_t<F>, Args...>>;
    };

template <typename F, typename... Args>
using pick_invoker2_t = pick_invoker<false and sizeof...(Args)>::template f<F, Args...>;

}  // namespace invoke_detail

/**
 * @brief Invoke an invocable object with the given arguments.
 *
 * Akin to std::invoke, but implements a few important features:
 *
 * - Error detection in case the invocable cannot be invoked with the given argumetns.
 * - Automatic unconst() of arguments if the underlying invocable cannot accept
 *   typed_constants directly.
 * - Detect a stateless invocation and wrap the result in Const{} if required.
 *
 * @param fn The invocable object
 * @param args The arguments to apply to the invocable object
 */
template <typename F,
          typename... Args,
          typename Invoker = invoke_detail::pick_invoker2_t<F, Args...>>
constexpr typename Invoker::template result_t<F, Args...>
invoke(F&& fn, Args&&... args) noexcept(Invoker::template is_nothrow_v<F, Args...>) {
    return Invoker::invoke(static_cast<F&&>(fn), static_cast<Args&&>(args)...);
}

#define LMNO_INVOKES(...) NEO_RETURNS(::lmno::invoke(__VA_ARGS__))

/**
 * @brief Obtain the type that would be returned by an invocation of lmno::invoke()
 * with the given argument types.
 *
 * @note This alias ALWAYS resolves to a type, but that type might be an error type!
 */
template <typename F, typename... Args>
using invoke_t = decltype(lmno::invoke(NEO_DECLVAL(F), NEO_DECLVAL(Args)...));

/**
 * @brief Match if the type F can be invoked with the given argument types,
 * and it *does not* return an error_type object
 */
template <typename F, typename... Args>
concept invocable = non_error<invoke_t<F, Args...>>;  //

using neo::remove_cvref_t;

template <typename F, typename... Args>
using invoke_error_t = remove_cvref_t<unconst_t<F>>::template error_t<Args...>;

template <typename F, typename... Args>
concept has_error_detail =  //
    requires {
        typename invoke_error_t<F, Args...>;
        requires any_error<invoke_error_t<F, Args...>>;
    };

template <bool HasDetail>
struct maybe_invoke_error {
    template <typename...>
    using f = void;
};

template <>
struct maybe_invoke_error<true> {
    template <typename F, typename... Args>
    using f = invoke_error_t<F, Args...>;
};

template <typename F, typename... Args>
using maybe_invoke_error_t
    = maybe_invoke_error<has_error_detail<F, Args...>>::template f<F, Args...>;

namespace invoke_detail {

struct nocast {
    template <typename T>
    using f = T;
};

template <typename T>
using just_ident = nocast;

template <bool DoUnconst>
struct unconst_arg {
    template <typename T>
    using f = T;
};

template <>
struct unconst_arg<true> {
    template <typename T>
    using f = unconst_t<T>;
};

template <typename... Cast>
struct basic_invoker {
    template <typename F, typename... Args>
    using result_t = neo::invoke_result_t<unconst_t<F>, typename Cast::template f<Args>...>;

    template <typename F, typename... Args>
    constexpr static bool is_nothrow_v = noexcept(
        neo::invoke(NEO_DECLVAL(unconst_t<F>), NEO_DECLVAL(typename Cast::template f<Args>)...));

    template <typename F, typename... Args>
    constexpr static decltype(auto) invoke(F&& f, Args&&... args) noexcept(is_nothrow_v<F, Args...>)
    // -> result_t<F, Args...> // Clang chokes when this is used?
    {
        return neo::invoke(static_cast<unconst_t<F>&&>(f),
                           static_cast<typename Cast::template f<Args>&&>(args)...);
    }
};

template <typename... Cast>
struct const_wrapping_invoker {
    using base = basic_invoker<Cast...>;

    template <typename F, typename... Args>
    using result_t = Const<base::invoke(remove_cvref_t<F>{}, remove_cvref_t<Args>{}...)>;

    template <typename...>
    constexpr static bool is_nothrow_v = true;

    template <typename F, typename... Args>
    constexpr static result_t<F, Args...> invoke(const F&, const Args&...) noexcept {
        return {};
    }
};

template <typename Child, cx_str Message, typename F, typename... Args>
struct [[nodiscard]] opaque_error : err::error_type<Message, Child> {
    static constexpr void show() noexcept
        requires neo::invocable2<F, Args...>
    {}
};

struct uninvocable {
    template <typename...>
    constexpr static bool is_nothrow_v = true;

    template <typename F, typename... Args>
    static constexpr auto make_error() {
        if constexpr (has_error_detail<F, Args...>) {
            using explained = invoke_error_t<F, Args...>;
            constexpr auto M
                = cx_fmt_v<"Object of type {:'} is not invocable with the given arguments {{{:}}}",
                           render::type_v<F>,
                           cx_str_join_v<", ", cx_fmt_v<"{:'}", render::type_v<Args>>...>>;
            return err::error_type<M, explained>{};
        } else {
            constexpr auto M
                = cx_fmt_v<"Object of type {:'} is not invocable with the given arguments {{{:}}}",
                           render::type_v<F>,
                           cx_str_join_v<", ", cx_fmt_v<"{:'}", render::type_v<Args>>...>>;
            return err::error_type<M, void>{};
        }
    }

    template <typename F, typename... Args>
    using result_t = decltype(make_error<unconst_t<F>, Args...>());

    template <typename F, typename... Args>
    [[nodiscard]] constexpr static result_t<F, Args...> invoke(F&&, Args&&...) noexcept {
        return {};
    }
};

template <typename... Cast>
struct error_chain_explaining_invoker {
    using base = basic_invoker<Cast...>;

    template <typename...>
    constexpr static bool is_nothrow_v = true;

    template <typename F, typename... Args>
    static constexpr auto make_error() {
        constexpr auto M
            = cx_fmt_v<"Object of type {:'} is not invocable with the given arguments {{{:}}}",
                       render::type_v<F>,
                       cx_str_join_v<", ", cx_fmt_v<"{:'}", render::type_v<Args>>...>>;
        if constexpr (has_error_detail<F, typename Cast::template f<Args>...>) {
            using explained = invoke_error_t<F, typename Cast::template f<Args>...>;
            return err::error_type<M, explained>{};
        } else {
            using inner_error = typename base::template result_t<F, Args...>;
            return err::error_type<M, inner_error>{};
        }
    }

    template <typename F, typename... Args>
    using result_t = decltype(make_error<unconst_t<F>, Args...>());

    template <typename F, typename... Args>
    [[nodiscard]] constexpr static result_t<F, Args...> invoke(F&&, Args&&...) noexcept {
        return {};
    }
};

template <cx_str M>
constexpr auto render_error(err::error_type<M, void>) {
    return M;
}

template <cx_str M, any_error Inner>
constexpr auto render_error(err::error_type<M, Inner>) {
    constexpr auto next   = render_error(Inner{});
    constexpr auto indent = cx_str_replace<next, cx_str{"\n"}, cx_str{"\n  "}>();
    return cx_fmt_v<"{}\n\n→ because:\n\n  {}", M, indent>;
}

template <typename Invoker>
struct error_renderer {
    template <typename...>
    constexpr static bool is_nothrow_v = true;

    template <typename F, typename... Args>
    static constexpr auto make_error() {
        using inner_error = typename Invoker::template result_t<F, Args...>;
        constexpr auto M
            = cx_fmt_v<"Invocation of an object of type {:'} with arguments of type {{{:}}} failed",
                       render::type_v<F>,
                       cx_str_join_v<", ", cx_fmt_v<"{:'}", render::type_v<Args>>...>>;
        using top                         = err::error_type<M, inner_error>;
        constexpr auto E [[maybe_unused]] = cx_fmt_v<
            "\n\n↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓\n"
            "\n"
            "{}\n\n"
            "↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑\n\n",
            render_error(top{})>;
        return err::error_type<E>{};
    }

    template <typename F, typename... Args>
    using result_t = decltype(make_error<F, Args...>());

    template <typename F, typename... Args>
    static constexpr result_t<F, Args...> invoke(F&&, Args&&...) {
        return {};
    }
};

template <typename... Caster>
struct pick_invoker_with_casts;

template <bool ReturnsStructural_and_IsStateless>
struct pick_invoker_for_const_wrapping;

template <bool IsConstexpr>
struct pick_is_constexpr;

template <typename... Cast>
struct const_wrapping_invoker;

template <std::size_t Stop, std::size_t Mask, typename Seq, typename F, typename... Args>
struct find_cast_combo;

template <std::size_t Stop, std::size_t Mask, typename Seq, typename F, typename... Args>
struct find_cast_combo_for_errors;

template <std::size_t N, typename Seq = std::make_index_sequence<N>>
constexpr auto make_bits_seq = 0;

template <std::size_t N, auto... Ns>
inline auto make_bits_seq<N, std::index_sequence<Ns...>> = std::index_sequence<(1 << Ns)...>{};

template <>
struct pick_invoker<false> {
    // We need to find the write combination of argument casts that will result in success
    template <typename F, typename... Args>
    using f = find_cast_combo<(1 << sizeof...(Args)),
                              0,
                              decltype(make_bits_seq<sizeof...(Args)>),
                              F,
                              Args...>  //
        ::template f<F, Args...>;
};

template <std::size_t Stop, std::size_t Mask, typename Seq, typename F, typename... Args>
struct find_cast_combo_for_errors;

template <std::size_t Stop, typename Seq, typename F, typename... Args>
struct find_cast_combo_for_errors<Stop, Stop, Seq, F, Args...> {  // Stop condition
    // Base error, the result is simply never invocable...
    template <typename, typename...>
    using f = error_renderer<uninvocable>;
};

template <std::size_t Stop, std::size_t Mask, auto... Ns, typename F, typename... Args>
    requires neo::invocable<unconst_t<F>,
                            typename unconst_arg<((1 << Ns) & Mask) != 0>::template f<Args>...>
struct find_cast_combo_for_errors<Stop, Mask, std::index_sequence<Ns...>, F, Args...> {
    template <typename, typename...>
    using f = error_renderer<uninvocable>;
    // = error_renderer<error_chain_explaining_invoker<unconst_arg<((1 << Ns) & Mask) != 0>...>>;
};

// Recursive case: The selected casts do not result in a valid call.
template <std::size_t Stop, std::size_t Mask, typename Seq, typename F, typename... Args>
struct find_cast_combo_for_errors {
    // Try the next set of cast bits:
    template <typename, typename...>
    using f = find_cast_combo_for_errors<Stop, Mask + 1, Seq, F, Args...>::template f<F, Args...>;
};

template <std::size_t Stop, std::size_t Mask, typename Seq, typename F, typename... Args>
struct find_cast_combo;

template <std::size_t Stop, typename Seq, typename F, typename... Args>
struct find_cast_combo<Stop, Stop, Seq, F, Args...> {  // Stop condition
    template <typename, typename...>
    using f = find_cast_combo_for_errors<Stop, 0, Seq, F, Args...>::template f<F, Args...>;
};

template <std::size_t Stop, std::size_t Mask, auto... Bits, typename F, typename... Args>
    requires check_invocable_without_error<
        F,
        typename unconst_arg<(Bits & Mask) != 0>::template f<Args>...>
struct find_cast_combo<Stop, Mask, std::index_sequence<Bits...>, F, Args...> {
    // This is the one we want
    template <typename, typename...>
    using f = pick_invoker_with_casts<unconst_arg<(Bits & Mask) != 0>...>  //
        ::template f<
            neo::invoke_result_t<unconst_t<F>,
                                 typename unconst_arg<(Bits & Mask) != 0>::template f<Args>...>,
            F,
            Args...>;
};

// Recursive case: The selected casts do not result in a valid call.
template <std::size_t Stop, std::size_t Mask, typename Seq, typename F, typename... Args>
struct find_cast_combo {
    // Try the next set of cast bits:
    template <typename, typename...>
    using f = find_cast_combo<Stop, Mask + 1, Seq, F, Args...>::template f<F, Args...>;
};

template <typename... Cast>
struct pick_invoker_with_casts {
    // Select an invoker based on the return type
    template <typename Ret, typename F, typename... Args>
    using f = pick_invoker_for_const_wrapping<
        // Does it NOT return a stateless object?
        //  - (We don't want to double-wrap, and we provide no value here)
        not stateless<Ret>
        // Does it return a structural type?
        //  - (We can't wrap non-structural types)
        and structural<Ret>
        // Are both the invocable and all of its arguments stateless objects?
        //  - (Otherwise not constexpr)
        and stateless<remove_cvref_t<F>> and (stateless<remove_cvref_t<Args>> and ...)>  //
        // Go on...
        ::template f<basic_invoker<Cast...>, F, Args...>;
};

template <>
struct pick_invoker_for_const_wrapping<false> {
    // Not a stateless invocation, or the return type is not structural. Just do a regular invoke:
    template <typename BaseInvoker, typename, typename...>
    using f = BaseInvoker;
};

template <>
struct pick_is_constexpr<false> {
    // Base case: Not a constexpr invocable, even though it qualified in every
    // other way
    template <typename BaseInvoker>
    using f = BaseInvoker;
};

template <>
struct pick_is_constexpr<true> {
    template <typename BaseInvoker>
    using f = neo::meta::rebind<BaseInvoker, const_wrapping_invoker>;
};

template <>
struct pick_invoker_for_const_wrapping<true> {
    template <typename BaseInvoker, typename F, typename... Args>
    using f = pick_is_constexpr<requires {
                                    typename lmno::Const<
                                        BaseInvoker::invoke(remove_cvref_t<F>{},
                                                            remove_cvref_t<Args>{}...)>;
                                }>::template f<BaseInvoker>;
};

}  // namespace invoke_detail

}  // namespace lmno
