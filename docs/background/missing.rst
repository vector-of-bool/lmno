Missing: Arrays
###############

.. default-domain:: cpp
.. default-role:: cpp
.. highlight:: cpp
.. namespace:: lmno

Those familiar with APL, BQN, and just about any other symbolic array language
will notice the strong lack of "arrays" in |lmno|.

What is an array language without arrays!?

Well to that I say: |lmno| is not an array language.

And maybe that's a disappointing answer. I agree!

C++ is not an array language, and it is surprisingly difficult to shoe-horn an
array language into it, but not for lack of trying!

I made several attempts to fit multi-dimensional arrays into |lmno|, but I found
all approaches to be lacking. Indeed, I even published a Tweet demonstrating
array features, but if you look through the code, you won't find that feature!

The issue lies in an impedence mismatch between C++ *ranges* and BQN/APL-style
*arrays*. A brief list of questions that I *couldn't quite answer* in my
attempts:

1. APL-style arrays have *arbitrary* but *finite* dimensions (AKA the "shape" of
   the arrays). C++ ranges are limited to a single dimension, which may be of a
   compile-time fixed size, a runtime-known size, a runtime-unknown size (i.e.
   `forward_range` is not a `sized_range`), or even *infinite* ranges (e.g.
   `iota()`).
2. C++ has multi-dimensional arrays, but these are very limited, but a possible
   rough starting point.
3. `mdspan` is coming and provides a way to view a range as a multi-dimensional
   array. Several early attempts in |lmno| were based on using `mdspan`.
4. We need *more* than `mdspan`: We need `mdarray` or `mdvector`. What about
   multi-dimensional arrays that are potentially infinite in one or more
   extents? Do we need an "`mdrange`" concept? What would that look like? (I
   tried to define such a thing, but I am unsure of my atttempts.)
5. If we have an `mdrange` with any infinite extents, what would it mean to
   "deshape" :lmno:`⥊` such a thing?
6. We can define lazy ranges, such as filters and transforms. How would one
   define a lazy `mdrange`?
7. How would one define a multi-dimensional `mdrange` with a dynamic rank?
   `mdspan` won't help you there.
8. An iterator is defined with a sentinel to tell it when to stop. What would a
   multi-dimensional sentinel look like? Is this even a non-sense question?

My attempts at defining an `mdrange` are in the repository in a set of unused
files (In ``src/lmno/md/``), for any curious readers that may want to inspect my
attempts and attempt to salvage anything useful.
