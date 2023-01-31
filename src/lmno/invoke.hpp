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

/**
 * All the magic of lmno::invoke is hidden behind this class template, defined later in this file
 */
template <bool JustFalse>
struct pick_invoker;

template <typename F, typename... Args>
concept check_invocable_without_error =  //
    requires {
        requires neo::invocable2<unconst_t<F>, Args...>;
        requires non_error<neo::invoke_result_t<unconst_t<F>, Args...>>;
    };

template <typename F, typename... Args>
using pick_invoker2_t
    = pick_invoker<check_invocable_without_error<F, Args...>>::template f<F, Args...>;

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
inline constexpr struct invoke_fn {
    template <typename F,
              typename... Args,
              typename Invoker = invoke_detail::pick_invoker2_t<F, Args...>>
    constexpr typename Invoker::template result_t<F, Args...> operator()(F&& fn,
                                                                         Args&&... args) const
        noexcept(Invoker::template is_nothrow_v<F, Args...>) {
        return Invoker::invoke(static_cast<F&&>(fn), static_cast<Args&&>(args)...);
    }
} invoke;

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

template <typename F, typename... Args>
concept nothrow_invocable
    = invocable<F, Args...> and noexcept(invoke(NEO_DECLVAL(F), NEO_DECLVAL(Args)...));

using neo::remove_cvref_t;

template <typename F, typename... Args>
using invoke_error_t = decltype(remove_cvref_t<unconst_t<F>>::template error<Args...>());

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

template <typename T>
concept declares_indirect_invocable
    = requires { requires static_cast<bool>(T::declares_indirect_invocable); };

template <typename F>
struct invoke_indirect {
    NEO_NO_UNIQUE_ADDRESS F _wrapped;

    using Fd = neo::remove_cvref_t<F>;

    template <typename... Args>
    constexpr auto operator()(Args&&... args) NEO_RETURNS(_wrapped.call(NEO_FWD(args)...));

    template <typename... Args>
    constexpr auto operator()(Args&&... args) const NEO_RETURNS(_wrapped.call(NEO_FWD(args)...));

    template <typename... Args>
        requires requires { Fd::template error<Args...>(); }
    constexpr static auto error() {
        return Fd::template error<Args...>();
    }
};
template <typename F>
explicit invoke_indirect(F&&) -> invoke_indirect<F>;

/**
 * @brief Annotation within a class body that declares a call operator that does
 * the error-checking semantics of lmno::invoke()
 *
 * Use this within a class that you wish to use as a function-object, but want
 * lmno::invoke to catch errors and decode them for callers.
 *
 * IMPORTANT: Instead of defining your own operator(), define a call() function
 * that accepts the same arguments and is properly constrained. The indirect
 * invoker will attempt to invoke your function via `.call()` instead of `operator()`.
 *
 * Define a static member function template "error()" to render error messages
 * in case of constraint failures.
 *
 * This macro can go away when we have deduced-this parameter support, since we
 * can use a regular base class to define the "magic" operator().
 */
#define LMNO_INDIRECT_INVOCABLE(ThisType)                                                          \
    template <typename... Args, typename This = ThisType&>                                         \
    constexpr auto operator()(Args&&... args)                                       /**/           \
        noexcept(::lmno::nothrow_invocable<::lmno::invoke_indirect<This>, Args...>) /**/           \
    {                                                                                              \
        return ::lmno::invoke(::lmno::invoke_indirect{*this}, NEO_FWD(args)...);                   \
    }                                                                                              \
                                                                                                   \
    template <typename... Args, typename This = ThisType const&>                                   \
    constexpr auto operator()(Args&&... args) const                                 /**/           \
        noexcept(::lmno::nothrow_invocable<::lmno::invoke_indirect<This>, Args...>) /**/           \
    {                                                                                              \
        return ::lmno::invoke(::lmno::invoke_indirect{*this}, NEO_FWD(args)...);                   \
    }                                                                                              \
    static_assert(true)

namespace invoke_detail {

using meta::apply_f;

/**
 * @brief Conditionally apply unconst_t to a type
 *
 * @tparam DoUnconst If true, applies unconst_t
 *
 * Invoke this with the template alias member `f`
 */
template <bool DoUnconst>
struct unconst_arg {
    // False: DO NOT apply any type transform
    template <typename T>
    using f = T;
};

template <>
struct unconst_arg<true> {
    // True: DO apply unconst_t
    template <typename T>
    using f = unconst_t<T>;
};

/**
 * @brief Basic invoker implementation. Applies the given casts to each argument before forwarding
 * those arguments to the underlying invocable.
 *
 * @tparam Cast A list of casting-classes, applied via the member template alias `f`
 */
template <typename... Cast>
struct basic_invoker {
    /// Calculate the result type
    template <typename F, typename... Args>
    using result_t = neo::invoke_result_t<
        // We unconditionally strip const from the invocable, since it can never
        // effect the result.
        unconst_t<F>,
        // Apply the caster to each argument
        apply_f<Cast, Args>...>;

    template <typename F, typename... Args>
    constexpr static bool is_nothrow_v
        = noexcept(neo::invoke(NEO_DECLVAL(unconst_t<F>), NEO_DECLVAL(apply_f<Cast, Args>)...));

    template <typename F, typename... Args>
    constexpr static decltype(auto) invoke(F&& f, Args&&... args) noexcept(is_nothrow_v<F, Args...>)
    // -> result_t<F, Args...> // Clang chokes when this is used?
    {
        return neo::invoke(static_cast<unconst_t<F>&&>(f),
                           static_cast<apply_f<Cast, Args>&&>(args)...);
    }
};

/**
 * @brief Special invoker that is selected if the return value can be placed in
 * an lmno::Const<> typed-constant for returning to the caller.
 */
template <typename... Cast>
struct const_wrapping_invoker {
    using base = basic_invoker<Cast...>;

    template <typename F, typename... Args>
    using result_t = Const<base::invoke(remove_cvref_t<F>{}, remove_cvref_t<Args>{}...)>;

    // Evaluation is constexpr and happens at compile time, so it never throws
    template <typename...>
    constexpr static bool is_nothrow_v = true;

    template <typename F, typename... Args>
    constexpr static result_t<F, Args...> invoke(const F&, const Args&...) noexcept {
        return {};
    }
};

/**
 * @brief An error-handling invoker that handles the case that no invocation
 * satisfies the constraints of the invocable object.
 */
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
        } else if constexpr (neo::invocable2<F, Args...>) {
            using E = neo::invoke_result_t<F, Args...>;
            constexpr auto M
                = cx_fmt_v<"Object of type {:'} is not invocable with the given arguments {{{:}}}",
                           render::type_v<F>,
                           cx_str_join_v<", ", cx_fmt_v<"{:'}", render::type_v<Args>>...>>;
            return err::error_type<M, E>{};
        }
    }

    template <typename F, typename... Args>
    using result_t = decltype(make_error<unconst_t<F>, Args...>());

    template <typename F, typename... Args>
    [[nodiscard]] constexpr static result_t<F, Args...>
    invoke(F&& f [[maybe_unused]], Args&&... args [[maybe_unused]]) noexcept {
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

/**
 * @brief Pick the invoker for the given set of argument casts
 *
 * @tparam Caster The casts to apply to the arguments
 */
template <typename... Cast>
struct pick_invoker_with_casts;

/**
 * @brief Template that searches for the compination of casts that will result
 * in the invocation succeeding.
 *
 * @tparam Stop The stop point for the iteration
 * @tparam Mask The iteration step. Starts at zero. Stops at 'stop'
 * @tparam Seq A sequence of 1s and 0s representing the casting set to try
 * @tparam F The function
 * @tparam Args The arguments
 */
template <std::size_t Stop, std::size_t Mask, typename Seq, typename F, typename... Args>
struct find_cast_combo;

// Convert an unsigned integer into a set of powers of two bit masks
template <std::size_t N, typename Seq = std::make_index_sequence<N>>
constexpr auto make_bits_seq = 0;

template <std::size_t N, auto... Ns>
inline auto make_bits_seq<N, std::index_sequence<Ns...>> = std::index_sequence<(1 << Ns)...>{};

template <>
struct pick_invoker<false> {
    // We need to find the write combination of argument casts that will result in success
    template <typename F, typename... Args>
    using f = apply_f<find_cast_combo<(1 << sizeof...(Args)),
                                      0,
                                      decltype(make_bits_seq<sizeof...(Args)>),
                                      F,
                                      Args...>,  //
                      F,
                      Args...>;
};

template <>
struct pick_invoker<true> {
    // We can invoke it without any cast-dancing
    template <typename F, typename... Args>
    using f = apply_f<pick_invoker_with_casts<meta::just_t<unconst_arg<false>, Args>...>,  //
                      F,
                      Args...>;
};

template <std::size_t Stop, typename Seq, typename F, typename... Args>
struct find_cast_combo<Stop, Stop, Seq, F, Args...> {  // Stop condition
    template <typename, typename...>
    using f = error_renderer<uninvocable>;
};

template <std::size_t Stop, std::size_t Mask, auto... Bits, typename F, typename... Args>
    requires check_invocable_without_error<F, apply_f<unconst_arg<(Bits & Mask) != 0>, Args>...>
struct find_cast_combo<Stop, Mask, std::index_sequence<Bits...>, F, Args...> {
    // This is the one we want
    template <typename, typename...>
    using f = apply_f<pick_invoker_with_casts<unconst_arg<(Bits & Mask) != 0>...>, F, Args...>;
};

// Recursive case: The selected casts do not result in a valid call.
template <std::size_t Stop, std::size_t Mask, typename Seq, typename F, typename... Args>
struct find_cast_combo {
    // Try the next set of cast bits:
    template <typename, typename...>
    using f = find_cast_combo<Stop, Mask + 1, Seq, F, Args...>::template f<F, Args...>;
};

template <typename I, typename F, typename... Args>
concept is_constexpr_invocation =  //
    requires { typename lmno::Const<I::invoke(remove_cvref_t<F>{}, remove_cvref_t<Args>{}...)>; };

template <typename... Cast>
struct pick_invoker_with_casts {
    template <typename F, typename... Args>
    static auto pick() {
        using basic = basic_invoker<Cast...>;
        using Ret   = basic::template result_t<F, Args...>;
        if constexpr (stateless<Ret> or not structural<Ret> or not stateless<remove_cvref_t<F>>
                      or not(stateless<remove_cvref_t<Args>> and ...)) {
            // Return type is stateless (we don't want to double-wrap)
            // OR: not-structural, so we can't put the value in an NTTP
            // OR: The function/arguments aren't stateless, so we can't default-init them and be
            // certain we'll get the same result on invocation.
            return basic{};
        } else if constexpr (not is_constexpr_invocation<basic, F, Args...>) {
            // The invocation itself is not a constant expression, so we can't
            // get a constexpr value to place in an NTTP
            return basic{};
        } else {
            // It qualifies: Wrap the result in a Const<>:
            return const_wrapping_invoker<Cast...>{};
        }
    }

    // Select an invoker based on the return type
    template <typename F, typename... Args>
    using f = decltype(pick<F, Args...>());
};

}  // namespace invoke_detail

}  // namespace lmno
