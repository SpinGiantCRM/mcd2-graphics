#include "../src/gdk/save_lifecycle.hpp"
#include <cassert>
#include <initializer_list>
int main() {
    using mcd2::gdk::closesSaveOwner;
    for(bool terminal:{false,true})for(bool resize:{false,true})
        for(std::uintptr_t owner=0;owner<3;++owner)for(std::uintptr_t destroyed=0;destroyed<3;++destroyed)
            assert(closesSaveOwner(terminal,resize,owner,destroyed)==(!terminal&&!resize&&owner!=0&&owner==destroyed));
}
