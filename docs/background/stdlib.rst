The Standard Library
####################

.. default-domain:: cpp
.. default-role:: cpp
.. highlight:: cpp
.. namespace:: lmno

By default, |lmno| contains no functions or operators. Even something as simple
as :lmno:`4 + 7` will not compile, because we don't have a name ":lmno:`+`"
defined.


The `define` Variable Templates
*******************************

The default name lookup context will fall back to `lmno::define<Name>` if `Name`
is not defined within the bound scope.

`define<>` is a variable template that is parameterized on a
:class:`lmno::lex::token`::

    template <lex::token Name>
    constexpr auto define = /* … */;

(The default value simply generates an unspecified error object.)

In order to evaluate any useful code, one must include the `<lmno/stdlib.hpp>`
header within the translation unit. This defines specializations of `define` for
all standard library names.

For example, the plus ":lmno:`+`" function is defined as an instance of a
callable object::

  namespace stdlib {
  struct plus {
    template <typename W>
    constexpr auto operator()(W&& w, addable<W> auto&& x) const
      NEO_RETURNS(w + x);
  };
  }

  template <>
  constexpr inline auto define<"+"> = stdlib::plus{};

Thus, when `default_context` looks for `define<"+">`, it will find this instance
of `stdlib::plus`.

All standard library components are defined in a similar manner.


Combinators
***********

The most complicated standard library definitions are the combinators. Almost
all of them are functions-returning-functions, and are very dense with
convenience macros to reduce reduntancy and boilerplate.

The definition of :lmno:`∘` "atop" is not the implementation of atop, but rather
a lambda expression that returns the atop closure object::

  template <>
  constexpr inline auto define<"∘"> =
      [](auto&& f, auto&& g) NEO_RETURNS_L(stdlib::atop{NEO_FWD(f), NEO_FWD(g)});

The `stdlib::atop` class template implements the actual semantics of :lmno:`∘`::

  template <typename F, typename G>
  struct atop {
    NEO_NO_UNIQUE_ADDRESS F _f;
    NEO_NO_UNIQUE_ADDRESS G _g;

    LMNO_INDIRECT_INVOCABLE(atop);

    constexpr auto call(auto&& x)
        NEO_RETURNS(invoke(_f, invoke(_g, NEO_FWD(x))));
    constexpr auto call(auto&& x) const
        NEO_RETURNS(invoke(_f, invoke(_g, NEO_FWD(x))));

    constexpr auto call(auto&& w, auto&& x)
        NEO_RETURNS(invoke(_f, invoke(_g, NEO_FWD(w), NEO_FWD(x))));
    constexpr auto call(auto&& w, auto&& x) const
        NEO_RETURNS(invoke(_f, invoke(_g, NEO_FWD(w), NEO_FWD(x))));
  };

.. note::

  The `LMNO_INDIRECT_INVOCABLE` macro defines `operator()` in terms of
  :func:`lmno::invoke` and the class's `call()` methods, and could be made far
  simple with C++23 *explicit object parameters* as a base class call calls
  `self.call()`. The :func:`lmno::invoke` function is used to generate better
  error messages in case of malformed calls.

  The *explicit object parameter* would also alleviate the redundancy in
  implementing `call()` twice for each `const`\ -ness.

