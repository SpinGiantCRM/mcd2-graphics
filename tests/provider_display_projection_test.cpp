#include "../src/providers/display_projection.hpp"
#include "../src/providers/fg_menu_projection.hpp"
#include "../src/providers/menu_snapshot.h"
#include "../src/providers/menu_requests.hpp"
#include "../src/latency/amd_latency.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
using namespace mcd2::providers;
int main(int argc,char** argv){
 GraphicsIntent intent;intent.revision=42;intent.sr=SrProvider::AmdFsr;
 intent.srPreferences[1]={Quality::Custom,7100,7100};
 intent.hdr={true,420,203,167};
 for(unsigned latency=0;latency<4;++latency)for(unsigned mode=0;mode<3;++mode){
  intent.latency=LatencyProvider(latency);intent.latencyMode=LatencyMode(mode);
  GraphicsRecord bytes;DecodedGraphicsRecord record;assert(encodeGraphicsRecord(intent,bytes)&&decodeGraphicsRecord(bytes,record));
  auto p=projectDisplay(record);assert(p.valid&&p.revision==42&&p.hdr==1&&p.peak==420&&p.paper==203&&p.ui==167);
  assert(p.reflex==(latency<2?mode:0));
  auto amd=mcd2::amd_latency::resolve(record);assert(amd.valid&&amd.enabled==(mode==1&&(latency==0||latency==2)));
  record.bootstrap.stamp.checksum^=1;assert(!projectDisplay(record).valid);
 }
 GraphicsRecord bytes;DecodedGraphicsRecord record;
 for(unsigned sr=0;sr<4;++sr)for(unsigned fg=0;fg<3;++fg){
  intent.sr=SrProvider(sr);intent.fg=FgProvider(fg);intent.fgEnabled=true;
  assert(encodeGraphicsRecord(intent,bytes)&&decodeGraphicsRecord(bytes,record));
  FgMenuProjection projection;assert(projectFgMenu(record,projection));
  assert(projection.pairEligible==((fg==0&&sr==1)||(fg==1&&(sr==0||sr==1||sr==2))));
  assert(fgMenuMatches(projection,42,fg,1));
  assert(!fgMenuMatches(projection,41,fg,1)&&!fgMenuMatches(projection,42,fg,0));
  assert(fgOwnerMatches(fg+1,fg)==(fg<2));assert(!fgOwnerMatches(0,fg));
  record.bootstrap.stamp.checksum^=1;assert(!projectFgMenu(record,projection));
 }
 intent.sr=SrProvider::AmdFsr;intent.fgStrategy=FgStrategy::NativeMfg;intent.requestedMultiplier=3;
 assert(encodeGraphicsRecord(intent,bytes)&&decodeGraphicsRecord(bytes,record));
 FgMenuProjection unqualified;assert(projectFgMenu(record,unqualified)&&!unqualified.pairEligible);
 intent.fgStrategy=FgStrategy::Single;intent.requestedMultiplier=2;
 intent.latency=LatencyProvider::RadeonAntiLag2;intent.latencyMode=LatencyMode::On;
 intent.fgEnabled=true;intent.fg=FgProvider::Nvidia;
 assert(encodeGraphicsRecord(intent,bytes)&&decodeGraphicsRecord(bytes,record));
 assert(projectDisplay(record).reflex==0);assert(!mcd2::amd_latency::resolve(record).enabled);
 // The host snapshot is bounded and carries only the committed request.
 MCD2MenuSnapshotV1 snapshot{sizeof(snapshot),123,0,0,{}};
 std::copy(bytes.begin(),bytes.end(),snapshot.record);
 assert(sizeof(snapshot)==144&&decodeGraphicsRecord(snapshot.record,record));
 if(argc==2){
  std::ifstream file(argv[1],std::ios::binary);assert(file);
  std::vector<std::uint8_t> request{std::istreambuf_iterator<char>(file),{}};
  MenuRequest decoded;assert(decodeMenuRequest(request,decoded));
  const auto& i=decoded.desired;
  assert(i.sr==SrProvider::AmdFsr&&i.srPreferences[1]==(SrPreference{Quality::Custom,7100,7100}));
  assert(i.srPreferences[0]==(SrPreference{Quality::Quality,6700,7700}));
  assert(i.fgEnabled&&i.fg==FgProvider::Nvidia&&i.latency==LatencyProvider::RadeonAntiLag2);
  assert(i.hdr==(HdrPreference{true,420,203,167}));
  MenuRequestBytes roundtrip;assert(encodeMenuRequest(decoded,roundtrip));
  assert(std::equal(request.begin(),request.end(),roundtrip.begin()));
 }
 std::cout<<"Shared display/latency/FG projection, invalid stamps, current pairings, startup ownership and optional exact C# request checks pass\n";
}
