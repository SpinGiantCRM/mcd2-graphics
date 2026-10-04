// Private single-integration-queue gate. A recorded command list remains an
// owner even after submission: it can legally be submitted again until reset.
// ReShade's execute event precedes ExecuteCommandLists, so never signal there.
using LeanBorrowKey = std::tuple<uint64_t,uint64_t,unsigned,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t>;
struct LeanBorrowEntry {
 ID3D12DescriptorHeap *heap=nullptr;
 std::array<ID3D12Resource*,7> resources{};
 size_t recordings=0;
 uint64_t last_use_frame=0,retirement_fence=0;
 bool awaiting_signal=false;
};
struct LeanBorrowCache {
 std::map<LeanBorrowKey,LeanBorrowEntry> entries;
 std::map<a::command_list*,std::set<LeanBorrowKey>> recordings;
 std::set<a::command_list*> reset_pending;
 std::vector<LeanBorrowEntry> ready_to_release;
 ID3D12Fence *fence=nullptr;
 uint64_t next_fence=0,completed_fence=0,retired=0,peak_entries=0,signals=0;
 bool blocked=false;

 void forget(a::command_list *cmd) {
  auto it=recordings.find(cmd);if(it==recordings.end())return;
  for(const auto &key:it->second){auto entry=entries.find(key);
   if(entry==entries.end() || !entry->second.recordings){blocked=true;continue;}
   --entry->second.recordings;
   // Fence is inserted at a subsequent finish_present, after reset/destroy
   // invalidates this recording. Keep references until that fence completes.
   entry->second.awaiting_signal=true;
  }
  recordings.erase(it);
 }
 // Reset is a pre-call event and can fail. Keep the old recording until a
 // subsequent pipeline bind demonstrates legal recording of its replacement.
 void begin_recording(a::command_list *cmd){if(reset_pending.erase(cmd))forget(cmd);}
 void record(a::command_list *cmd,const LeanBorrowKey &key,uint64_t frame_number) {
  auto &entry=entries.at(key);
  if(recordings[cmd].insert(key).second)++entry.recordings;
  entry.last_use_frame=frame_number;
 }
 static void release(LeanBorrowEntry &entry) {
  if(entry.heap)entry.heap->Release();entry.heap=nullptr;
  for(auto *r:entry.resources)if(r)r->Release();entry.resources.fill(nullptr);
 }
 void retire_ready(uint64_t completed,uint64_t current_frame) {
  if(blocked || completed==UINT64_MAX)return;
  for(auto it=entries.begin();it!=entries.end();){auto &entry=it->second;
   if(!entry.recordings && !entry.awaiting_signal && entry.retirement_fence && entry.retirement_fence<=completed && current_frame>entry.last_use_frame+2){
    // Removal is protected by the observer lock; COM destruction is not.
    // Destruction can call other addons while another thread holds their locks.
    ready_to_release.push_back(entry);it=entries.erase(it);++retired;
   }else ++it;
  }
 }
 void clear_after_idle() {
  for(auto &[key,entry]:entries)ready_to_release.push_back(entry);
  entries.clear();recordings.clear();reset_pending.clear();
  if(fence)fence->Release();fence=nullptr;
  next_fence=completed_fence=0;blocked=false;
 }
};
