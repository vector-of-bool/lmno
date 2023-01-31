#pragma once

namespace lmno {

template <auto Func>
struct func_wrap : decltype(Func) {
    using decltype(Func)::operator();
};

}  // namespace lmno
