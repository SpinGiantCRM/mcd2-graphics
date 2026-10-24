
// Portable doubles; actual export and retirement block are inserted by the test.
#ifdef __declspec
#undef __declspec
#endif
#define __declspec(x)
#define MCD2_NATIVE_FG_GUIDES 1
#include <memory>
#include <mutex>
#include <condition_variable>
#include <set>
#include <vector>
#include <thread>
#include <stdexcept>
#include <iostream>
namespace mcd2::process_exit {bool exiting=false;bool terminating(){return exiting;}}
std::recursive_mutex lock;std::mutex signals;std::condition_variable changed;bool releaseEntered=false,allowRelease=false;
struct Resource {void Release(){std::unique_lock guard(signals);releaseEntered=true;changed.notify_all();changed.wait(guard,[]{return allowRelease;});}};
struct Cache {std::set<int>recordings,entries;bool blocked=false;void clear_after_idle(){}};
struct Generation {void*queue=nullptr;Cache borrows;bool retiring=true;struct{std::vector<Resource*>resources;}owned;};
namespace native_fg {std::unique_ptr<Generation>generation;bool retirementReleasing=false;unsigned retirements=0;
void admit(){std::unique_lock guard(lock);bool eligible=true;void*q=nullptr;
// ACTUAL_CREATION
}
void retire(){std::unique_lock guard(lock);auto&c=*generation;auto&cache=c.borrows;
// ACTUAL_RELEASE
}}
// ACTUAL_EXPORT
void require(bool b,const char*reason){if(!b)throw std::runtime_error(reason);}
int main(){try{
 require(mcd2_native_fg_retired_v1()==1,"empty producer not retired");native_fg::generation=std::make_unique<Generation>();require(mcd2_native_fg_retired_v1()==0,"live generation called retired");
 Resource resource;native_fg::generation->owned.resources.push_back(&resource);
 std::thread cleanup([]{native_fg::retire();});{std::unique_lock guard(signals);changed.wait(guard,[]{return releaseEntered;});}
 require(!native_fg::generation,"fixture missed moved-generation window");int during=mcd2_native_fg_retired_v1();native_fg::admit();require(!native_fg::generation,"new generation overlapped unlocked retirement");
 {std::lock_guard guard(signals);allowRelease=true;changed.notify_all();}cleanup.join();require(during==0,"unlocked COM release falsely reported retired");require(mcd2_native_fg_retired_v1()==1&&native_fg::retirements==1,"completed release not retired");
 int busy=0;{std::unique_lock guard(lock);std::thread reader([&]{busy=mcd2_native_fg_retired_v1();});reader.join();}require(busy==-1,"readback blocked or claimed busy owner retired");
 mcd2::process_exit::exiting=true;require(mcd2_native_fg_retired_v1()==-1,"termination accepted as retirement");
 std::cout<<"Actual Native FG readback: live, moved/unlocked release, busy and termination decline; only complete retirement succeeds\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
