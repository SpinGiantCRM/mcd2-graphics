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
 // A failed Reset leaves the original recording replayable and retained.
 cache.ready_to_release.clear();cache.entries.emplace(key,LeanBorrowEntry{});cache.record(&list,key,20);
 cache.note_reset(&list,true,10);assert(!cache.confirm_reset(&list,10));assert(cache.recordings.size()==1);
 // A successful reset of a dormant list needs no subsequent PSO/barrier call.
 assert(cache.confirm_reset(&list,11));assert(cache.recordings.empty());assert(cache.entries.at(key).awaiting_signal);
 assert(!cache.confirm_reset(&list,12));assert(cache.entries.at(key).recordings==0&&!cache.blocked);
 cache.retire_ready(200,30);assert(cache.ready_to_release.empty());
 cache.entries.at(key).awaiting_signal=false;cache.entries.at(key).retirement_fence=201;
 cache.retire_ready(200,30);assert(cache.ready_to_release.empty());cache.retire_ready(201,30);assert(cache.ready_to_release.size()==1);
 // Repeated pre-call notifications must not discard earlier successful proof.
 cache.entries.emplace(key,LeanBorrowEntry{});cache.record(&second,key,40);cache.note_reset(&second,true,20);cache.note_reset(&second,true,21);assert(cache.confirm_reset(&second,21));
 cache.record(&second,key,41);cache.note_reset(&second,false,0);assert(!cache.confirm_reset(&second,22));assert(cache.recordings.contains(&second));assert(cache.begin_recording(&second));assert(!cache.reset_epochs.contains(&second));

 // Two independent consumers can lease the same game resources/recording.
 // Retiring one consumer never invalidates the other's descriptor ownership.
 LeanBorrowCache native_guides,dlss;
 native_guides.entries.emplace(key,LeanBorrowEntry{});dlss.entries.emplace(key,LeanBorrowEntry{});
 native_guides.record(&list,key,100);dlss.record(&list,key,100);
 native_guides.note_reset(&list,true,50);assert(native_guides.confirm_reset(&list,51));
 native_guides.entries.at(key).awaiting_signal=false;native_guides.entries.at(key).retirement_fence=1;
 native_guides.retire_ready(1,104);assert(native_guides.entries.empty());
 assert(dlss.recordings.contains(&list)&&dlss.entries.at(key).recordings==1);
 dlss.note_reset(&list,true,50);assert(!dlss.confirm_reset(&list,50));
 dlss.retire_ready(100,105);assert(!dlss.entries.empty());
 assert(dlss.confirm_reset(&list,51));dlss.entries.at(key).awaiting_signal=false;dlss.entries.at(key).retirement_fence=101;
 dlss.retire_ready(100,105);assert(!dlss.entries.empty());dlss.retire_ready(101,105);assert(dlss.entries.empty());
}
