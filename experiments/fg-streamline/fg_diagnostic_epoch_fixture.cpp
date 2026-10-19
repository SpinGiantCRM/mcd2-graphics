
// Portable SDK-free doubles around extracted production decision bodies.
#include "runtime_diagnostics_projection.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
namespace d=mcd2::diagnostics;
struct Session {bool closing=false;d::FgAttribution diagnosticFrames;MCD2FgDiagnosticBindingV1 diagnosticBinding{};uint64_t ordinal=0;};
Session storage;Session*session=&storage;std::mutex guard;
// ACTUAL_BIND_EXPORT
int presentGate(void*queue,uint64_t id,uint32_t wanted){(void)queue;
// ACTUAL_PRESENT_PREFIX
return 0;}
MCD2DiagnosticIdentityV1 validatedIdentity{};
bool identityKnown=true;
mcd2::diagnostics::IdentityRead diagnostic_identity(MCD2DiagnosticIdentityV1){return {identityKnown,validatedIdentity};}
void bindFrame(uint64_t id,bool ready,bool wanted,MCD2DiagnosticIdentityV1 validated){(void)ready;validatedIdentity=validated;MCD2DiagnosticIdentityV1 previousIdentity{};auto bind=&mcd2_afg_diagnostic_bind_v1;
// ACTUAL_GUIDE_IDENTITY
// ACTUAL_BINDING
}
void finalAdmission(uint64_t id,int configured){auto&s=*session;
// ACTUAL_FINAL_ADMISSION
}
d::FgAttribution nvidiaDiagnosticFrames;
void nvidiaFrame(uint64_t id,unsigned mode,unsigned activeFGMode,MCD2DiagnosticIdentityV1 identity){d::IdentityRead identityRead{identityKnown,identity};
// ACTUAL_NVIDIA
}
void require(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
void amd(){MCD2DiagnosticIdentityV1 identity{1,2,3,4};auto&tracker=storage.diagnosticFrames;
 for(uint64_t n=1;n<=100;++n){bindFrame(n,true,true,identity);presentGate(reinterpret_cast<void*>(1),n,1);storage.ordinal=n;finalAdmission(n,0);tracker.outcome(n,true,1,n+100);}
 require(tracker.snapshot().generated==100,"initial outcomes missing");auto observed=tracker.snapshot().observedAtMs;
 identityKnown=false;bindFrame(101,true,true,{});storage.ordinal=101;finalAdmission(101,0);tracker.outcome(101,true,1,201);require(tracker.snapshot().generated==100,"Busy identity erased or admitted epoch");identityKnown=true;
 for(uint64_t n=101;n<105;++n){bindFrame(n,false,true,identity);presentGate(reinterpret_cast<void*>(1),n,0);require(tracker.snapshot().generated==100,"unready frame erased current epoch");require(tracker.snapshot().observedAtMs==observed,"unready frame refreshed timestamp");tracker.outcome(n,true,1,300);require(tracker.snapshot().generated==100,"unadmitted frame counted");}
 bindFrame(105,true,true,identity);presentGate(reinterpret_cast<void*>(1),106,1);storage.ordinal=105;finalAdmission(106,0);tracker.outcome(105,true,1,300);require(tracker.snapshot().generated==100,"mismatched binding erased/admitted epoch");
 storage.ordinal=106;finalAdmission(105,1);require(tracker.snapshot().generated==100&&tracker.snapshot().observedAtMs==observed,"failed configure erased/freshened epoch");
 bindFrame(107,true,true,identity);presentGate(reinterpret_cast<void*>(1),107,1);storage.ordinal=107;finalAdmission(107,0);tracker.outcome(107,true,1,301);require(tracker.snapshot().generated==101,"recovery must extend same epoch");
 auto fg=tracker.snapshot();fg.provider=2;fg.active=0;MCD2SrDiagnosticsV1 sr{sizeof(sr),0,0,0,0,0,0};auto p=d::project(identity,sr,fg,0,0,0,0,302);require(p.fg.generated==101&&!p.fg.active,"retained history invented active state");
 p=d::project(identity,sr,fg,0,0,0,0,3302);require(!p.fg.generated,"old outcome escaped expiry");
 uint64_t frame=200,lastSdk=107;
 for(unsigned field=0;field<4;++field){auto next=identity;if(field==0)++next.session;if(field==1)++next.revision;if(field==2)++next.checksum;if(field==3)++next.world;
  auto old=lastSdk;bindFrame(++frame,true,true,next);tracker.outcome(107,true,1,400);require(!tracker.snapshot().generated,"old epoch callback accepted");storage.ordinal=old;finalAdmission(frame,0);tracker.outcome(old,true,1,400);require(!tracker.snapshot().generated,"unadmitted reused/old ID accepted");storage.ordinal=frame;finalAdmission(frame,0);tracker.outcome(frame,true,1,401);require(tracker.snapshot().generated==1,"new epoch outcome missing");identity=next;lastSdk=frame;
 }
 bindFrame(++frame,false,false,identity);tracker.outcome(frame-1,true,1,402);require(!tracker.snapshot().session&&!tracker.snapshot().generated,"Off failed to clear epoch");
 bindFrame(++frame,true,true,identity);storage.ordinal=frame;finalAdmission(frame,0);tracker.outcome(frame,true,1,403);require(tracker.snapshot().generated==1,"Off recovery failed");presentGate(nullptr,UINT64_MAX,0);require(!tracker.snapshot().session,"explicit owner invalidation failed to clear");
}
void nvidia(){MCD2DiagnosticIdentityV1 identity{1,2,3,4};auto&t=nvidiaDiagnosticFrames;nvidiaFrame(10,1,1,identity);t.outcome(10,true,100,100);require(t.snapshot().generated==100,"initial NVIDIA outcome missing");identityKnown=false;nvidiaFrame(11,1,1,{});t.outcome(11,true,1,101);require(t.snapshot().generated==100,"NVIDIA Busy identity erased or admitted epoch");identityKnown=true;nvidiaFrame(11,0,0,identity);require(t.snapshot().generated==100&&t.snapshot().observedAtMs==100,"NVIDIA frame gap erased/freshened epoch");t.outcome(11,true,1,101);require(t.snapshot().generated==100,"NVIDIA unadmitted outcome counted");nvidiaFrame(12,1,1,identity);t.outcome(12,true,1,102);require(t.snapshot().generated==101,"NVIDIA recovery reset epoch");nvidiaFrame(13,0,0,{});t.outcome(12,true,1,103);require(!t.snapshot().session,"NVIDIA Off did not clear");nvidiaFrame(14,1,1,identity);t.admit(12,identity);t.outcome(12,true,1,104);require(!t.snapshot().generated,"NVIDIA reused ID accepted");}
int main(int argc,char**argv){try{if(argc!=2)throw std::runtime_error("case required");if(std::string(argv[1])=="amd")amd();else nvidia();std::cout<<"Actual FG producer epoch/admission decisions pass\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
