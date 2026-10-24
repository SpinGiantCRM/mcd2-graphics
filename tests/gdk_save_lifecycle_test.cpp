#include "../src/gdk/save_lifecycle.hpp"
#include <cassert>
#include <initializer_list>
#include <future>
#include <thread>
int main() {
    using mcd2::gdk::closesSaveOwner;
    for(bool terminal:{false,true})for(bool resize:{false,true})
        for(std::uintptr_t owner=0;owner<3;++owner)for(std::uintptr_t destroyed=0;destroyed<3;++destroyed)
            assert(closesSaveOwner(terminal,resize,owner,destroyed)==(!terminal&&!resize&&owner!=0&&owner==destroyed));
    mcd2::gdk::WorkerCompletion completion;
    std::promise<void> release;auto blocked=release.get_future();
    std::thread worker([&]{blocked.wait();completion.finish();});
    // A synchronous store call cannot be cancelled: bounded wait retains ownership.
    assert(!completion.wait(std::chrono::milliseconds(1)));
    release.set_value();assert(completion.wait(std::chrono::seconds(5)));worker.join();
    assert(completion.wait(std::chrono::milliseconds(0)));
}
