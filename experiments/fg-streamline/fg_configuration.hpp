#pragma once
#include "fg_bridge_contract.h"
namespace mcd2::fg {
inline bool sameConfiguration(const MCD2FGConfig& a,const MCD2FGConfig& b) {
 return a.size==b.size && a.mode==b.mode && a.width==b.width && a.height==b.height &&
  a.motionWidth==b.motionWidth && a.motionHeight==b.motionHeight &&
  a.colorFormat==b.colorFormat && a.uiFormat==b.uiFormat &&
  a.depthFormat==b.depthFormat && a.motionFormat==b.motionFormat &&
  a.backBuffers==b.backBuffers && a.generatedFrames==b.generatedFrames;
}
// Fixed-ratio render-scale changes require suspension before accepting new
// guide dimensions. Resume on the following frame with reset input history.
inline MCD2FGConfig configurationForPresent(const MCD2FGConfig& requested,const MCD2FGConfig& accepted) {
 if(accepted.size && (!requested.mode || accepted.mode)) {
  auto comparable=accepted;comparable.mode=requested.mode;
  if(!requested.mode || !sameConfiguration(requested,comparable)) {
   auto suspended=accepted;suspended.mode=0;return suspended;
  }
 }
 return requested;
}
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
