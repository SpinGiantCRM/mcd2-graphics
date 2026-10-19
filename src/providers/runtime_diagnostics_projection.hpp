#pragma once
#include "sr_runtime_protocol.hpp"
#include "fg_diagnostics_attribution.hpp"
namespace mcd2::diagnostics {
struct IdentityRead {bool known=true;MCD2DiagnosticIdentityV1 identity{};};
inline MCD2DiagnosticIdentityV1 contextIdentity(unsigned session,const providers::DecodedGraphicsRecord& authority,const providers::SrContext& context,std::uint64_t observed,std::uint64_t now){
 if(!session||context.session!=session||context.intent!=authority.bootstrap.stamp||!providers::validContext(context)||!context.ready||!observed||observed>now||now-observed>3000)return {};
 return {session,context.intent.revision,context.intent.checksum,context.world};
}
// Worker-owned activity sample. Metadata from the V1 getter never substitutes
// for an unavailable FSR runtime sample or extends its original observation.
struct SrWorkerSample {
 providers::SrProvider requested=providers::SrProvider::Native;
 MCD2DiagnosticIdentityV1 identity{};
 std::uint64_t observedAtMs=0;
 providers::SrRuntimeState state{};
};
inline bool currentFsrSample(MCD2DiagnosticIdentityV1 identity,const SrWorkerSample& sample,std::uint64_t now){
 const auto& state=sample.state;
 return valid(identity)&&sample.requested==providers::SrProvider::AmdFsr&&same(identity,sample.identity)&&
  sample.observedAtMs&&sample.observedAtMs<=now&&now-sample.observedAtMs<=1500&&
  providers::validRuntimeState(state)&&state.session==identity.session&&state.intent==providers::RecordStamp{identity.revision,identity.checksum}&&
  state.world==identity.world&&state.provider==providers::SrProvider::AmdFsr&&state.phase==providers::SrPhase::Active&&!state.error;
}
class SrWorkerCache {
 SrWorkerSample sample_{};
 void clear(){sample_.observedAtMs=0;sample_.state={};}
public:
 void observe(MCD2DiagnosticIdentityV1 identity,providers::SrProvider requested,int result,const providers::SrRuntimeState* state,std::uint64_t observed,std::uint64_t now){
  if(!same(identity,sample_.identity)||requested!=sample_.requested){sample_={};sample_.identity=identity;sample_.requested=requested;}
  if(!valid(identity)||requested!=providers::SrProvider::AmdFsr){clear();return;}
  if(result==0){
   SrWorkerSample next{requested,identity,observed,state?*state:providers::SrRuntimeState{}};
   if(currentFsrSample(identity,next,now))sample_=next;else clear();
  }else if(result!=-2)clear();
  if(!currentFsrSample(identity,sample_,now))clear();
 }
 SrWorkerSample snapshot()const{return sample_;}
};
// Worker-only reuse on a nonblocking getter's Busy result. Neither a read nor
// a Busy poll can invent a producer outcome or refresh its timestamp.
class FgWorkerCache {
 MCD2DiagnosticIdentityV1 identity_{};MCD2FgDiagnosticsV2 sample_{};std::uint64_t readAt_=0;bool reusable_=false;
 void clear(){sample_={};readAt_=0;reusable_=false;}
 bool current(std::uint64_t now)const{return valid(identity_)&&readAt_&&readAt_<=now&&now-readAt_<=1500&&
  sample_.size==sizeof(sample_)&&!sample_.reserved&&sample_.provider>0&&sample_.provider<=2&&sample_.active<=1&&
  same(identity_,{sample_.session,sample_.revision,sample_.checksum,sample_.world})&&sample_.observedAtMs&&
  sample_.observedAtMs<=now&&now-sample_.observedAtMs<=3000;}
public:
 void observe(MCD2DiagnosticIdentityV1 identity,int result,const MCD2FgDiagnosticsV2* sample,std::uint64_t observed,std::uint64_t now){
  if(!same(identity,identity_)){clear();identity_=identity;}
  if(result==0){if(sample){sample_=*sample;readAt_=observed;reusable_=sample_.active==1&&!sample_.fault;}else clear();}
  else if(result!=-2||!reusable_)clear();
  if(!current(now))clear();
 }
 MCD2FgDiagnosticsV2 snapshot()const{return sample_;}
};
class SrMetadataCache {
 MCD2DiagnosticIdentityV1 identity_{};unsigned implementation_=0,capabilities_=0;std::uint64_t observed_=0;
public:
 void observe(MCD2DiagnosticIdentityV1 identity,int result,MCD2SrDiagnosticsV1& sample,std::uint64_t now){
  if(!same(identity,identity_)){identity_=identity;implementation_=capabilities_=0;observed_=0;}
  if(result==0&&sample.size==sizeof(sample)){implementation_=sample.implementation;capabilities_=sample.capabilities;observed_=now;}
  else if(result!=-2){implementation_=capabilities_=0;observed_=0;}
  if(!valid(identity)||!observed_||observed_>now||now-observed_>1500){implementation_=capabilities_=0;observed_=0;}
  if(!valid(identity))sample.implementation=sample.capabilities=0;
  if(result==-2)sample={sizeof(sample),0,0,0,implementation_,capabilities_,0};
 }
};
struct Projection {unsigned world=0;MCD2SrDiagnosticsV1 sr{};MCD2FgDiagnosticsV2 fg{};unsigned amdMode=0;std::uint64_t amdFrames=0;};
inline Projection project(MCD2DiagnosticIdentityV1 identity,MCD2SrDiagnosticsV1 sr,MCD2FgDiagnosticsV2 fg,unsigned amdRevision,unsigned amdChecksum,unsigned amdMode,std::uint64_t amdFrames,std::uint64_t now,const SrWorkerSample* worker=nullptr){
 Projection out;out.sr=sr;out.fg=fg;
 if(worker&&(worker->requested==providers::SrProvider::Native||worker->requested==providers::SrProvider::AmdFsr)){
  out.sr={sizeof(out.sr),0,0,0,sr.implementation,sr.capabilities,0};
  if(currentFsrSample(identity,*worker,now))out.sr={sizeof(out.sr),2,worker->state.intent.revision,worker->state.world,sr.implementation,sr.capabilities,worker->state.evaluations};
 }
 const auto& activity=out.sr;
 if(!valid(identity)||activity.size!=sizeof(activity)||!activity.provider||activity.provider>2||activity.revision!=identity.revision||activity.world!=identity.world){out.sr.provider=0;out.sr.evaluations=0;}
 if(!valid(identity)||fg.size!=sizeof(fg)||fg.reserved||!fg.provider||fg.provider>2||fg.active>1||!same(identity,{fg.session,fg.revision,fg.checksum,fg.world})||!fg.observedAtMs||fg.observedAtMs>now||now-fg.observedAtMs>3000){out.fg.provider=out.fg.active=out.fg.fault=0;out.fg.rendered=out.fg.generated=0;}
 if(valid(identity)){out.world=identity.world;if(amdRevision==identity.revision&&amdChecksum==identity.checksum){out.amdMode=amdMode;out.amdFrames=amdFrames;}}
 return out;
}
}
