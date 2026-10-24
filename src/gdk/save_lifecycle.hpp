#pragma once
#include <cstdint>

namespace mcd2::gdk {
constexpr bool closesSaveOwner(bool terminal,bool resize,std::uintptr_t owner,std::uintptr_t destroyed) {
    return !terminal&&!resize&&owner&&owner==destroyed;
}
}
