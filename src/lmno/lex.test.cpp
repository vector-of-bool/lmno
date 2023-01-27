#include "./lex.hpp"

#include <concepts>

namespace lex = lmno::lex;
using lex::token;
using lex::token_list;

namespace {

using Toks1 = lex::tokenize_t<"foo bar">;

static_assert(std::same_as<Toks1, token_list<"foo", "bar">>);
static_assert(std::same_as<lex::tokenize_t<"÷√π∞·">, token_list<"÷", "√", "π", "∞", "·">>);

static_assert(std::same_as<lex::tokenize_t<"                                ÷    baz ">,
                           token_list<"÷", "baz">>);

static_assert(
    std::same_as<
        lex::tokenize_t<"                      bar baz ∞                                f    ">,
        token_list<"bar", "baz", "∞", "f">>);

using t3 = lex::tokenize_t<"1⊸⍳(˜∘)1⊸·/+⟜÷∘2⊸^">;

static_assert(std::same_as<lex::tokenize_t<"2⊸^">, token_list<"2", "⊸", "^">>);

using big1 [[maybe_unused]] = lex::tokenize_t<
    "÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞"
    "·÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞··÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞··÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷"
    "√π∞·÷√π∞·÷√π∞··÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞··÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞··÷√π∞·÷√"
    "π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞··÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞··÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞··÷√π∞·÷√π∞·÷√π"
    "∞·÷√π∞·÷√π∞··÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞··÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞··÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞··÷√π"
    "∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞··÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞··÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞··÷√π∞·÷√π∞·÷√π∞·÷√π∞"
    "∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞··÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞··÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞··÷√π∞·÷√π∞·÷√π∞·÷√π∞"
    "∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞··÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞··÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞··÷√π∞·÷√π∞·÷√π∞·÷√π∞"
    "∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞··÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞··÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞··÷√π∞·÷√π∞·÷√π∞·÷√π∞"
    "∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞··÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞··÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞··÷√π∞·÷√π∞·÷√π∞·÷√π∞"
    "·÷√π∞··÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞··÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞··÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞··÷√π∞·÷√π∞"
    "·÷√π∞·÷√π∞·÷√π∞··÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞··÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞··÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞·"
    "·÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞··÷√π∞·÷√π∞·÷√π∞·÷√π∞·÷√π∞··÷√π∞·÷√π∞·÷√π∞·">;

using big2 [[maybe_unused]] = lmno::lex::tokenize_t<
    "this_is_a_long_word_token this_is_a_long_word_token this_is_a_long_word_token "
    "this_is_a_long_word_token this_is_a_long_word_token this_is_a_long_word_token "
    "this_is_a_long_word_token this_is_a_long_word_token this_is_a_long_word_token "
    "this_is_a_long_word_token this_is_a_long_word_token this_is_a_long_word_token "
    "this_is_a_long_word_token this_is_a_long_word_token this_is_a_long_word_token "
    "this_is_a_long_word_token this_is_a_long_word_token this_is_a_long_word_token "
    "this_is_a_long_word_token this_is_a_long_word_token this_is_a_long_word_token "
    "this_is_a_long_word_token this_is_a_long_word_token this_is_a_long_word_token "
    "this_is_a_long_word_token this_is_a_long_word_token this_is_a_long_word_token "
    "this_is_a_long_word_token this_is_a_long_word_token this_is_a_long_word_token "
    "this_is_a_long_word_token this_is_a_long_word_token this_is_a_long_word_token "
    "this_is_a_long_word_token this_is_a_long_word_token this_is_a_long_word_token ">;

}  // namespace

int main() {}