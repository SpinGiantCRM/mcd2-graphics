#include "../src/providers/shared_sdk_shutdown.hpp"
#include <cassert>
#include <iostream>

int main() {
    using namespace mcd2::providers;
    unsigned admitted = 0;
    for (unsigned mask = 0; mask < 64; ++mask) {
        SharedSdkShutdownAdmission a{
            bool(mask & 1), bool(mask & 2), bool(mask & 4),
            bool(mask & 8), bool(mask & 16), bool(mask & 32)};
        const bool expected = mask == 30;
        assert(admitSharedSdkShutdown(a) == expected);
        admitted += admitSharedSdkShutdown(a);
        if (expected) {
            a.alreadyIssued = true;
            assert(!admitSharedSdkShutdown(a));
        }
    }
    assert(admitted == 1);
    assert(!admitSharedSdkShutdown({}));
    std::cout << "64 shared SDK retirement combinations passed\n";
}
