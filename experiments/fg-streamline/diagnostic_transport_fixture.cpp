#include "runtime_diagnostics_projection.hpp"
#include <cstring>
#include <iostream>
#include <stdexcept>
using namespace mcd2::diagnostics;
std::uint64_t tick=100;int srResult=0,fgResult=0;bool missing=false;
MCD2SrDiagnosticsV1 srValue{sizeof(srValue),2,7,4,30105,1,999};MCD2FgDiagnosticsV2 fgValue{};
int readSr(MCD2SrDiagnosticsV1*out){if(srResult)return srResult;*out=srValue;return 0;}
int readFg(MCD2FgDiagnosticsV1*out){*out={sizeof(*out),2,1,0,30106,0,500,500};return 0;}
int readAttributed(MCD2FgDiagnosticsV2*out){if(fgResult)return fgResult;*out=fgValue;return 0;}
void* GetModuleHandleW(const wchar_t*){return reinterpret_cast<void*>(1);}
void* GetProcAddress(void*,const char*name){if(missing)return nullptr;if(!strcmp(name,"mcd2_sr_diagnostics_v1"))return reinterpret_cast<void*>(&readSr);if(!strcmp(name,"mcd2_fg_diagnostics_v1"))return reinterpret_cast<void*>(&readFg);return reinterpret_cast<void*>(&readAttributed);}
std::uint64_t GetTickCount64(){return tick;}
struct Result{MCD2SrDiagnosticsV1 sr;MCD2FgDiagnosticsV2 fg;};
Result poll(MCD2DiagnosticIdentityV1 identity){
// ACTUAL_GETTERS
 return {sr,attributed};
}
void require(bool b,const char*label){if(!b)throw std::runtime_error(label);}
void sample(MCD2DiagnosticIdentityV1 id,std::uint64_t observed=0){fgResult=srResult=0;missing=false;fgValue={};fgValue.size=sizeof(fgValue);fgValue.provider=2;fgValue.active=1;fgValue.session=id.session;fgValue.revision=id.revision;fgValue.checksum=id.checksum;fgValue.world=id.world;fgValue.observedAtMs=observed?observed:tick;fgValue.rendered=fgValue.generated=100;}
int main(){try{
 MCD2DiagnosticIdentityV1 id{123,7,9,4};sample(id);auto r=poll(id);require(r.fg.generated==100&&r.sr.implementation==30105,"initial valid sample missing");
 tick=500;srResult=fgResult=-2;r=poll(id);require(r.fg.generated==100&&r.fg.observedAtMs==100,"Busy lost/refreshed FG outcome");require(r.sr.implementation==30105&&r.sr.capabilities==1&&!r.sr.provider&&!r.sr.evaluations,"metadata Busy lost version or borrowed activity");
 tick=1600;r=poll(id);require(r.fg.generated==100&&r.sr.implementation==30105,"inclusive bound rejected");tick=1601;r=poll(id);require(!r.fg.provider&&!r.fg.generated&&!r.sr.implementation,"expired Busy retained sample");
 tick=2000;sample(id);fgValue.active=0;r=poll(id);require(!r.fg.active&&r.fg.provider==2,"explicit inactive hidden by active cache");tick=2001;fgResult=-2;r=poll(id);require(!r.fg.provider&&!r.fg.generated,"inactive observation reused as active history");
 tick=2100;sample(id);fgValue.fault=7;r=poll(id);require(r.fg.fault==7,"explicit fault hidden");tick=2101;fgResult=-2;r=poll(id);require(!r.fg.provider,"faulted active history reused");
 tick=2200;sample(id);poll(id);auto changed=id;changed.world++;srResult=fgResult=-2;r=poll(changed);require(!r.fg.provider&&!r.sr.implementation,"wrong-world Busy borrowed previous snapshot");
 for(unsigned field=0;field<4;++field){tick+=10;sample(id);poll(id);changed=id;if(field==0)changed.session++;if(field==1)changed.revision++;if(field==2)changed.checksum++;if(field==3)changed.world++;srResult=fgResult=-2;r=poll(changed);require(!r.fg.provider&&!r.sr.implementation,"full identity mismatch escaped");}
 tick=3000;sample(id);poll(id);fgValue.world++;r=poll(id);require(!r.fg.provider,"known mismatched sample reused previous activity");
 tick=3100;sample(id);poll(id);srResult=fgResult=-3;r=poll(id);require(!r.fg.provider&&!r.sr.implementation,"hard getter error reused history");
 tick=3200;sample(id);poll(id);missing=true;r=poll(id);require(!r.fg.provider&&!r.sr.implementation,"missing export reused history");
 tick=3300;sample(id);srValue.implementation=0;r=poll(id);require(!r.sr.implementation,"known unused metadata hidden by cache");srResult=-2;tick=3301;r=poll(id);require(!r.sr.implementation,"unused metadata revived on Busy");srValue.implementation=30105;
 for(unsigned malformed=0;malformed<3;++malformed){tick=3400+malformed;sample(id);if(malformed==0)fgValue.reserved=1;if(malformed==1)fgValue.provider=3;if(malformed==2)fgValue.active=2;r=poll(id);require(!r.fg.provider,"malformed attributed snapshot accepted");}
 tick=4001;sample(id,1000);r=poll(id);require(!r.fg.provider,"producer outcome expiry hidden by fresh read");
 sample(id,tick+1);r=poll(id);require(!r.fg.provider,"future outcome accepted");
 sample(id);r=poll({});require(!r.fg.provider&&!r.sr.implementation,"invalid current identity borrowed metadata/activity");
 std::cout<<"Actual diagnostics transport: Busy preserves original observations within1500ms; full identity/expiry/inactive/fault/error invalidate; metadata never borrows activity\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
