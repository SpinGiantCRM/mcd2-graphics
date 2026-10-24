#pragma once
#include <cstdint>
#include <condition_variable>
#include <mutex>
#include <chrono>

namespace mcd2::gdk {
class WorkerCompletion {
    std::mutex mutex_;std::condition_variable wake_;bool finished_=false;
public:
    void finish(){std::lock_guard lock(mutex_);finished_=true;wake_.notify_all();}
    template<class Rep,class Period>bool wait(std::chrono::duration<Rep,Period> budget){
        std::unique_lock lock(mutex_);return wake_.wait_for(lock,budget,[&]{return finished_;});
    }
};
constexpr bool closesSaveOwner(bool terminal,bool resize,std::uintptr_t owner,std::uintptr_t destroyed) {
    return !terminal&&!resize&&owner&&owner==destroyed;
}
}
