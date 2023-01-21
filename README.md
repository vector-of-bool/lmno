# `lmno` an Embedded Programming Language for C++

`lmno` (pronounced "elemenoh") is a programming language presented as a generic
C++ library. The core language itself is symbolic, following in the path of many
array programming languages such as APL, J, and BQN. The `lmno` language borrows
many symbols and concepts from APL and BQN, but has several important
differences.


## Using `lmno`

At time of writing, there are no ready-made packages for `lmno` that can be
easily downloaded and integrated into another project. Besides that, `lmno` is
very experimental and is undergoing a lot of frequent significant changes. To
experiment with `lmno`, one can download the source repository and build and
experiment with it using [`bpt`](https://bpt.pizza/).


## The Basic API

The main "entrypoint" for the `lmno` language is the `lmno::eval` function
template. In its most basic usage, it accepts a string literal as its sole
template parameter, and returns the result of evaluating that string as an
`lmno` program:

```c++
#include <iostream>

// The main header with lmno::eval
#include <lmno/eval.hpp>
// Required to define all default functions:
#include <lmno/stddefs.hpp>

int main() {
    // Evaluate a simple expression:
    auto result = lmno::unconst(lmno::eval<"2+3">());
    std::cout << std::format("The sum is {}\n", result);
}
```

The meaning of `lmno::unconst()` will become clear later. In short: In this
example, `lmno` detects that the evaluation will generate a compile-time
constant, and it will encode the evaluation result in a type
`lmno::Const<5, std::int64_t>` that can later be given back to `lmno` to recover
the result of the previous compile-time evaluation. One can use `unconst()` to
strip away the annotated type so that we can print the plain `std::int64_t{5}`.
`lmno` tries very hard to record compile-time results losslessly. More on `lmno::Const` can be read below


## The `lmno` Language

The actual programming language that `lmno` provides is inspired by BQN and APL.
It borrows symbology and terms from both, but also has a unique grammar and
tokenization strategy.


### Tokens

The tokenization rules are very simple to understand. An "identifier" includes
any C-like identifier.

```ebnf
DIGIT: '0-9'
ID_START: 'a-z' | 'A-Z' | '_'
ID_CHAR: ID_START | DIGIT
IDENT: ID_START ID_CHAR*
```

A "name" is more general. A *name* includes a C identifier, but also includes
any other Unicode codepoint with a few exceptions. Note that each individual codepoint is a
distinct *name* token, and not a sequence of codepoints for a single name.

```ebnf
NAME: IDENT | MOST_OTHER_CODEPOINTS
INTEGER: "¯"? DIGIT+
```

Excess whitespace between tokens is insignificant (including the newline). The
high-bar `¯` must be directly attached to the digits.


### Expressions

The expression grammar of `lmno` is extremely simple. An `lmno` program is a
sequence of expressions separated using the diamond "`⋄`" character:

```ebnf
stmt-seq: assign-expr ("⋄" stmt-seq)?
```

Each sequences expression is an `assign-expr`:

```ebnf
assign-expr: (NAME "←")? loose-expr
```

A `loose-expr` is any set of `main-expr` separated with an ASCII period:

```ebnf
loose-expr: main-expr ("." main-expr ("." loose-expr)?)?
```

The `main-expr` is where most code will live, and appears as:

```ebnf
main-expr: tight-expr (tight-expr main-expr?)?
```

There are no separators between `main-expr`s. The first and second elements are
both `tight-expr`:

```ebnf
tight-expr: strand-expr (":" strand-expr (":" tight-expr)?)?
```

> The meaning of *tight-expr* and *loose-expr* are explained in the next
> section.

Within the `tight-expr` are `strand-expr`s, borrowed from BQN:

```ebnf
strand-expr: primary-expr ("‿" primary-expr)*
```

These are used to create lists of expressions, e.g. `1‿2‿3` is an array
`[1, 2, 3]`. The "`‿`" character goes by several names. The official Unicode
name is *undertie*.

And finally, we have our *primary* expressions:

```ebnf
primary-expr: NAME | INTEGER | grouped-expr | block-expr | NOTHING
grouped-expr: "(" stmt-seq ")"
block-expr: "{" stmt-seq "}"
NOTHING: "·"
```


### Associativity and Precedence

`lmno` is strictly right-associative and is based on prefix and infix
expressions. There are three main "kinds" of expression: *single*, *prefix*, and
*infix*. A *single* expression is any strand expression or primary expression. A
*prefix* expression is any two expressions adjacent to each other under the
associativity rules, and an *infix* is any three expressions under the
associativity rules. The associativity rules do not allow the formation of any
larger groups.

For example, the following is a *prefix* expression:

```apl
¬ bar
```

Here, "`¬`" is the prefix of the expression "`bar`". Likewise we can form
*infix* expressions:

```apl
2 + 3
```

And now "`+`" is the infix between `2` and `3`. Importantly: "`+`" is an
expression that resolves to a binary function, and is *not* a special built-in
operator.

Combining the rules together, four expressions in sequence form a prefix
followed by an infix:

```apl
¬ 2 ≥ 3
```

In many languages, the "`¬`" would be an "operator" that binds "more tightly"
than "≥". **This is not the case in `lmno`**: The "`¬`" is just a prefix
function applied to the infix expression "`2 ≥ 3`". This would be parsed as the
following:

```apl
¬ (2 ≥ 3)
```

Likewise, all infix applications are right-associative. Order-of-operations does
not apply:

```py
2 × 3 + 7
# Parses as:
2 × (3 + 7)
```

To force a different parse, parenthesis must be used.


### Shifting Precedence with "`:`" and "`.`"

The *tight* expression and the *loose* expression separators are used to adjust
precedence without needing parenthesis (and I believe are unique to `lmno`).

A sequence of expressions separated by a colon "`:`" is equivalent to wrapping
those expressions in parenthesis and replacing the colon with a blank space.

```py
2 + |:bar - 5
# Equivalent to:
2 + (| bar) - 5
```

The `:` is *not* an infix operator, but mereley a precedence-shifting token.

Likewise, the "`.`" can be used for the opposite effect. A sequence of
expressions separated with a "`.`" is equivalent to surrounding everything
between/around the dots with parenthesis, and removing the dots entirely.

```py
2 ⊸ + . 5 ÷ ω
# Equivalent to:
(2 ⊸ +) (5 ÷ ω)
```


## More on `lmno::Const`

Consider the following example:

```c++
auto add_five = lmno::eval<"+⟜5">();
```

In this case, the expression `+⟜5` evaluates to a new invocable object that adds
`5` to its argument and returns it:

```c++
std::int64_t eight = add_five(3);
```

Note the lack of `unconst()`: This time, `lmno` will return a `std::int64_t`
directly, since it cannot know that the caller has passed a constant argument
(in this case, there *is* a constant `3`, but this appears to `add_five` as just
a plain `int`, which could be a variable or other runtime value).

If one wishes to give `lmno` more information about this const-ness, you can utilize the `lmno::Const` mechanism to guarantee `add_five` a constant value:

```c++
auto const_eight = add_five(lmno::Const<3>{});
```

To compile-time assert that `lmno` has returned a compile-time constant, one can
use the `lmno::typed_constant` concept as the deduction constraint on the
variable:

```c++
lmno::typed_constant auto definitely_const_eight = add_five(lmno::Const<3>{});
```

This will generate a compile-time error if the result of the evaluation fails to
generate an `lmno::Const` value for some reason.

On the other, hand, one can assert the opposite by using `lmno::variate`:

```c++
lmno::variate auto definitely_runtime_eight = add_five(3);
```

`lmno::variate` matches any type that *isn't* an `lmno::Const`.

When in doubt, `lmno::unconst` can be used to coerce the result to match
`lmno::variate`, regardless of whether the argument is a typed-constant or
already a variate.
