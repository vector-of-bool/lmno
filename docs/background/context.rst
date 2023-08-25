The Context (Name Lookup)
#########################

.. default-role:: cpp
.. highlight:: cpp
.. default-domain:: cpp
.. namespace:: lmno

The default :doc:`evaluator <evaluator>` accepts a context as input, and only
uses two methods on that context: `bind` and `get`.

`bind()`
    - Accepts a name-value pair object and returns a new context that has that
      name bound to the value.

`get<Name>()`
    - Accepts a name token (as a *template* argument) and returns a reference to
      the value (or an error object if the name is not found).


The `default_context`
*********************

|lmno| comes with a simple `default_context`, which implemented using
`lmno::scope`. `lmno::scope` is simply a tuple of named values, and it starts
out empty.

Getting a Name
==============

`default_context::get()` performs two lookups::

  template <lex::token Name>
  constexpr decltype(auto) get() const noexcept {
    if constexpr (Scope::template has_name_v<Name>) {
      return _scope.template get<Name>();
    } else {
      return lmno::define<Name>;
    }
  }

First, it introspects the `Scope` template parameter to see if it has a value
bound to `Name`. If it does, we call `.get()` on that scope object to resolve
the name. (Names are compile-time constants, but they may refer to runtime
values.)

If the scope does not contain the `Name`, then we return `lmno::define<Name>`,
which is a variable template that allows names to be defined globally. This is
how the |lmno| standard library is defined. If there is no specialization of
`define` for `Name`, then the default value will be an error object.

.. seealso::

  :doc:`stdlib` for information on `lmno::define` and the standard library.


The `scope` Template
********************

The `scope` class template implements the core of name storage and retrieval.
It is a variadic template with only a partial specialization defined::

  template <lex::token... Names, typename... Types>
  struct scope<named_value<Names, Types>...> {
    using tuple = detail::name_value_pairs<named_value<Names, Types>...>;

    tuple _values;
    // …
  };

The variadic pack `Names` allows us to implement `has_name_v` as a simple pack expansion expression::

  template <lex::token Name>
  constexpr static bool has_name_v = ((Name == Names) or ...);

The `name_value_pairs` class template uses variadic inheritance to create a
tuple of each value. It inherits from each `named_value` that is given as a
template argument.


Getting a Name
==============

`scope::get` is implemented using a special tuple retrieval::

  template <lex::token Name>
    requires has_name_v<Name>
  constexpr named_t<Name> get() const noexcept {
    return detail::get_named<Name>(_values).get();
  }

The `get_named` function appears as::

  template <lex::token Name, typename T>
  constexpr named_value<Name, T> const&
  get_named(const named_value<Name, T>& p) noexcept {
    return p;
  }

Importantly, we provide only the `Name` argument explicitly, and leave `T` to be
deduced. The compiler must now match the `name_value_pairs` class against this
argument set. It scans through all the base classes to find a match. Because
there is only one base class that has the correct `Name`, there is only one
acceptable candidate for the conversion. The compile is then able to infer the
argument `T` and bind to the correct subobject of the tuple.

The `.get()` function on the pair will simply return the value that is enclosed.


Binding Names
=============

The `bind` function on `scope` needs to create a new `scope` type that contains
the same values as the current scope, plus new names being bound, minus the
names that are being shadowed.

For `scope`, this is simply a sequence of metafunctions::

  template <typename... Pairs>
  constexpr auto bind(Pairs&&... ps) const noexcept {
    using to_add    = meta::list<Pairs...>;
    using current   = meta::rebind<tuple, meta::list>;
    using replace   = detail::replace_named<to_add, current>;
    using new_list  = replace::type;
    using keep_list = replace::keep;
    return this->_bind_copy(static_cast<keep_list*>(nullptr),
                            static_cast<new_list*>(nullptr),
                            FWD(ps)...);
  }

It works as:

1. Convert the variadic pack into a list `to_add`
2. Convert our existing names (`tuple`) back into a list
3. Call the "replace named" metafunction to remove items from `current` that
   exist in `to_add`, and appends them to the current list.
4. `new_list` is the new list, including the names that are being bound.
5. `keep_list` is the list from `current` of names that *are not* in `new_list`.

The `_bind_copy` function simply constructs the new scope::

  template <lex::token... Keep,
            typename... KeepTypes,
            typename... NewPairs,
            typename... AddPairs>
  constexpr auto _bind_copy(meta::list<named_value<Keep, KeepTypes>...>*,
                            meta::list<NewPairs...>*,
                            AddPairs&&... ps) const noexcept {
    auto tup = detail::name_value_pairs<NewPairs...>{{this->template get<Keep>()}...,
                                                     {FWD(ps)}...};
    return scope<NewPairs...>(std::move(tup));
  }

List initialization is used to construct each base class of `name_value_pairs`,
using a pack expansion of `Keep` to obtain the current values from our own
tuple.
