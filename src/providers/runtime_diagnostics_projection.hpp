#pragma once
#include "sr_runtime_protocol.hpp"
#include "fg_diagnostics_attribution.hpp"
namespace mcd2::diagnostics {
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
