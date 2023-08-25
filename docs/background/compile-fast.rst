Compiling *as Fast as Possible*
###############################

.. default-role:: cpp
.. highlight:: cpp

Implementing a compile-time programming-language-compiler puts a huge strain on
the C++ compiler itself. In order to maintain usability of the library, it is
essential that compile/run iteration times stay as low as possible. We need to
get feedback to the user within a couple seconds, *at most*.

This page won't be |lmno|-specific, but it *will* detail the thought process
that is useful in any template-heavy C++ project (and likely any programming
language with "reified generics").


Knowing Your Tool
*****************

When metaprogramming, consider the compiler to be a scripting interpreter, that
accepts your script as input and produces a program as output. Operations are
not free, but some operations are more expensive than others.

The cost of particular compile-time operations will vary between
implementations, but we can make hueristic estimates about the cost of certain
operations.


Hueristics, as this Author Understands Them
*******************************************

This author uses the following heuristics to consider the cost of certain
entities.

.. rubric:: C++ parsing and semantic analysis

1. C++ tokenization and non-dependent parsing is basically free. Adding a
   `noexcept` to a function will not make any noticeable slowdown. You will need
   to add hundreds of `struct`\ s to a file before the compiler starts to slow
   down. Don't break a single 500-line file into five 100-line files "to save
   compile time".
2. Looking up `#include` files can vary wildly between systems. Most Unix-like
   systems are okay here, but Windows is especially troublesome because
   `CreateFileW()` has an absurd overhead compared to a POSIX `open`/`openat`.
3. The preprocessor can be slow if used heavily, but is usually very cheap.
   Token-replacement is cheap, but repeated rescanning and pasting and
   rescanning and pasting over and over will add up. Avoid using headers that do
   this in other high-traffic headers. `<ranges>` is relatively cheap (not
   counting its transitive dependencies), but ``range-v3`` is very expensive due
   to its extremely heavy preprocessor usage.
4. `inline` codegen can add up. If a function is declared `inline` and is used
   within a translation unit, then the compiler must emit codegen for that
   function and embed it within every translation unit where it is used. This
   author considers this a significant deficiency in the archaic 80s'
   compile/link model. This can change when using LTO and will be alleviated
   with C++ Modules.
5. All `inline` functions must have semantic analysis performed in every
   translation unit in which they are seen, regardless of whether they are used.
   This means that semantic analysis will be done for an `inline` function
   multiple times during a build, even if there's only one definition in the
   program.
6. Dependent expressions within function templates are cheaper than a
   non-template counterpart, since their semantic analysis will be deferred
   until the templated entity is used. Whether this is significant enough to
   notice is unknown to this author.


.. rubric:: Understanding the Costs of C++ Templates

1. The mere presence of a templated entity within a translation unit is not more
   costly than a non-templated entity. In fact, an unused function template or
   unused class template *may be cheaper* than a non-templated counterpart,
   since a subset of their body likely contains dependent types and
   subexpressions that do not require full semantic analysis until a template
   instantiation is generated::

      inline int foo(widget a, gadget b) {
        // Costly: Semantic analysis must validate this expression every time
        // `foo` appears in a translation unit, even if it is unused:
        return a + b;
      }

      template <typename A, typename B>
      auto bar(A a, B b) {
        // Free if `bar` is unused: The semantics of `a + b` are entirely
        // dependent on the template parameters, so there's nothing to do for
        // this function template except a basic syntax check.
        return a + b;
      }

2. As a counter to the prior: Function templates must undergo codegen and
   semantic analysis once for *each* unique set of template arguments!

   ::

      widget w;
      gadget g;

      // Cheap: Compiler has already done sema on `foo`, now
      //        just needs to do codegen.
      foo(w, g);
      // Second is free: Compiler has already done sema
      //                 and codegen for `foo`
      foo(w, g);

      // Costly: An implicit instantiation is used, so the compiler must run
      //         sema and codegen for `bar<widget, gadget>()`
      bar(w, g);
      // Almost-free: The compiler already did sema and codegen, but must run
      //              template-argument deduction again.
      bar(w, g);
      // Costly again! We're instantiating `bar<gadget, widget>`, and the
      //               compiler must do sema and codegen for it as a completely
      //               different function!
      bar(g, w);

      // Free: Grab a template instantiation explicitly;
      auto bar_func = bar<widget, gadget>;
      // Free: Compiler has already done sema+codegen on bar<widget, gadget>,
      //       and `bar_func` is not a template, so no argument deduction
      //       is necessary.
      bar_func(w, g);

   This factor is *essential* to understand why C++ templates are often
   considered "slow." Parsing, semantic analysis, and codegen of a template are
   not significantly different from a non-templated entity, but it is easy to
   accidentally explode the number of specializations of a template.
   Instantiating `bar` with a hundred different sets of arguments will generate
   a hundred different functions, which is roughly as expensive as having
   written those hundred functions as `inline` functions.

3. Requesting the specialization of a class or function template that has
   already been instantiated within that same translation unit is virtually
   free. Compilers remember the template specializations they have already
   generated and will quickly look them up when referenced again. This is known
   as "memoization".

4. For templates, differing `cvr`-qualifiers are completely different types::

      template <typename S>
      auto size(S&& s) {
        return s.size();
      }

   ::

      auto f(string s1, const string s2, string s3) {
        // Yikes!
        return size(s1) + size(s2) + size(move(s3));
      }

   In the above example, there are *three* different specializations of the
   `size` function generated: `size<string&>`, `size<const string&>` and
   `size<string>`! And all of them will require full semantic analysis and
   codegen!

   If you're certain you can get away with it, avoid using forwarding
   references. Prefer to use `const&` or by-value parameters::

      template <typename S>
      auto size(const S& s) {
        return s.size();
      }

      // Better: now `f` will only request `size<string>` three times, and we
      //         only pay for this one specialization.

   If you're going to request a template specialization and you know your
   `cvr`-qualifiers are irrelevant, try to normalize your `cvr`-qualifiers
   before doing the call (e.g. with a `static_cast` and/or a `remove_cvref_t`).

   .. note::
      For fully generic code, one can't often get away with such optimizations,
      since adding a `const` or collapsing a `&&` may effect the semantics of
      the program in important ways. Be careful.

5. Generating an instantiation is infinitely more expensive than simply uttering
   the name of the specialization::

      template <typename T>
      struct tag { };

      template <typename T>
      struct undefined_tag;

      void fun(tag<int>) {}
      // ↑ Costly: We're requesting the compiler to instantiate `tag<int>`, as
      //           well as its special member functions.

      void fun2(undefined_tag<int> const&) {}
      // ↑ Free: We're only uttering the name of `undefined_tag<int>`. The
      //         template body is not available, so there is no sema/codegen to
      //         do.
      //    But: This function is uncallable, because we cannot obtain an
      //         instance of `undefined_tag<int>` to bind to the reference.
      //         However…

      void func3(undefined_tag<int>*);
      // ↑ Also free: And we /can/ construct a pointer to `undefined_tag<int>`:

      void callit() {
        // Call `func3`!
        func3(static_cast<undefined_tag<int>*>(nullptr));
      }

   This trick also allows us to use function templates via tag-dispatch without
   paying for instantiating the tag::

      template <typename T>
      auto func4(undefined_tag<T>*) {
        // ...
      }

      template <typename X>
      auto g() {
        // Good: We pay for the specialization of `func4`, but we do not
        //       pay for the specialization of `undefined_tag<X>`
        return func4(static_cast<undefined_tag<X>*>(nullptr));
      }

   .. note::

      |lmno| defines a type-alias utility `lmno::meta::ptr<T>` that is simply an
      alias for `T*`. This allows a more terse generation of the `nullptr` than
      the `static_cast`: `ptr<undefined_tag<X>>{}`

   .. note::

      Uttering the name of a specialization is *not* free if the template has
      constraints on its template parameters, since the compiler must validate
      that the constraints are satisfied, even if the body of the class or
      function template is not defined.

6. Prefer undefined-function type-transformers to class-template
   simple type-transformers::

      template <typename X>
      struct remove_pointer;

      template <typename X>
      struct remove_pointer<X*> {
        using type = X;
      };

      // Slow and sad: Every distinct invocation of `remove_pointer_t` will
      // necessarily generate a new class-template instantiation of
      // `remove_pointer<Ptr>`. We don't actually even care about the class,
      // we just want the type that's inside.
      template <typename Ptr>
      using remove_pointer_t = remove_pointer<Ptr>::type;

   The alternative using an undefined-function::

      template <typename X>
      auto remove_pointer_f(undefined_tag<X*>*)
        -> X;

      // Fast: We instantiate zero templates!
      template <typename Ptr>
      using remove_pointer_t =
          decltype(remove_pointer_f(ptr<undefined_tag<Ptr>>{}));

   .. note::

      This may be slower in cases of passing the same type many many times,
      since the traits-class design will hit the memoization of the
      specialization, whereas the function-template version requires
      pattern-matching against the `undefined_tag` argument on every usage.

7. Prefer member-alias templates to additional class parameters for
   parameterized type-transformers::

      template <bool DoAddRef, typename X>
      struct maybe_add_ref {
        using type = X&;
      };

      template <typename X>
      struct maybe_add_ref<false, X> {
        using type = X;
      };

      // Slow and sad: For every type we pass to maybe_add_ref_t, we necessarily
      // instantiate a new `maybe_add_ref` class
      template <bool B, typename T>
      using maybe_add_ref_t = maybe_add_ref<B, T>::type;

   Instead, parameterize the transformer class on the least-varying parameters,
   and pass the type to a nested alias template::

      template <bool DoAddRef>
      struct maybe_add_ref {
        template <typename T>
        using f = T&;
      };

      template <>
      struct maybe_add_ref<false> {
        template <typename T>
        using f = T;
      };

      // Much faster in almost all cases: Only two specializations of
      // `maybe_add_ref` can ever exist.
      template <bool B, typename T>
      using maybe_add_ref_t = maybe_add_ref<B>::template f<T>;

8. Prefer explicitly providing function template arguments *when possible*.
   (**Do not** provide template arguments to functions that don't explicitly
   allow that as part of their API.)

9. Avoid deduced return types on functions that may appear as candidates of an
   overload set::

      template <typename X>
        requires frombulatable<X>
      auto frombulate(X&&) { /* ... */ }

      template <typename X>
        rqeuires frombulatable<X> and copyable<X>
      auto frombulate(X&&) { /* ... */ }

   When performing overload selection on `frombulate` above, the compiler may
   instantiate both function templates, even if the second version is a better
   match, since it cannot know the full signature of `frombulate` until it has
   deduced the return type.

   (This may not be true on all compilers or in all situations.)

10. Additionally, when a return type is deduced, the compiler must perform
    *eager* instantiation of the function template. If the return type is
    non-deduced, then the compiler can *defer* the specialization and semantic
    analysis of the function template instantiation until the end of processing
    the translation unit. This will not make a successful compile faster, but it
    will allow the compiler to save the effort if a later error within the same
    translation unit renders the TU invalid. This will result it faster
    turnaround times when users encounter unrelated compiler errors.

11. Avoid recursive template instantiations unless *absolutely necessary*. This
    is by far the most expensive operation you can ask of a compiler.

12. The standard `<concepts>` and `<type_traits>` are very slow because they are
    defined in terms of variable templates rather than the other way around.
    Especially in libstdc++, the variable templates are then defined in terms of
    the trait class templates.

    Routinely when reviewing time-trace profiles, I find that
    `std::is_trivially_destructible<T>` is a huge slow-down, because a lot of
    concepts transitively depend on it.

    |lmno| uses type traits and concepts from neo-fun__, which are equivalent to
    the standard-library counterparts but are defined in terms of
    `requires`-expressions and compiler intrinsics rather than class templates.
    These concepts and traits are enormously faster to compile and use than the
    ones found in `<concepts>`.

    __ https://github.com/vector-of-bool/neo-fun/blob/develop/src/neo/concepts.hpp


.. rubric:: The cost of `constexpr`

1. When writing `constexpr` code, assume that no optimizations are performed,
   and every single subexpression has a cost.

   Maybe someday we'll have JIT-compiled `constexpr` code. Until then, "naive"
   optimizations within `constexpr` functions are very effective.

   The "zero-overhead" abstractions can have quite high overhead within
   `constexpr`. e.g. The |lmno| tokenizer uses a `const char*` directly rather
   than a `string_view`, and uses raw loops rather than `<algorithm>`.

2. Be aware of when a `constexpr` happens. Constructors and operators can be
   `constexpr` even if they are undeclared. It is generally good to have as much
   be `constexpr` as possible, but it is important to know when it happens.
