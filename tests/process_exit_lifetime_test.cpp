#include "../src/latency/process_lifetime.hpp"
#include "../src/latency/token_coordinator.hpp"
#include <cassert>
#include <cstdio>

static bool exiting = false;
static bool terminating() { return exiting; }
struct Value {
    static inline unsigned destroyed = 0;
    ~Value() { ++destroyed; }
};
struct Provider : mcd2::streamline_reflex::Provider {
    unsigned releases = 0;
    bool mode(mcd2::streamline_reflex::Mode) override { return true; }
    void* begin(std::uint32_t) override { return nullptr; }
    std::uint32_t index(void*) override { return 0; }
    bool sleep(void*) override { return false; }
    bool marker(void*, mcd2::streamline_reflex::Marker) override { return false; }
    void release() override { ++releases; }
};
int main() {
    using mcd2::process_exit::Lifetime;
    { Lifetime<Value, terminating> owner; }
    assert(Value::destroyed == 1); // Ordinary unload still cleans up.
    Value* retained = nullptr;
    {
        Lifetime<Value, terminating> owner;
        retained = &owner.get();
        exiting = true;
    }
    assert(Value::destroyed == 1);
    exiting = false;
    delete retained; // Fixture cleanup, never part of final process exit.
    assert(Value::destroyed == 2);

    using Coordinator = mcd2::streamline_reflex::TokenCoordinator;
    Provider provider;
    {
        Lifetime<std::shared_ptr<Coordinator>, terminating> owner;
        owner.get() = std::make_shared<Coordinator>(provider);
        assert(owner.get()->attach());
    }
    assert(provider.releases == 1);
    std::shared_ptr<Coordinator>* held = nullptr;
    {
        Lifetime<std::shared_ptr<Coordinator>, terminating> owner;
        owner.get() = std::make_shared<Coordinator>(provider);
        assert(owner.get()->attach());
        held = &owner.get();
        exiting = true;
    }
    assert(provider.releases == 1); // No SDK call from static destruction.
    exiting = false;
    (*held)->shutdown();
    (*held)->drain();
    delete held;
    assert(provider.releases == 2);
    std::puts("Process-exit retention and ordinary SDK cleanup passed.");
}
