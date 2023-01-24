#pragma once

#include "./concepts/stateless.hpp"
#include "./define.hpp"
#include "./lex.hpp"

#include <neo/tuple.hpp>
#include <tuple>

namespace lmno {

/**
 * @brief A scope of names and their associated values
 *
 * @tparam Pairs The name-value pairs in scope
 */
template <typename... Pairs>
struct scope;

/**
 * @brief A single name-value pair
 */
template <lex::token Name, typename T>
struct named_value {
    NEO_NO_UNIQUE_ADDRESS T _value;

    constexpr static const auto& name = Name;

    constexpr T&       get() noexcept { return _value; }
    constexpr const T& get() const noexcept { return _value; }
};

template <lex::token Name, typename T>
constexpr named_value<Name, T> make_named(T&& item) noexcept {
    return named_value<Name, T>{NEO_FWD(item)};
}

/**
 * @brief A default name lookup context. Contains a scope. When a name lookup fails,
 * falls back to lmno::define<> to find the name.
 *
 * @tparam Scope
 */
template <typename Scope = scope<>>
struct default_context {
    NEO_NO_UNIQUE_ADDRESS Scope _scope;

    /**
     * @brief Obtain the definition of the given name
     *
     * @tparam Name The name being looked up
     * @return The value bound to that name, or a sentinel representing its absence.
     */
    template <lex::token Name>
    constexpr decltype(auto) get() const noexcept {
        if constexpr (Scope::template has_name_v<Name>) {
            return _scope.template get<Name>();
        } else {
            return lmno::define<Name>;
        }
    }

    /**
     * @brief Create a new context based on the current context, with new names
     * bound to the scope.
     *
     * @tparam Items The new name-value pairs to add.
     * @param items
     * @return constexpr auto
     */
    template <typename... Items>
    constexpr auto bind(Items&&... items) const noexcept {
        auto new_scope = _scope.bind(NEO_FWD(items)...);
        return lmno::default_context{NEO_MOVE(new_scope)};
    }
};
LMNO_AUTO_CTAD_GUIDE(default_context);

namespace detail {

template <lex::token Name, typename Type>
struct nv_pairs_base {
    NEO_NO_UNIQUE_ADDRESS Type value;
};

template <lex::token Name, typename T>
constexpr named_value<Name, T> const& get_named(const named_value<Name, T>& p) noexcept {
    return p;
}

template <lex::token Name, typename T>
constexpr named_value<Name, T>& get_named(named_value<Name, T>& p) noexcept {
    return p;
}

using meta::list;

template <typename... Pairs>
struct name_value_pairs;

template <lex::token... Name, typename... Type>
struct name_value_pairs<named_value<Name, Type>...> : named_value<Name, Type>... {};

template <typename New, typename Old, typename Acc = meta::list<>>
struct replace_named;

template <typename... News, typename Old1, typename... Olds, typename... Acc>
struct replace_named<list<News...>, list<Old1, Olds...>, list<Acc...>>
    : replace_named<list<News...>, list<Olds...>, list<Acc..., Old1>> {};

template <typename... News, typename Old1, typename... Olds, typename... Acc>
    requires((News::name == Old1::name) or ...)
struct replace_named<list<News...>, list<Old1, Olds...>, list<Acc...>>
    : replace_named<list<News...>, list<Olds...>, list<Acc...>> {};

template <typename... News, typename... Acc>
struct replace_named<list<News...>, list<>, list<Acc...>> {
    using keep = list<Acc...>;
    using type = list<Acc..., News...>;
};

}  // namespace detail

template <lex::token... Names, typename... Types>
struct scope<named_value<Names, Types>...> {

    using tuple = detail::name_value_pairs<named_value<Names, Types>...>;

    NEO_NO_UNIQUE_ADDRESS tuple _values;

    scope() = default;

    constexpr scope(tuple&& t) noexcept
        : _values(NEO_MOVE(t)) {}

    template <lex::token N>
    constexpr static bool has_name_v = ((N == Names) or ...);

    template <lex::token Name>
        requires has_name_v<Name>
    constexpr decltype(auto) get() const noexcept {
        return detail::get_named<Name>(_values).get();
    }

    template <typename... Pairs>
    constexpr auto bind(Pairs&&... ps) const noexcept {
        using to_add    = meta::list<Pairs...>;
        using current   = meta::rebind<tuple, meta::list>;
        using replace   = detail::replace_named<to_add, current>;
        using new_list  = replace::type;
        using keep_list = replace::keep;
        using new_tuple = meta::rebind<new_list, detail::name_value_pairs>;
        return this->_bind_copy(static_cast<keep_list*>(nullptr),
                                static_cast<new_tuple*>(nullptr),
                                NEO_FWD(ps)...);
    }

    template <lex::token... Keep, typename... KeepTypes, typename... NewPairs, typename... AddPairs>
    constexpr auto _bind_copy(meta::list<named_value<Keep, KeepTypes>...>*,
                              detail::name_value_pairs<NewPairs...>*,
                              AddPairs&&... ps) const noexcept {
        auto tup = detail::name_value_pairs<NewPairs...>{{this->template get<Keep>()}...,
                                                         {NEO_FWD(ps)}...};
        return scope<NewPairs...>(NEO_MOVE(tup));
    }
};

template <lex::token... Names, typename... Types>
constexpr bool enable_stateless_v<scope<named_value<Names, Types>...>> = (stateless<Types> and ...);

template <lex::token... Names, typename... Types>
constexpr bool enable_stateless_v<default_context<scope<named_value<Names, Types>...>>>
    = (stateless<Types> and ...);

}  // namespace lmno
