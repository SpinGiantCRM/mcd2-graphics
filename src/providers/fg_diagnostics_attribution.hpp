#pragma once
#include "runtime_diagnostics.h"
#include <array>
#include <mutex>
namespace mcd2::diagnostics {
inline bool valid(MCD2DiagnosticIdentityV1 id){return id.session&&id.revision&&id.world&&id.session<=0x7fffffffu&&id.revision<=0x7fffffffu&&id.world<=0x7fffffffu;}
inline bool same(MCD2DiagnosticIdentityV1 a,MCD2DiagnosticIdentityV1 b){return a.session==b.session&&a.revision==b.revision&&a.checksum==b.checksum&&a.world==b.world;}
// Its lock is independent of the SDK/presenter lock: asynchronous SDK callbacks
// never acquire the presenter gate. Storage and callback work are bounded.
class FgAttribution {
 struct Entry {std::uint64_t frame=0;unsigned seen=0;bool valid=false;};
 mutable std::mutex mutex_;std::array<Entry,64> entries_{};
 MCD2DiagnosticIdentityV1 identity_{};MCD2FgDiagnosticsV2 state_{};
 std::uint64_t lastFrame_=0;bool admitted_=false;unsigned nextEntry_=0;
public:
 void select(MCD2DiagnosticIdentityV1 id){std::lock_guard lock(mutex_);if(!valid(id))id={};if(same(id,identity_))return;
  identity_=id;entries_={};nextEntry_=0;state_={};state_.size=sizeof(state_);state_.session=id.session;state_.revision=id.revision;state_.checksum=id.checksum;state_.world=id.world;
 }
 void admit(std::uint64_t frame,MCD2DiagnosticIdentityV1 id){std::lock_guard lock(mutex_);
  if(!valid(id)||!same(id,identity_)||(admitted_&&frame<=lastFrame_))return;
  admitted_=true;lastFrame_=frame;entries_[nextEntry_++%entries_.size()]={frame,0,true};
 }
 void outcome(std::uint64_t frame,bool generated,unsigned count,std::uint64_t now){std::lock_guard lock(mutex_);
  if(!valid(identity_)||!count)return;
  const unsigned bit=generated?2:1;
  for(auto& entry:entries_)if(entry.valid&&entry.frame==frame){if(entry.seen&bit)return;
   entry.seen|=bit;if(generated)state_.generated+=count;else state_.rendered+=count;state_.observedAtMs=now;return;}
 }
 MCD2FgDiagnosticsV2 snapshot()const{std::lock_guard lock(mutex_);return state_;}
};
}
