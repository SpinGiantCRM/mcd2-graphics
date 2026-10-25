#include "../experiments/fg-streamline/fg_configuration.hpp"
#include "../src/providers/fg_menu_projection.hpp"
#include <cassert>
int main(){
 using namespace mcd2;
 static_assert(sizeof(MCD2FGConfig)==48);
 for(unsigned maximum:{0u,1u,2u,5u}) {
  for(unsigned mode=0;mode<5;++mode)assert(fg::supportsFrameMode(mode,maximum,false)==(mode+1<=maximum));
  assert(!fg::supportsFrameMode(5,maximum,true)); // Dynamic has no schema-5 rollback contract.
  assert(!fg::supportsGeneratedFrames(0,maximum,true));
 }
 providers::GraphicsIntent intent;intent.revision=1;intent.sr=providers::SrProvider::AmdFsr;
 intent.fg=providers::FgProvider::Nvidia;intent.fgStrategy=providers::FgStrategy::NativeMfg;intent.requestedMultiplier=6;
 providers::GraphicsRecord bytes;providers::DecodedGraphicsRecord record;providers::FgMenuProjection projection;
 assert(providers::encodeGraphicsRecord(intent,bytes)&&providers::decodeGraphicsRecord(bytes,record));
 assert(providers::projectFgMenu(record,projection)&&projection.pairEligible&&projection.frameMode==4);
 assert(providers::fgStartupOwner(projection)==0); // Fresh Off stays native.
 intent.fgEnabled=true;assert(providers::encodeGraphicsRecord(intent,bytes)&&providers::decodeGraphicsRecord(bytes,record));
 assert(providers::projectFgMenu(record,projection)&&providers::fgStartupOwner(projection)==1);
 intent.fg=providers::FgProvider::Amd;assert(providers::encodeGraphicsRecord(intent,bytes)&&providers::decodeGraphicsRecord(bytes,record));
 assert(providers::projectFgMenu(record,projection)&&!projection.pairEligible);
}
