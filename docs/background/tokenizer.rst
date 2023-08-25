The Tokenizer
#############

.. default-role:: cpp
.. highlight:: cpp
.. cpp:namespace:: lmno

This page will detail the implementation and design of the |lmno| tokenizer. The
tokenizer is by far the most "wow" feature from the outside perspective, since
it has been difficult to process strings at compile time for most of C++'s
history. However, the tokenizer itself is probably the simplest component in the
stack. Once tokenization is complete, the rest of |lmno| operates using
mostly-familiar C++ template metaprogramming.


Accepting a String Input
************************

The primary input for the tokenizer is a compile-time constant array of `char`.
Within |lmno|, the default compile-time string type is
:type:`lmno::cx_str\<N\> <cx_str>`, where `N` is the number of `char`\ s
in the array (not including the nul-terminator).

The tokenizer entrypoint is a regular alias template::

  template <cx_str S>
  using tokenize_t = decltype(tokenization_impl<S>();

If invoked with a character array (including a string literal), |CTAD| will take
care of deducing the `N` for :type:`cx_str`.

We next pass that string to the internal entrypoint of the tokenizer, which is
itself a function-template::

  template <cx_str S>
  auto tokenization_impl() {
    ...
  }

.. note::

  The names here are only expository. The actual names in code may vary and are
  not part of the public API.


Preparing an Array
******************

Since we will be accumulating tokens into an array, it may be tempting to use
`std::vector`, which is `constexpr` in C++20. However: This is of little help to
us here, since such a `vector` cannot be used as an argument to a template.

Instead, we can make the observation that we *cannot* generate more tokens than
there are `char` in the string, so we can instead just use a `std::array`::

  template <cx_str S>
  auto tokenization_impl() {
    array<something?, S.size()> tokens = {};
    ...
  }


Defining the Intermediate Type
******************************

But what would the element type be? A token can be variable-length, so we can't
put a `std::string` inside (even if it is `constexpr`-capable, we still run into
the same trouble as `std::vector`). Maybe we can use `std::string_view`, since
it's non-allocating and cheap to copy? Again: This doesn't work.
`std::string_view` is not valid as a non-type template parameter.

The answer is pretty simple: Just a pair of *position*\ +\ *length* values that
refer to subranges of the string::

  struct token_range {
    int pos;
    int len;
  };

::

  template <cx_str S>
  auto tokenization_impl() {
    array<token_range, S.size()> tokens = {};
    // ...
  }


Filling and Using the Array
***************************

This specialization of `std::array` is valid as a non-type template parameter,
so we're good to use this. After we have filled in the `tokens` array, we just
need to pass it to a template metafunction to turn the array of ranges back into
a :cpp:type:`~lmno::lex::token_list`::

  // Writes token ranges into 'out', returns the number of tokens.
  constexpr int do_tokenize(token_range* out, const char* string);

  template <cx_str S>
  auto tokenization_impl() {
    array<token_range, S.size()> tokens = {};
    do_tokenize(tokens.data(), S.data());
    return finalize_tokens_somehow_v<tokens>;
  }

The above code looks promising, but it doesn't work: The `tokens` array needs to
be `constexpr` in order to be used as a template argument, but if we declare it
`constexpr`, we won't be able to modify it in `do_tokenize`!


More Indirection
****************

As always, every problem can be solved with another level of indirection::

  constexpr int do_tokenize(token_range* out, const char* string);

  template <cx_str S>
  constexpr auto tokenization_impl_inner() {
    array<token_range, S.size()> tokens = {};
    int num_tokens =  do_tokenize(tokens.data(), S.data());
    return make_pair(num_tokens, tokens);
  }

  template <cx_str S>
  auto tokenization_impl() {
    constexpr auto pair = tokenization_impl_inner<S>();
    constexpr int num_tokens = pair.first;
    constexpr auto tokens = pair.second;
    // Finalize the tokens:
    return finalize_tokens_v<S, tokens, std::make_index_sequence<num_tokens>>;
  }


Finalization
************

The last step, `finalize_tokens_v`, accepts the string, array of `token_range`\
s, and an index sequence based on the number of tokens in the we found. We just
define a partial specialization of the variable template `finalize_tokens_v` to
unpack the sequence and use it to rebind into a :type:`~lmno::lex::token_list`::

  template <cx_str String, auto TokenRanges, typename Seq>
  auto finalize_tokens_v = delete;

  template <cx_str String, auto TokenRanges, auto... I>
  auto finalize_tokens_v<String,
                         TokenRanges,
                         std::index_sequence<I...>>
      = token_list<fin_1_token(S.data(), TokenRanges[I])...>{};

`fin_1_token` is a very simple function that accepts a pointer to the beginning
of the source string and a `token_range`, and returns a
:class:`~lmno::lex::token` corresponding to the range.
