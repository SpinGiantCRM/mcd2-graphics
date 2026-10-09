#pragma once
#include <memory>

namespace mcd2::process_exit {
// Ordinary unload destroys the value. Final process termination cannot safely
// join stopped threads or call SDKs whose DLL teardown has already started.
// Only that terminal path leaves destruction to the operating system.
template<class T, bool (*Terminating)()>
class Lifetime {
    std::unique_ptr<T> value_ = std::make_unique<T>();
public:
    Lifetime() = default;
    Lifetime(const Lifetime&) = delete;
    Lifetime& operator=(const Lifetime&) = delete;
    ~Lifetime() { if (Terminating()) value_.release(); }
    T& get() { return *value_; }
};
}
