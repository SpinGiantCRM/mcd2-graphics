#include "../src/providers/native_fg_admission.hpp"
#include <cassert>
using namespace mcd2::providers;
int main(){
 GraphicsIntent i;i.fg=FgProvider::Amd;i.fgEnabled=true;
 GraphicsRecord bytes;DecodedGraphicsRecord r;
 assert(encodeGraphicsRecord(i,bytes)&&decodeGraphicsRecord(bytes,r));assert(nativeFgSelection(r));
 SrContext c{};c.session=321;c.intent=r.bootstrap.stamp;c.world=1;c.ready=true;c.worldToMetersMilli=100000;c.observedScaleMicro=100000000;
 assert(nativeFgContext(r,c,321,4000,1000));
 assert(!nativeFgContext(r,c,321,4001,1000));assert(!nativeFgContext(r,c,321,1000,1001));
 assert(!nativeFgContext(r,c,322,4000,1000));assert(!nativeFgContext(r,c,0,4000,1000));
 for(auto provider:{SrProvider::NvidiaDlss,SrProvider::AmdFsr,SrProvider::IntelXeSs}){
  i.sr=provider;assert(encodeGraphicsRecord(i,bytes)&&decodeGraphicsRecord(bytes,r));assert(!nativeFgSelection(r));
 }
 i.sr=SrProvider::Native;i.fg=FgProvider::Nvidia;assert(encodeGraphicsRecord(i,bytes)&&decodeGraphicsRecord(bytes,r));assert(!nativeFgSelection(r));
 i.fg=FgProvider::Amd;i.fgEnabled=false;assert(encodeGraphicsRecord(i,bytes)&&decodeGraphicsRecord(bytes,r));assert(!nativeFgSelection(r));
 i.fgEnabled=true;assert(encodeGraphicsRecord(i,bytes)&&decodeGraphicsRecord(bytes,r));c.intent=r.bootstrap.stamp;
 auto valid=c;c.ready=false;assert(!nativeFgContext(r,c,321,4000,1000));c=valid;
 c.world=0;assert(!nativeFgContext(r,c,321,4000,1000));c=valid;
 c.observedScaleMicro=99999999;assert(!nativeFgContext(r,c,321,4000,1000));c=valid;
 c.worldToMetersMilli=100001;assert(!nativeFgContext(r,c,321,4000,1000));c=valid;
 c.intent.checksum^=1;assert(!nativeFgContext(r,c,321,4000,1000));
 r.bootstrap.stamp.checksum^=1;assert(!nativeFgSelection(r));
}
