#pragma once

#include "./error.hpp"
#include "./lex.hpp"

namespace lmno {

template <cx_str Name>
constexpr auto make_undefined_name() {
    return err::make_error<"The name {:'} is not defined", Name>();
}

template <lex::token Name>
constexpr auto define = make_undefined_name<cx_str<Name.size()>{std::string_view(Name)}>();

}  // namespace lmno
