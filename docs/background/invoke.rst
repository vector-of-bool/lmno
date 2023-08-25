Rich Invocation with :func:`lmno::invoke`
#########################################

.. namespace:: lmno

.. |invoke| replace:: :func:`invoke`
.. |Const| replace:: :class:`Const`
.. |stateless| replace:: :concept:`stateless`

|invoke| is one of the core powerful APIs of |lmno|. It provides
machinery to encode and transfer compile-time constants, as well as generate
"pretty" error messages in the case of constraint failure.

At its core, |invoke| serves the same purpose of `std::invoke`, and has the same
API-shape. The first argument must be an *invocable* object, and the subsequent
arguments will be the arguments to pass through to the underlying invocable::

  static_assert(lmno::invoke(std::plus<>{}, 4, 3) == 7);

But it has a few important differences.


Error Handling
**************

::

  // Compiles?!
  lmno::invoke(std::plus<>{}, 4, std::string("cat"));

The above code *will* compile, but will also generate a compile-time warning.
Here, |invoke| detects that `std::plus<>` cannot be invoked with arguments of
type `int` and `std::string`, and will modify its return type to be a special
error-type that indicates this error. The error-type is marked as
:cpp:`[[nodiscard]]`, so dropping it (as above) will produce a compiler warning.
If the compiler's diagnostic is formatted properly, the error message content of
the error-type will be printed in the compiler output.

Beyond :cpp:`[[nodiscard]]`, the error-type value is not usable for most
purposes. Attempting to access members or use it in other expressions will cause
a hard compile-error that will also include the error-type's diagnostic.

|lmno| also includes a concept, :concept:`err::non_error`, whose only purpose is
to reject the error-type returned by |invoke|. One can use this as a placeholder
for a variable or return type to hard-error immediately when an error-type is
generated::

  // Does not compile: lmno::non_error is not satisfied
  lmno::non_error auto sum = lmno::invoke(std::plus<>{}, 4, std::string("cat"));


Constant Propagation
********************

|invoke| propagates compile-time constants up and down the call stack. The class
template :class:`Const` is the primary representation of compile-time constants
in |lmno|.

If an argument of type |Const| is passed to |invoke| as a function argument,
|invoke| will attempt to invoke the underlying function using that |Const|
object. If the invocable does not accept the |Const| object directly, |invoke|
will retry the invocation with the `Const::value` instead. Thus |invoke| will
automatically "unwrap" |Const| values if the underlying callable object does not
handle them directly::

  constexpr int seven = invoke(std::plus<>{}, 4, Const<3>{});
  static_assert(seven == 7);

In the above, `std::plus` will reject the `Const<3>` as an argument, since
`4 + Const<3>{}` is ill-formed (|Const| does not provide a :cpp:`+` operator).
|invoke| will then fall-back on unwrapping the `Const<3>` and invoke `std::plus`
with two regular `int`\ s.

The auto-|Const| goes in both directions: If |invoke| detects that an invocation
will generate a stateful structural constant expression, it will wrap the
returned value in |Const|::

  std::same_as<Const<7>> auto seven
      = invoke(std::plus<>{}, Const<4>{}, Const<3>{});

This means that |invoke| with |Const| can be used to pass/return
":cpp:`constexpr` arguments" to/from functions.

This has a downside: The return type of |invoke| may be wrapped in |Const|
automatically in generic contexts. For this reason, generic code must be ready
to handle |Const| values.


When Does `Const`-return "kick-in"?
===================================

|invoke| will wrap the return value in |Const| if-and-only-if the following
conditions are met:

1. The invocable :cpp:`remove_cvref_t<F>` as well as each argument
   :cpp:`remove_cvref_t<Args>...` must be |stateless| types [#const_stateless]_.
2. The invocation must not be ill-formed nor produce an
   :concept:`~err::any_error` type.
3. The return type of the invocation must **not** be |stateless|.
   [#ret_not_stateless]_
4. The return type is :concept:`structural`.
5. The invocation itself must be a valid constant expression.

If all of the above conditions are satisfied, |invoke| will return a |Const|
object encoding the result of the invocation.


.. [#const_stateless]

  Note that |Const| is always a |stateless| type, since it has no
  non-static data members, and therefore encodes no runtime state.

.. [#ret_not_stateless]

  The return-type of |invoke| will not be wrapped in |Const| if the return type
  is |stateless|, because such a wrapping would be redundant: There is no value
  to encode within a stateless type, and so there is nothing that needs to be
  saved into a |Const|. Since the type is stateless, it will already be valid as
  a subsequent argument to produce other |Const| values.
