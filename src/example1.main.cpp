#include <iostream>

// The main header with lmno::eval
#include <lmno/eval.hpp>
#include <lmno/stddefs.hpp>

int main() {
    // Invoke the result
    auto result = lmno::unconst(lmno::eval<"2+3">());
    std::cout << "The sum of 2 and 3 is " << result << '\n';
}
