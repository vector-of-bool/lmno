The Evaluator
#############

.. default-domain:: cpp
.. default-role:: cpp
.. highlight:: cpp
.. cpp:namespace:: lmno

After :doc:`parsing a program <parser>`, we are ready to evaluate it. The
current |lmno| repository does not have a separate "compile" stage, and instead
relies on the C++ compiler itself to emit compiled code.

The evaluator accepts an AST node, a "semantics" object, and a name lookup
context as input, and returns the result of evaluating the encoded expression.
Importantly, because


The `evaluate` Function
***********************

`lmno::evaluate` is a callable object that accepts three arguments:

- `code` - The actual AST to evaluate.
- `sema` - A "semantics" implementation.
- `ctx` - A name-lookup context.

The default evaluator and `default_sema` are simply AST walking code. The `ctx`
object will be covered later.

`evaluate` simply calls and returns `sema.evaluate(ctx, code)`.


The `default_sema` Semantics
****************************

`default_sema` provides `evaluate()` methods for each AST node, so calling
`evaluate(ctx, code)` will match the `evaluate()` implementation based on the
content of the AST node `code`.

Example: Evaluate a Literal Constant
====================================

The parser will emit instances of :expr:`Const<>` whenever it encounters a
numeric literal. `default_sema::evaluate` implements a very simple handler for
this::

  template <typed_constant C>
  constexpr C evaluate(auto&&, C val) const noexcept {
    return val;
  }

.. seealso:: :concept:`typed_constant` - Matches "typed constant" types


Example: Evaluate a `dyad` (infix) Expression
=============================================

The evaluation of `dyad` is more complex::

  template <typename Left, typename Mid, typename Right>
  constexpr auto evaluate(const auto& ctx, ast::dyad<Left, Mid, Right>) const noexcept {
    decltype(auto) left  = this->evaluate(ctx, Left{});
    decltype(auto) fn    = this->evaluate(ctx, Mid{});
    decltype(auto) right = this->evaluate(ctx, Right{});
    if constexpr (LMNO_IS_ERROR(left)) {
      return left;
    } else if constexpr (LMNO_IS_ERROR(right)) {
      return right;
    } else if constexpr (LMNO_IS_ERROR(fn)) {
      return fn;
    } else {
      return invoke(FWD(fn), FWD(left), FWD(right));
    }
  }

The operands are evaluated from left to right. The `LMNO_IS_ERROR` macro simply
checks whether the operand satisfies :concept:`lmno::err::any_error`, which
signifies that evaluation of that subexpression caused an error. After
evaluating each operand, we call :func:`lmno::invoke` with the `fn` function,
and `left` and `right` as the first and second arguments, respectively. The
actual behavior of calling `fn` is not implemented the `default_sema`.


Evaluate a "Statement Sequence"
===============================

The `stmt_seq` AST node represents a sequence of expressions (yeah, the name is
wrong, oh well).

Evaluating a `stmt_seq` involves four functions: First, `evaluate` catches
`stmt_seq` and then calls `evaluate_stmts`::

  template <typename... Stmts>
  constexpr decltype(auto) evaluate(const auto& context, ast::stmt_seq<Stmts...> seq) const {
    return this->evaluate_stmts(context, seq);
  }

There are three overloads of `evaluate_stmts`. The first overload is the fixed
point case of a single statement, which simply evaluates the single node::

  template <typename Final>
  constexpr decltype(auto) evaluate_stmts(const auto& context, ast::stmt_seq<Final>) const {
    return this->evaluate(context, Final{});
  }

The second overload handles any expression except for assignments::

  template <typename Head, typename... Tail>
  constexpr auto evaluate_stmts(const auto& context, ast::stmt_seq<Head, Tail...>) const {
    auto&& el = this->evaluate(context, Head{});
    if constexpr (LMNO_IS_ERROR(el)) {
      return el;
    } else {
      return this->evaluate_stmts(context, ast::stmt_seq<Tail...>{});
    }
  }

It simply evaluates the expression, intercepts it if it is an error, then
discards that value and evaluates the remainder of the sequence by synthesizing
a new AST node that contains the remainder of the expressions in the sequence.

The third overload of `evaluate_stmts` handles assignment expressions only. See
the next section for information on that.


Performing an Assignment
========================

The "assignment" is not implemented as an infix operator, but as its own AST
node. Assignment binds the result of a subexpression to a name that can be
referrenced in subsequence expressions. Assignment only has an effect if it is a
non-final element in a `stmt_seq` sequence, since otherwise the name being bound
is simply be discarded.

Assignment is handled by `evaluate_stmts`::

  template <typename ID, typename RHS, typename Peek, typename... Tail>
  constexpr auto evaluate_stmts(const auto& in_ctx,
                                ast::stmt_seq<ast::assignment<ID, RHS>,
                                              Peek,
                                              Tail...>) const {
    auto&& value = this->evaluate(in_ctx, RHS{});
    if constexpr (LMNO_IS_ERROR(value)) {
      return value;
    } else {
      auto new_ctx = this->bind_assignment(in_ctx, ID{}, FWD(value));
      return this->evaluate_stmts(new_ctx, ast::stmt_seq<Peek, Tail...>{});
    }
  }

The `Peek` name is only required to ensure that the assignment is not the final
element in the sequence, and is simply forwarded on to the next evaluation.

Instead of discarding or returning the result of the subexpression `RHS`, we
bind it to the context::

  template <lex::token Name>
  constexpr decltype(auto)
  bind_assignment(const auto& context, ast::name<Name>, auto&& value) const {
      return context.bind(lmno::make_named<Name>(FWD(value)));
  }

Name contexts are immutable, especially because we can't allocate key-value maps
at compile time. This isn't a problem, though: Instead, the context `bind()`
method returns a copy of the context with the new named value bound in scope.
Subsequent name lookups on the returned context will find the bound value.


Creating Closures
=================

|lmno| "blocks" are snippets of code enclosed in braces `{}`. When the evaluator
encounters a block AST node, it does not evaluate the content, but instead
creates a closure object::

  template <typename Code, typename Ctx>
  constexpr auto evaluate(const Ctx& context, ast::block<Code>) const {
    auto c = lmno::closure<Code, default_sema, Ctx>{*this, context};
    return c;
  }

The `closure` is a class template parameterized on the AST node within the
braces, the `sema` that will be evaluated, and the context in which the block
node appears. Binding the context into the closure allows the closure to refer
to names in the enclosing scope.

`closure` itself is a regular C++ callable object, and it can be called with one
or two arguments. The two-argument `operator()` looks like this::

  constexpr decltype(auto) operator()(auto&& w, auto&& x) const {
    auto inner = _bound.bind(lmno::make_named<"α">(FWD(w)),
                             lmno::make_named<"ω">(FWD(x)));
    return invoke(evaluate, Code{}, _sema, inner);
  }

`_bound` is the name of the context that was bound when the closure was created,
`Code` is the AST node within the closure, and `_sema` is the sema object that
created the closure.

The closure will first bind two names into a new context: :lmno:`α` is the name
of the first argument, and :lmno:`ω` is the name of the second argument. This
allows code within the block to refer to the parameters when the evaluator
evaluates names in the context.

We then simply invoke the `evaluate` function with the AST, the semantics
object, and the temporary context we just created.


Name Lookup
***********

In |lmno|, all non-special tokens are treated as names to be looked up during
evaluation. This includes the symbolic characters that name many operators and
functions: They are not treated special by the parser nor by the evaluator.

The `default_sema` object evaluates an `ast::name` node by simply calling `get`
on the context, with the name passed as the sole *template* argument::

  template <lex::token Name>
  constexpr decltype(auto) evaluate(const auto& context, ast::name<Name>) const noexcept {
    return context.template get<Name>();
  }

The evaluator does not introspect the `context.get<>()` beyond this point.
Instead it is up to the context to decide how a name is resolved.

To read more, see :doc:`context`.