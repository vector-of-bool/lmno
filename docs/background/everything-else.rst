Everything Else
###############

.. default-domain:: cpp
.. default-role:: cpp
.. highlight:: cpp
.. namespace:: lmno

This documentation is incomplete, and there are several aspects that haven't
been covered here.

Rather than leave things unmentioned, it'll be best to let you, dear reader,
*know what you don't know* than to let you blindly stumble upon them. Here's a
brief summary:

1. The machinery underlying :func:`lmno::invoke`, which was very carefully
   optimized for compile-time speed and pretty error messages.
2. `error.hpp` and error generation with :func:`lmno::invoke`. The standard
   library functions implement an interface `T::error<Args...>()`, which should
   return an error object representing an explanation on why `T` is not callable
   with `Args`. This is used to generate error messages, rather than relying on
   the C++ compiler to generate enormous nasty template backtraces (I've seen
   them, and they're *bad*, and should not be shown to the user, if at all
   possible).
3. The implementation of "strand", represented by the underty ":lmno:`‿`". It's
   a range-ish thing that's also a :concept:`typed_constant` sometimes.
4. Compile-time string formatting with :class:`lmno::cx_str` and
   :var:`lmno::cx_fmt_v`.
5. Compile-time type and constexpr value rendering, implemented in
   `<lmno/render.hpp>`. Most types provide speicalizations of the `render_v` and
   `render_type_v` variable templates that will emit `cx_str` instances. These
   are used in error message generation.
6. The `rational` abstraction. Because `float` is just awful at compile-time.
