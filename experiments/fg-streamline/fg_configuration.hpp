#pragma once
#include "fg_bridge_contract.h"
namespace mcd2::fg {
inline bool restartRequired(bool available,unsigned requested,unsigned startup,bool retired) {
 return available && (retired || requested!=startup);
}
// Unbound/stale startup intent must not permanently retire an On swapchain.
inline bool permanentOff(unsigned requested,bool currentSession,unsigned fault) {
 return fault!=0 || (currentSession && requested==0);
}
// Menu frames do not carry world guides. Suspending must not advertise their
// absent guide dimensions as a new output-resolution allocation.
struct Configuration {
 MCD2FGConfig last{};
 MCD2FGConfig prepare(const MCD2FGConfig& requested) const {
  if (!requested.mode && last.size) { auto suspended=last; suspended.mode=0; return suspended; }
  return requested;
 }
 void accepted(const MCD2FGConfig& value) { last=value; }
 void clear() { last={}; }
};
}
