#include "../src/providers/sr_runtime_protocol.hpp"
#include "../src/providers/sr_runtime_save_transport.hpp"
#include "../src/providers/sr_runtime_snapshot.h"
#include "../src/providers/sr_viewport.hpp"
#include "../src/providers/nvidia_menu_projection.hpp"
#include <cassert>
#include <iostream>
using namespace mcd2::providers;
int main(){
 GraphicsIntent intent;intent.revision=9;intent.sr=SrProvider::NvidiaDlss;GraphicsRecord recordBytes;DecodedGraphicsRecord record;NvidiaMenuProjection projection;
 for(unsigned q=0;q<6;q++){
  intent.srPreferences[0].quality=Quality(q);intent.srPreferences[0].customScaleBasisPoints=7700;
  assert(encodeGraphicsRecord(intent,recordBytes)&&decodeGraphicsRecord(recordBytes,record)&&projectNvidiaMenu(record,projection));
  assert(projection.revision==9&&projection.mode==(q==0?1u:2u)&&projection.preset==(q==0?5u:q==5?4u:q-1));
  assert(projection.scale==(q==0?10000u:q==1?6667u:q==2?5800u:q==3?5000u:q==4?3333u:7700u));
 }
 intent.sr=SrProvider::AmdFsr;assert(encodeGraphicsRecord(intent,recordBytes)&&decodeGraphicsRecord(recordBytes,record)&&projectNvidiaMenu(record,projection)&&projection.mode==0&&projection.scale==10000);
 record.intent.revision++;assert(!projectNvidiaMenu(record,projection)); // stale checksum/bootstrap pair
 intent.srPreferences[0].customScaleBasisPoints=6700;
 assert(encodeGraphicsRecord(intent,recordBytes)&&decodeGraphicsRecord(recordBytes,record)&&!projectNvidiaMenu(record,projection));
 SrViewport viewport;
 assert(srViewport(2260,1272,{0,0,2258,1270},viewport));
 assert(viewport.width==2259&&viewport.height==1271);
 assert(srViewportMatchesPlan(viewport,2258,1270));
 assert(!srViewportMatchesPlan(viewport,2257,1270));
 assert(!srViewportMatchesPlan(viewport,2260,1270));
 assert(srViewport(2560,1440,{0,0,2559,1439},viewport)&&srViewportMatchesPlan(viewport,2560,1440));
 const auto original=viewport;
 for(auto rect:{std::array<uint32_t,4>{1,0,2559,1439},{0,1,2559,1439},{0,0,2560,1439},
                {0,0,2559,1440},{0,0,UINT32_MAX,1439},{0,0,638,1439},{0,0,2559,358}}){
  assert(!srViewport(2560,1440,rect,viewport));assert(viewport.width==original.width&&viewport.height==original.height);
 }
 assert(!srViewport(2268,1280,{0,0,2258,1270},viewport)); // unqualified padding
 assert(!srViewport(0,0,{0,0,0,0},viewport));
 SrContext c{123,10,{7,0xdeadbeef},2,true,100000,66666668,5,66666667,0};
 SrRuntimeState s{123,5,{7,0xdeadbeef},2,SrPhase::ApplySource,0,2560,1440,3840,2160,66666667,0,SrProvider::AmdFsr,Quality::Quality,1};
 SrRuntimeBytes bytes;SrContext decoded;SrRuntimeState state;
 assert(encodeSrContext(c,bytes)&&decodeSrContext(bytes,decoded)&&decoded==c);
 for(unsigned n=0;n<128;n++){auto bad=bytes;bad[n]^=1;assert(!decodeSrContext(bad,decoded));}
 assert(encodeSrRuntimeState(s,bytes)&&decodeSrRuntimeState(bytes,state)&&state==s);
 for(unsigned n=0;n<128;n++){auto bad=bytes;bad[n]^=1;assert(!decodeSrRuntimeState(bad,state));}
 assert(sourceAcknowledged(s,c));
 auto old=c;old.session++;assert(!sourceAcknowledged(s,old));old=c;old.intent.checksum++;assert(!sourceAcknowledged(s,old));
 old=c;old.world++;assert(!sourceAcknowledged(s,old));old=c;old.sourceAck--;assert(!sourceAcknowledged(s,old));
 old=c;old.sourceError=1;assert(!sourceAcknowledged(s,old));old=c;old.observedScaleMicro+=101;assert(!sourceAcknowledged(s,old));
 old=c;old.ready=false;assert(!sourceAcknowledged(s,old));s.phase=SrPhase::RestoreSource;
 s.sourceScaleMicro=100000000;old.observedScaleMicro=old.sourceScaleMicro=100000000;assert(sourceAcknowledged(s,old));
 s.phase=SrPhase::Active;s.renderWidth=10;assert(!encodeSrRuntimeState(s,bytes));s.renderWidth=2560;
 assert(encodeSrRuntimeState(s,bytes));setRecordWord(bytes,84,1);setRecordWord(bytes,16,recordChecksum(bytes));assert(!decodeSrRuntimeState(bytes,state));
 c.worldToMetersMilli=0;assert(!encodeSrContext(c,bytes));c.ready=false;assert(encodeSrContext(c,bytes));
 // Runtime response framing stays distinct from settings and commit receipts.
 MenuAuthority authority;assert(!decodeMenuAuthority(bytes,authority)); // No codec accepts the other channel.
 assert(sizeof(MCD2SrContextSnapshotV1)==288&&sizeof(MCD2SrRuntimeSnapshotV1)==136);
 std::cout<<"SR runtime: exact source ACK, process/world scope, rollback, corruption and ABI checks pass\n";
}
