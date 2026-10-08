#include "../src/providers/sr_runtime_protocol.hpp"
#include "../src/providers/sr_runtime_save_transport.hpp"
#include "../src/providers/sr_runtime_snapshot.h"
#include <cassert>
#include <iostream>
using namespace mcd2::providers;
int main(){
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
