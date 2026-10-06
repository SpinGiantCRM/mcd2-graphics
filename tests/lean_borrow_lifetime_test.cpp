#include <array>
#include <cassert>
#include <cstdint>
#include <map>
#include <set>
#include <tuple>
#include <vector>
namespace a { struct command_list {}; }
struct ID3D12DescriptorHeap { void Release() {} };
struct ID3D12Resource { void Release() {} };
struct ID3D12Fence { void Release() {} };
#include "lean_borrow_lifetime.hpp"
int main() {
 LeanBorrowCache cache;
 a::command_list list, second;
 LeanBorrowKey key{1,2,3,4,5,6,7,8};
 cache.entries.emplace(key, LeanBorrowEntry{});
 cache.record(&list,key,1);cache.record(&list,key,1);
 cache.record(&second,key,1);
 assert(cache.entries.at(key).recordings==2);
 // A pre-call Reset alone never releases a potentially replayable recording.
 cache.reset_pending.insert(&list);
 cache.retire_ready(100,10);
 assert(cache.recordings.size()==2 && cache.ready_to_release.empty());
 // A subsequent recording operation need not bind a PSO (barrier-only list).
 auto *pending=&list;
 if(cache.begin_recording(&list) && pending==&list)pending=nullptr;
 assert(pending==nullptr);
 assert(cache.recordings.size()==1 && cache.entries.at(key).recordings==1);
 assert(cache.entries.at(key).awaiting_signal);
 assert(!cache.begin_recording(&list)); // No double decrement.
 assert(cache.entries.at(key).recordings==1 && !cache.blocked);
 pending=&second;
 assert(!cache.begin_recording(&second) && pending==&second);
 cache.reset_pending.insert(&second);
 if(cache.begin_recording(&second) && pending==&second)pending=nullptr;
 assert(pending==nullptr);
 assert(cache.recordings.empty() && cache.entries.at(key).recordings==0);
 cache.retire_ready(100,10);assert(cache.ready_to_release.empty());
 // Retirement still requires a post-invalidation signal and completed fence.
 auto &entry=cache.entries.at(key);entry.awaiting_signal=false;entry.retirement_fence=101;
 cache.retire_ready(100,10);assert(cache.ready_to_release.empty());
 cache.retire_ready(UINT64_MAX,10);assert(cache.ready_to_release.empty());
 cache.retire_ready(101,10);assert(cache.entries.empty() && cache.retired==1);
 assert(cache.ready_to_release.size()==1);
}
