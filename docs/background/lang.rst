How was the language designed?
##############################

.. default-role:: lmno
.. highlight:: lmno

The |lmno| parsing and grammar rules are inspired by the symbolic notation of
most array programming languages.


Why Symbolic?
*************

There are a few reasons to choose *symbolic* notation rather than a plain
lexical notation:

#. **Pragmatism**: The :cpp:`constexpr` interpreter isn't particularly fast, so
   having a language that is extremely terse is a huge benefit.

#. **Distinction**: C++ is a lexical language, so it "catches the eye" to see
   symbolic sub-programs within a larger context.

#. **Novelty**: It wouldn't be nearly as interesting if I just wrote yet-another
   JavaScript interpreter :)


Where do these symbols come from?
*********************************

The symbols chosen for the "standard library" of |lmno| are based on the symbols
from |BQN|, with a few symbols taken instead from APL (e.g. BQN uses `´` and
|backtick| for the "fold" and "scan" operators respectively, whereas |lmno| uses
the `/` and `\\` symbols from APL). The choice of symbols in BQN is related to
grammar and parseability, while the choices in |lmno| focus on the familiarity
and distinguishability of the latter (although |backtick| is also supported
since the `\\` character is special is non-raw C++strings).

Note: Except for a small subset of tokens, the |lmno| grammar doesn't attribute
meaning to any functions, operators, literals, or otherwise. Read more on the
grammar below.


Why is the grammar?
*******************

  but nobody ever asks: *how* is the grammar?

Grammar Troubles
================

APL suffers from a "deficiency" in its grammar: It's *context-sensitive*. That
is: You cannot actually parse any sequence of tokens without knowing their
larger meaning. For example:

.. code-block:: apl

  +/⍳5

.. default-role:: apl

Anyone familiar with the APL syntax will quickly see the structure of the above
snippet. It's structure can be made explicit with parentheses: `(+/)(⍳5)`

Colorization also helps: Syntax highlighters will call out the role of the
tokens such that a reader can understand how the tokens bind. In the above, `/`
is an *operator*, while `+` and `⍳` are both *functions*.

.. default-role:: lmno

|BQN| alleviates this ambiguity by introducing a context-free grammar. In
particular: without knowing the runtime properties of a token, it is possible to
assign an unambiguous "syntactic role" to a token or subexpression in such a way
that parsing can be done without additional context. For the BQN built-ins, the
*unary operators* are defined to be super-script-like (e.g. |backtick|, `´`,
`¨`, `⌜`, `⁼`) and the *infix operators* are defined as symbols containing a
circle (e.g. `∘`, `○`, `⟜`, `⊸`).


Simple Associativity
====================

Important to note: APL and BQN both have fairly simple precedence rules:

1. All functions are right-associative, i.e. `1×5-2` is the same as `1×(5-2)`
   (No "order of operations" is in play here).
2. All operators are *left*-associative, and have higher parsing precedence than
   function application. i.e. `5⊸×∘↕5` parses as `((5⊸×)∘↕) 5`
3. In APL, whether an operator is monadic (unary) or dyadic (binary-infix) is a
   matter of just knowing. In BQN, the aforementioned visual distinction was
   made.

|lmno| extents the simplicity of associativity even further: *All* expressions
are right-associative, and there is no syntactic distinction between "operators"
and functions (an "operator" can be thought of as a function that manipulates
other functions). This has both pros and cons.


No Precedence? No Associativity?
================================

I set out to have a parser that had as few "blessed" tokens as possible. At time
of writing, the only "blessed" tokens in |lmno| are seven from ACSII: `{}()$:;`
and three inherited directly from APL/BQN: `‿·←`. There *are* a few precedence
rules to note, though:

1. As usual, the parentheses `()` and curly-braces `{}` have the highest
   overriding precedence in the grammar.
2. The undertie/strand/ligature "`‿`" has the next-highest precedence::

     1‿2+3

   is the same as::

     (1‿2)+3

3. The colon `:` has the next highest precedence::

      foo:bar baz

   is equivalent to::

      (foo bar) baz

4. Placing two expressions adjacent to each other creates prefix/infix function
   application, and has the next highest precedence.
5. The dollar `$` infix has lower precedence, and is roughly based on the dollar
   from Haskell.
6. Assignment "`←`" binds next.
7. The lowest precedence goes to the semicolon `;`, which acts as a statement
   separator.


Comments
========

|lmno| defines comments differently from either language. Because the colon `:`
is never the valid beginning of an expression, the token sequence "`(:`" is used
to "begin" a comment. The tokenizer will consume a balanced-symbol sequence
until it encounters the symmetric comment-closing sequence "`:)`". This allows
the `#` symbol to be used for a different meaning.

.. note::

   This has the added benefit that well-commented code is always smiling back at
   the reader ``:)``


Precedence and Structure
************************

There are very few precedence rules in the |lmno| grammar. The main thing to
note are familar grouping with parenthesis (`(` and `)`), grouping with braces
(`{` and `}`), grouping with brackets (`[` and `]`), and two additional
structural tokens: dollar "`$`" and colon "`:`". The meanings of `$` and `:`
will be explained later after we first explore the expression *kinds*.


Expression Kinds
================

The |lmno| language is expression-oriented, and all expressions can be
grouped into one of three kinds:

- *Singular* expressions: which includes stranding expressions and all primary
  expressions (which includes groups and function blocks)::

    foo         (: An identifier name :)
    ω           (: Other codepoint name :)
    ∞           (: Also a name :)
    12          (: An integer :)
    ¯42         (: Also an integer :)
    1‿2‿3       (: Strand expression :)
    (2 + 4)     (: A group expression with something else inside :)
    {α + ω}     (: A function block :)

- *Prefix* expressions: Also called "monadic" function application. Is the
  application of expression on the left to an expression on its right, with no
  left-hand operand expression::

    foo bar         (: Apply `foo` with argument `bar` :)
    foo (2 + 3)     (: Apply `foo` to a grouped subexpression :)
    foo (÷ 4)       (: Apply `foo` to another grouped prefix expression :)
    ⌽ seq ~ other   (: Apply `⌽` to an infix expression (see below) :)
    (foo bar) baz   (: Apply the result of `foo bar` to `baz` :)

- *Infix* expressions: Also called "dyadic" function application. This is the
  application of an expression in the middle of two operands. The center operand
  acts as the function, while the left-hand and right-hand act as the
  arguments::

    foo bar baz     (: Apply `bar` with left-hand `foo` and right-hand `baz` :)
    g f h           (: Apply `f` to `g` and `h` :)
    2 + 3           (: Use the `+` function with left-hand `2` and right-hand `3` :)
    2 + 3 × 7       (: Two infix expressions in a tree :)
    2 + (3 × 7)     (: Equivalent to the previous, with an explicit grouping :)

Note that infix and prefix expressions are *always* right-associative! That is:
`2 + 3 × 7` is parsed as `2 + (3 × 7)`! Order-of-operations does not apply here.

Note also that none of the above examples use `$` nor `:`, which introduce some
additional precedence-shifting rules.


*Tight* Expressions with `:`
============================

The ASCII colon `:` character is can be used as an infix between two primary
strand/primary expressions to bind them together as-if they were wrapped in
parentheses::

    (: This: :)
    foo:bar
    (: is equivalent to this: :)
    (foo bar)

The parsing of `:` has higher precedence than regular function application, but
lower precedence than stranding expressions::

    (: This: :)
    foo bar:baz quux
    (: Is the same as this: :)
    foo (bar baz) quux

**Importantly**, the colon `:` cannot be used to form infix expressions.
Instead, it is right-associative with itself and will only form
prefix-expressions::

    (: This: :)
    foo bar:baz:quux zop
    (: Is equivalent to: :)
    foo (bar (baz quux)) zop

The colon `:` is not a named function, and not an operator. It is only
structural.


*Loose* Expressions with `$`
============================

The dollar sign `$` is inspired by the infix operator from Haskell, although in
|lmno| the dollar `$` is purely structural, and has no operator semantics. The
dollar `$` is used to regroup expressions, essentially wrapping the surrounding
"operands" in parentheses, with the lowest precedence::

    (: This: :)
    foo bar baz $ quux zop zing
    (: Is equivalent to this: :)
    (foo bar baz) (quux zop zing)

Unlike `:`, the `$` infix *can* be used to form infix expressions::

    (: This: :)
    foo bar $ baz quux $ zop zing
    (: Becomes: :)
    (foo bar) (baz quux) (zop zing)

Like everything else, the `$` is right-associative::

    (: This :)
    foo $ bar 9 $ baz 7 $ quux 31
    (: Becomes: :)
    foo ((bar 9) (baz 7) (quux 31))


Downsides to the Grammar
************************

As mentioned, inspiration languages used strong-left-binding operators to
compose functions, while |lmno| makes no distinction and between operators and
functions. It is important to acknowledge that this decision has downsides. The
biggest is that otherwise "terse" expressions require more disambiguation. For
example, the following BQN expression::

   +´↕

parses as::

   ((+´)↕)

On the other hand, a "similar-looking" |lmno| expression::

   /+∘⍳

parses as::

   (/ (+ ∘ ⍳))

which is not at all equivalent to the BQN code.

A simple solution is to use parentheses to control the operand binding::

   (/+)∘⍳

But this is definitely not as "pretty" as the BQN expression. As a compromise,
|lmno| comes with additional precedence-shifting rules that can make things
a bit better. There are actually two options for the rewrite here::

   /+$∘⍳    (: Using '$' to "loosen" precedence :)
   (/+)∘(⍳) (: Equivalent expression :)

   /:+∘⍳    (: Using ':' to tighten precedence :)
   (/+)∘⍳   (: Equivalent :)

Again: Not nearly as pretty as BQN, but an improvment over fully parenthesizing
everything.


Function Composition (Trains)
*****************************

You may note in the above examples that |lmno| requires an explicit *atop* `∘`
between the two function operands, whereas in BQN the *atop* is implicit.

In many symbolic languages, "applying" a function to another function would
implicitly form a composition of those functions. For example, in BQN, "`-×`"
means "multiply, then negate." Similarly, the famous "four-token-average"
expression: "`+´÷≠`" could be parsed as "apply `÷` with a left-hand of `+´` and
a right-hand of `≠`." This makes sense in some mathematical notations, and is
the basis of the *3-train* or *fork*.

Common in mathematical notation, function composition is represented by the
small-circle "|∘|" or "jot" symbol:

.. math::

   (f ∘ g)(x) = f(g(x))

In |lmno| the `∘` infix operator is pronounced "atop" (inherited from BQN), and
conveys similar meaning to its common notational definition. We can formalize
the meaning of |∘| as a simple higher-order function in the lambda calculus:

.. math::

   \begin{align}
   f &: A → B \\
   g &: A → B \\
   ∘ &: (A → B) → (B → C) → A → C \\
   ∘ &= λf.λg.λx.f(g(x))
   \end{align}

.. pull-quote:: *(Forgive my abuses of notation)*

In standard combinatory logic, this is the :math:`B` combinator. Of course, we
do not use prefix-application of "|∘|" as we would with :math:`B` in the lambda
calculus, but instead |∘| is an *infix* "operator", where the left-hand operand
acts as the first argument and the right-hand acts as the second. This is the
same in |lmno|.

One could imagine a mathematical convention of "higher-order-application" of a
function that automatically forms such a composition:

.. math::

   \begin{align}
   f &: A → B \\
   g &: B → C \\
   f\ g  &= f ∘ g \\
   \end{align}

This lends a sort of "intuition" to the syntax of function application:

.. math::

   \begin{align}
   f\ g\ x &= f(g(x)) \\
   f\ (g\ x) &= f(g(x)) \\
   (f\ g)\ x &= (f ∘ g)\ x \\
             &= f(g(x))
   \end{align}

Of course, |lmno| is a language focused on *infix*-expressions, and we would
like to be able to compose these in a similar manner. One could simply extend
this "higher-order-application" to include arbitrary infix functions:

.. math::

   \begin{align}
   f \oplus g &= λx.(f\ x) \oplus (g\ x)
   \end{align}

In the sense of the lambda calculus, for two unary functions :math:`f` and
:math:`g` and a binary function :math:`h`, the expression :math:`(hfg)` returns a
new composed function, and we can give this a more formal definition:

.. math::

   \begin{align}
   f &: A → B \\
   g &: B → C \\
   h &: B → C → D\\
   h f g &: A → D\\
   h f g &= λx.h (fx) (gx)
   \end{align}

This form involving three functions is likely less familiar, and there is no
dedicated symbol for forming such composition (as there is with |∘|). In
combinatory logic, this composition is sometimes known as the "phi" combinator:
|Phi|. Because a capital ϕ can be easily confused with the APL "vertical stile"
⌽ symbol, |lmno| (and these docs) refer to it using lowercase: |phi|. As with
the infix jot "|∘|", we can define the meaning of |phi| as a higher-order
function:

.. math::

  \begin{align}
  φ &: (B → C → D) → (A → B) → (A → C) → A → D \\
  φ &= λh.λf.λg.λx.h(f(x), g(x))
  \end{align}

In |lmno|, the symbol `φ` is used with this exact meaning::

  avg ← /:+φ:÷#
  val ← avg 1‿2‿3‿4 (: val = (5÷2) :)

That may be "kinda neat", but `/:+φ:÷#` is quite a few more symbols than `+´÷≠`,
where the φ-combinator is implicit. These implicit function compositions are
known as *trains* in the APL family, and are a hallmark feature of terse tacit
programming. So can be get away with the equivalent `/:+÷#` in |lmno|?


Problem: How to Best Implement This
===================================

At time of writing, |lmno| does not support the implicit function composition of
trains. This is not an "intentional" omission, but rather it is an open question
as to the optimal way to implement such a feature.

Unlike APL and BQN, expressions do not have a "kind"/"role"/"part of speech".
That is, the |lmno| parser will see this::

  4 + 5

as an infix application of `+` with a left-hand of `4` and a right-hand of `5`,
while this::

  × + ⌊

is an infix application of `+` with a left-hand of `×` and a right-hand of `⌊`.
They parse the same! And so how would we transform this construct into the
explicit equivalent `×φ:+⌊`?

Ultimately, there are four possibilities:

1. Make it the responsibility of the infix function to transform as-if by |phi|
   when given function operands.
2. Make the :func:`lmno::invoke` API detect the construct and automatically
   apply |phi|.
3. Have the semantic analysis/evaluator detect the construct and insert a |phi|
   automatically.
4. Don't support trains.

.. rubric:: Option #1

Option #1 has allure in being "clean" by delegating the train machinery into the
functions themselves. This would allow forming trains within C++ code natural:

.. code-block:: c++

  constexpr auto fork = stdlib::plus(stdlib::times, stdlib::min)
  // "fork" is now a 3-train function of `×+⌊`
  static_assert(fork(3, 4) == 15);

But this has the drawback of possibly making the implementation of functions
more complex, since they will now need to differenciate between function
operands and value operands. Additionally, user-defined functions will also
be required to know about this machinery!

An option to make this more appealing is to use a base class that abstracts the
automatic-composition by using a explicit-object parameter:

.. code-block:: c++

  struct composable_invocable {
    template <typename Self, typename... Args>
    auto operator()(this Self&& self, Args&&... args) {
      if constexpr (make_composed_func<Self, Args...>) {
        return compose_func(self, args...);
      } else {
        return self.regular_invoke(args...);
      }
    }
  };

Because implementations of the explicit-object parameter feature are still
unavailable in GCC and Clang, this design space has not yet been explored fully.


.. rubric:: Option #2

.. default-role:: cpp:expr
.. namespace:: lmno

Having `invoke` manage the function composition is appealing in that `invoke` is
already "the way" to handle "magic" function application. This is *unappealing*
in that `invoke` is already very heavy, and it would fail were a caller to
bypass `invoke` and call an object directly.


.. rubric:: Option #3

|lmno| supports swapping out the semantics/evaluator/compiler of the language,
so it would be tempting to give this work to that semantic analyzer. This would
allow the formation of trains without needing to "invoke" any objects, which may
represent not-yet-available resources.


.. rubric:: Option #4

This is the current approach: Don't support trains.

The appeal of this approach is that it leaves time to develop and play with
possible designs. Once a particular design is deployed, it would be very
difficult to *un*-deploy.
