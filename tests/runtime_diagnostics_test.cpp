#include "../src/providers/runtime_diagnostics_projection.hpp"
#include "../src/providers/fg_diagnostics_attribution.hpp"
#include <cassert>
#include <thread>
#include <iostream>
int main(){
 namespace d=mcd2::diagnostics;namespace p=mcd2::providers;
 p::GraphicsIntent intent;intent.revision=7;p::GraphicsRecord bytes;p::DecodedGraphicsRecord record;
 assert(p::encodeGraphicsRecord(intent,bytes)&&p::decodeGraphicsRecord(bytes,record));
 p::SrContext context{123,1,record.bootstrap.stamp,4,true,100000,100000000,0,0,0};
 auto identity=d::contextIdentity(123,record,context,100,200);assert(identity.session==123&&identity.world==4);
 assert(!d::contextIdentity(123,record,context,100,3101).session);
 assert(!d::contextIdentity(123,record,context,201,200).session);
 auto wrong=context;wrong.intent.checksum^=1;assert(!d::contextIdentity(123,record,wrong,100,200).session);
 wrong=context;wrong.ready=false;assert(!d::contextIdentity(123,record,wrong,100,200).session);
 MCD2SrDiagnosticsV1 sr{sizeof(sr),0,0,0,30105,0,0};MCD2FgDiagnosticsV2 fg{};fg.size=sizeof(fg);
 auto projected=d::project(identity,sr,fg,7,record.bootstrap.stamp.checksum,1,100,200);
 assert(projected.world==4&&projected.sr.provider==0&&projected.sr.evaluations==0&&projected.amdMode==1);
 sr.provider=2;sr.revision=7;sr.world=3;sr.evaluations=99;
 projected=d::project(identity,sr,fg,6,record.bootstrap.stamp.checksum,1,100,200);
 assert(projected.world==4&&!projected.sr.provider&&!projected.sr.evaluations&&!projected.amdMode&&!projected.amdFrames);
 projected=d::project(identity,sr,fg,7,record.bootstrap.stamp.checksum^1u,1,100,200);assert(!projected.amdMode);
 d::FgAttribution tracker;tracker.select(identity);tracker.admit(10,identity);tracker.outcome(10,false,1,201);tracker.outcome(10,true,1,202);
 auto current=tracker.snapshot();assert(current.rendered==1&&current.generated==1&&current.world==4&&current.observedAtMs==202);
 tracker.outcome(10,true,1,203);assert(tracker.snapshot().generated==1); // duplicate callback
 auto next=identity;next.world=5;tracker.select(next);tracker.admit(11,next);
 tracker.outcome(10,true,1,204);assert(tracker.snapshot().generated==0); // queued old-world callback
 tracker.outcome(11,true,1,205);assert(tracker.snapshot().generated==1&&tracker.snapshot().world==5);
 tracker.admit(10,next);tracker.outcome(10,true,1,206);assert(tracker.snapshot().generated==1); // reused frame ID is refused
 for(unsigned n=12;n<90;++n)tracker.admit(n,next);
 tracker.outcome(11,false,1,207);assert(tracker.snapshot().rendered==0); // bounded retention
 tracker.select({});tracker.outcome(89,true,1,208);assert(!tracker.snapshot().session);
 tracker.select(next);tracker.outcome(999,true,1,209);assert(tracker.snapshot().generated==0); // unknown SDK frame
 d::FgAttribution sparse;sparse.select(identity);sparse.admit(1,identity);
 for(unsigned n=1;n<=64;++n)sparse.admit(n*64,identity);
 sparse.outcome(1,true,1,209);assert(sparse.snapshot().generated==0); // sparse IDs still evict after 64 admissions
 tracker.select(identity);tracker.admit(100,identity);tracker.outcome(100,true,1,209);fg=tracker.snapshot();fg.provider=2;fg.active=1;
 projected=d::project(identity,sr,fg,7,record.bootstrap.stamp.checksum,0,0,210);assert(projected.fg.generated==1);
 projected=d::project(identity,sr,fg,7,record.bootstrap.stamp.checksum,0,0,3210);assert(!projected.fg.generated);
 projected=d::project(next,sr,fg,7,record.bootstrap.stamp.checksum,0,0,210);assert(!projected.fg.generated);
 auto wrongFg=fg;wrongFg.checksum^=1;projected=d::project(identity,sr,wrongFg,7,record.bootstrap.stamp.checksum,0,0,210);assert(!projected.fg.generated);
 wrongFg=fg;wrongFg.session++;projected=d::project(identity,sr,wrongFg,7,record.bootstrap.stamp.checksum,0,0,210);assert(!projected.fg.generated);
 wrongFg=fg;wrongFg.reserved=1;projected=d::project(identity,sr,wrongFg,7,record.bootstrap.stamp.checksum,0,0,210);assert(!projected.fg.generated);
 // FSR activity reuses this worker poll's decoded native runtime, independently of metadata.
 p::SrRuntimeState runtime;runtime.session=123;runtime.sequence=5;runtime.intent=record.bootstrap.stamp;runtime.world=4;
 runtime.provider=p::SrProvider::AmdFsr;runtime.phase=p::SrPhase::Active;runtime.evaluations=100;
 runtime.renderWidth=1280;runtime.renderHeight=800;runtime.outputWidth=1920;runtime.outputHeight=1200;
 d::SrWorkerCache samples;samples.observe(identity,p::SrProvider::AmdFsr,0,&runtime,100,100);
 auto sampled=samples.snapshot();MCD2SrDiagnosticsV1 unavailable{sizeof(unavailable),0,0,0,0,0,0};
 projected=d::project(identity,unavailable,fg,7,record.bootstrap.stamp.checksum,1,100,100,&sampled);
 assert(projected.sr.provider==2&&projected.sr.evaluations==100&&projected.world==4); // Metadata getter may be busy.
 auto metadata=unavailable;metadata.provider=2;metadata.revision=7;metadata.world=4;metadata.implementation=30105;metadata.capabilities=1;metadata.evaluations=999;
 projected=d::project(identity,metadata,fg,7,record.bootstrap.stamp.checksum,1,100,100,&sampled);
 assert(projected.sr.evaluations==100&&projected.sr.implementation==30105); // Metadata cannot replace activity.
 samples.observe(identity,p::SrProvider::AmdFsr,-2,nullptr,400,400);sampled=samples.snapshot();
 assert(sampled.observedAtMs==100); // Busy is unknown, not a fresh zero/reset or a refreshed timestamp.
 projected=d::project(identity,unavailable,fg,7,record.bootstrap.stamp.checksum,1,100,1600,&sampled);
 assert(projected.sr.evaluations==100); // Inclusive 1500 ms bound.
 projected=d::project(identity,unavailable,fg,7,record.bootstrap.stamp.checksum,1,100,1601,&sampled);
 assert(!projected.sr.provider&&!projected.sr.evaluations&&projected.amdMode==1);
 auto independentFg=fg;independentFg.observedAtMs=100;
 projected=d::project(identity,metadata,independentFg,7,record.bootstrap.stamp.checksum,1,100,1601,&sampled);
 assert(!projected.sr.provider&&projected.fg.generated==1&&projected.amdMode==1); // SR expiry cannot erase FG/AL.
 samples.observe(identity,p::SrProvider::AmdFsr,-2,nullptr,1601,1601);assert(!samples.snapshot().observedAtMs);
 for(auto phase:{p::SrPhase::Native,p::SrPhase::Evaluate,p::SrPhase::Retire,p::SrPhase::Failed}){
  samples.observe(identity,p::SrProvider::AmdFsr,0,&runtime,1700,1700);auto inactive=runtime;inactive.phase=phase;
  samples.observe(identity,p::SrProvider::AmdFsr,0,&inactive,1701,1701);assert(!samples.snapshot().observedAtMs);
  samples.observe(identity,p::SrProvider::AmdFsr,-2,nullptr,1702,1702);assert(!samples.snapshot().observedAtMs);
 }
 auto failed=runtime;failed.error=1;samples.observe(identity,p::SrProvider::AmdFsr,0,&failed,1800,1800);assert(!samples.snapshot().observedAtMs);
 for(unsigned field=0;field<4;++field){auto changed=identity;if(field==0)changed.session++;if(field==1)changed.revision++;if(field==2)changed.checksum^=1;if(field==3)changed.world++;
  samples.observe(identity,p::SrProvider::AmdFsr,0,&runtime,1900,1900);samples.observe(changed,p::SrProvider::AmdFsr,-2,nullptr,1901,1901);
  assert(!samples.snapshot().observedAtMs);samples.observe(changed,p::SrProvider::AmdFsr,0,&runtime,1902,1902);assert(!samples.snapshot().observedAtMs);
 }
 samples.observe(identity,p::SrProvider::AmdFsr,0,&runtime,2000,2000);samples.observe({},p::SrProvider::AmdFsr,-2,nullptr,2001,2001);assert(!samples.snapshot().observedAtMs); // Not-ready/stale context.
 samples.observe(identity,p::SrProvider::AmdFsr,0,&runtime,2100,2100);samples.observe(identity,p::SrProvider::AmdFsr,-1,nullptr,2101,2101);assert(!samples.snapshot().observedAtMs); // Hard/invalid packet failure.
 samples.observe(identity,p::SrProvider::Native,0,&runtime,2200,2200);sampled=samples.snapshot();
 projected=d::project(identity,sr,fg,7,record.bootstrap.stamp.checksum,1,100,2200,&sampled);assert(!projected.sr.provider&&!projected.sr.evaluations&&projected.world==4&&projected.amdMode==1);
 samples.observe(identity,p::SrProvider::AmdFsr,0,&runtime,100,1601);assert(!samples.snapshot().observedAtMs); // Delayed worker work cannot freshen an old read.
 samples.observe(identity,p::SrProvider::AmdFsr,0,&runtime,2300,2300);runtime.evaluations=1;samples.observe(identity,p::SrProvider::AmdFsr,0,&runtime,2301,2301);assert(samples.snapshot().state.evaluations==1); // Preserve an actual counter reset.
 sampled=samples.snapshot();projected=d::project(identity,unavailable,fg,7,record.bootstrap.stamp.checksum,1,100,2300,&sampled);assert(!projected.sr.provider); // Future observation rejected.
 next.revision=8;next.checksum^=1;
 // Callback and epoch publication use the production tracker's synchronization.
 std::thread producer([&]{for(unsigned n=101;n<10000;++n){tracker.select(n%2?identity:next);tracker.admit(n,n%2?identity:next);tracker.outcome(n,true,1,n);}});
 for(unsigned n=0;n<10000;++n){auto snapshot=tracker.snapshot();assert(!snapshot.session||(snapshot.world==4&&snapshot.revision==7&&snapshot.checksum==identity.checksum)||(snapshot.world==5&&snapshot.revision==8&&snapshot.checksum==next.checksum));}
 producer.join();std::cout<<"Diagnostics: independent Native context, exact stamps, callback epochs, bounded/reused IDs, worker Busy/freshness/identity sampling and concurrency pass\n";
}
