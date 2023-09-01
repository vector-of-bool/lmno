The |lmno| Grammar
##################

The tokenization rules for |lmno| are intentionally simplistic and easy to
understand.

An |lmno| program is a sequence of Unicode codepoints. Larger Unicode structures
are not considered.

.. default-role:: token

Token Kinds
***********

The following productions are whitespace-sensitive:

.. container:: grammar

  .. productionlist::
    ID_START : ALPHA | "_"
    ID_CONT  : `ID_START` | DIGIT
    IDENT : `ID_START` `ID_CONT`*
    INTEGER : ["¯"] DIGIT+
    WHITESPACE : " " | "\n" | "\t"
    SPECIAL : ";" | ":" | "." | "(" | ")"
            : "{" | "}" | "←" | "‿" | "¯"
            : "[" | "]" | " " | "·" |
            : `WHITESPACE` | DIGIT | ALPHA
    NAME : IDENT | NON-`SPECIAL`
    COMMENT : "(:" ... ":)"

The psuedo-token "``NON-SPECIAL``" refers to any single Unicode codepoint that
is not `SPECIAL`. This forms the basis of the `NAME` token.

.. default-role:: lmno
.. note::

  :token:`IDENT` includes any C-like identifier. An identifier begins with any
  ASCII-equivalent letter or an underscore, and is followed by zero or more
  ASCII letters, digits, or underscores. `foo`, `bar`, `foo_bar`, and `_bar9`
  are all identifiers, but `9foo` and `foo-bar` are *not* identifiers.

.. note::

  The ASCI hyphen "`-`" is *not* part of :token:`INTEGER`, but is actually
  parsed as a stand-alone :token:`NAME`. Negative :token:`INTEGER` literals are
  represented using an APL high-bar codepoint "``¯``". The high-bar *must* be
  attached to the digits, otherwise it is an invalid syntax: `9`, `30`, `42`,
  `¯7` are all numeric literals. `-9` is *not* a numeric literal, but is two
  separate tokens. The string "``¯ 4``" is *not* a numeric literal, and is
  actually invalid (The high-bar ``¯`` must be attached to the digits).

.. note::

  Other ``NON-``\ :token:`SPECIAL` codepoints do not merge together like the
  characters within an identifier. Instead, each codepoint stands as a lone
  name. e.g. The source string `⌽÷` is composed of *two* :token:`NAME`\ s: `⌽`
  and `÷`.


Grammar
*******

.. default-role:: token

The following productions are whitespace-insensitive (`WHITESPACE` and
`COMMENT`\ s between tokens are discarded).

.. container:: grammar

  .. productionlist::
    program       : `expr_seq`
    expr_seq      : `expr_assign` (";" `expr_assign`)*
    expr_assign   : `expr_dollar` ["←" `expr_dollar`]
    expr_dollar   : `expr_dollar_prefix` | `expr_dollar_infix` | `expr_main`
    expr_dollar_prefix : `expr_main` "$" `expr_dollar_infix`
    expr_dollar_infix : `expr_main` "$" `expr_main` "$" (`expr_dollar_infix` | `expr_main`)
    expr_main     : `expr_prefix` | `expr_infix` | `expr_colon`
    expr_prefix   : `expr_colon` `expr_infix`
    expr_infix    : `expr_colon` `expr_colon` (`expr_infix` | `expr_colon`)
    expr_colon    : `expr_strand` (":" `expr_strand`)*
    expr_strand   : `expr_primary` ("‿" `expr_primary`)*
    expr_primary  : `NAME` | `INTEGER` | `expr_group` | `expr_block`
    expr_group    : "(" `expr_seq` ")"
    expr_block    : "{" `expr_seq` "}"

.. note::

  The above productions `expr_dollar` and `expr_main` require indefinite
  lookahead to determine whether to accept the ``prefix`` or ``infix`` variant.
  For simplicity and performance of implementation, it is easier to parse a
  flat sequence of operand expressions and then regroup them as appropriate.
