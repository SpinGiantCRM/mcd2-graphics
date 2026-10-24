#include "runtime_diagnostics_projection.hpp"
#include "sr_runtime_snapshot.h"
#include "fg_menu_projection.hpp"
#include <cstring>
#include <iostream>
#include <stdexcept>
namespace p=mcd2::providers;namespace d=mcd2::diagnostics;
int contextResult=0,menuResult=0;uint64_t tick=100;
MCD2SrContextSnapshotV1 packet;MCD2MenuSnapshotV1 menuPacket;
extern "C" int mcd2_sr_context_v1(MCD2SrContextSnapshotV1* p){if(contextResult)return contextResult;*p=packet;return 0;}
extern "C" int mcd2_menu_snapshot_v1(MCD2MenuSnapshotV1*p){if(menuResult)return menuResult;*p=menuPacket;return 0;}
void* GetModuleHandleW(const wchar_t*){return reinterpret_cast<void*>(1);}
void* GetProcAddress(void*,const char*name){return !strcmp(name,"mcd2_sr_context_v1")?reinterpret_cast<void*>(&mcd2_sr_context_v1):reinterpret_cast<void*>(&mcd2_menu_snapshot_v1);}
uint64_t GetTickCount64(){return tick;}
namespace fg_controls {bool enabled=true,wanted=true;struct{unsigned settingsRevision=7,provider=1,mode=1;}intent;}
// ACTUAL_IDENTITY_READER
void require(bool b,const char*reason){if(!b)throw std::runtime_error(reason);}
void setup(unsigned world=4,unsigned revision=7,bool ready=true,bool on=true){
 p::GraphicsIntent intent;intent.revision=revision;intent.sr=p::SrProvider::AmdFsr;intent.fgEnabled=on;intent.fg=p::FgProvider::Amd;p::GraphicsRecord bytes;p::DecodedGraphicsRecord record;
 require(p::encodeGraphicsRecord(intent,bytes)&&p::decodeGraphicsRecord(bytes,record),"fixture authority invalid");
 menuPacket={};menuPacket.size=sizeof(menuPacket);menuPacket.session=123;memcpy(menuPacket.record,bytes.data(),128);packet={};packet.size=sizeof(packet);packet.authority=menuPacket;packet.observedAtMs=tick;
 p::SrContext context{123,1,record.bootstrap.stamp,world,ready,100000,100000000,0,0,0};p::SrRuntimeBytes encoded;require(p::encodeSrContext(context,encoded),"fixture context invalid");memcpy(packet.context,encoded.data(),128);
 contextResult=menuResult=0;
}
int main(){try{
 setup();auto read=diagnostic_identity();require(read.known&&read.identity.world==4,"valid identity rejected");auto id=read.identity;d::FgAttribution tracker;tracker.select(id);tracker.admit(10,id);tracker.outcome(10,true,100,tick);
 for(int which:{0,1,2}){setup();contextResult=which!=1?-2:0;menuResult=which!=0?-2:0;read=diagnostic_identity(id);require(!read.known,"Busy misclassified as invalid");if(read.known)tracker.select(read.identity);require(tracker.snapshot().generated==100&&tracker.snapshot().observedAtMs==100,"Busy cleared/refreshed epoch");}
 setup();read=diagnostic_identity(id);tracker.select(read.identity);tracker.admit(11,id);tracker.outcome(11,true,1,101);require(tracker.snapshot().generated==101,"recovery did not extend epoch");
 setup(5);menuResult=-2;read=diagnostic_identity(id);require(read.known&&!d::valid(read.identity),"known changed world retained through other Busy");
 setup(4,8);contextResult=-2;read=diagnostic_identity(id);require(read.known&&!d::valid(read.identity),"known changed revision retained through other Busy");
 setup(4,7,false);menuResult=-2;read=diagnostic_identity(id);require(read.known&&!d::valid(read.identity),"known inactive context hidden by Busy");
 setup(4,7,true,false);contextResult=-2;read=diagnostic_identity(id);require(read.known&&!d::valid(read.identity),"known Off hidden by Busy");
 for(int result:{-1,-3}){setup();contextResult=result;menuResult=-2;read=diagnostic_identity(id);require(read.known&&!d::valid(read.identity),"hard getter error hidden by Busy");}
 setup();packet.observedAtMs=101;menuResult=-2;read=diagnostic_identity(id);require(read.known&&!d::valid(read.identity),"future observation retained");
 setup();tick=3101;menuResult=-2;read=diagnostic_identity(id);require(read.known&&!d::valid(read.identity),"expired observation retained");
 setup();fg_controls::wanted=false;contextResult=menuResult=-2;read=diagnostic_identity(id);require(read.known&&!d::valid(read.identity),"explicit Off treated as Busy");
 std::cout<<"Actual FG identity: Busy skips epoch/admission; successful Off/world/stamp/inactive/errors/expiry invalidate\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
