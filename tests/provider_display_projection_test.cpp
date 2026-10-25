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
 for(unsigned provider=0;provider<3;++provider)for(bool enabled:{false,true})for(bool eligible:{false,true}){
  FgMenuProjection selection{42,provider,enabled,eligible,0};
  const unsigned expected=eligible&&provider<2&&enabled?provider+1:0;
  assert(fgStartupOwner(selection)==expected);
 }
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
  assert(projection.pairEligible==((fg==0&&(sr==1||sr==2))||(fg==1&&(sr==0||sr==1||sr==2))));
  assert(fgMenuMatches(projection,42,fg,1));
  assert(!fgMenuMatches(projection,41,fg,1)&&!fgMenuMatches(projection,42,fg,0));
  assert(fgOwnerMatches(fg+1,fg)==(fg<2));assert(!fgOwnerMatches(0,fg));
  record.bootstrap.stamp.checksum^=1;assert(!projectFgMenu(record,projection));
 }
 intent.sr=SrProvider::AmdFsr;intent.fgStrategy=FgStrategy::NativeMfg;intent.requestedMultiplier=3;
 // Preserve ownership changes and unsupported refusal while requiring restart
 // after a permanent NVIDIA release, even when the startup owner still matches.
 for(unsigned available=0;available<2;++available)for(unsigned enabled=0;enabled<2;++enabled)
 for(unsigned owner=0;owner<4;++owner)for(unsigned provider=0;provider<3;++provider)
 for(unsigned retired=0;retired<2;++retired){
  const bool matching=provider<2&&owner==provider+1;
  const bool expected=available&&(retired||(enabled?!matching:owner!=0));
  assert(fgMenuRestartRequired(available,enabled,owner,provider,retired)==expected);
 }
 assert(fgMenuRestartRequired(true,true,1,0,true));
 assert(!fgMenuRestartRequired(true,true,1,0,false));
 assert(!fgMenuRestartRequired(true,true,2,1,false));
 assert(fgMenuRestartRequired(true,false,2,1,false));
 // Clear the status only after a current, fault-free AMD Off request has an
 // actual native presenter and the bridge confirms live switching is available.
 for(unsigned current=0;current<2;++current)for(unsigned available=0;available<2;++available)
 for(unsigned enabled=0;enabled<2;++enabled)for(unsigned provider=0;provider<3;++provider)
 for(unsigned route=0;route<4;++route)for(unsigned canSwitch=0;canSwitch<3;++canSwitch)
 for(unsigned fault=0;fault<2;++fault)for(unsigned retired=0;retired<2;++retired){
  const bool expected=current&&available&&!enabled&&provider==1&&route==1&&canSwitch==1&&!fault&&!retired;
  assert(fgMenuNativeOffRecovered(current,available,enabled,provider,route,canSwitch,fault,retired)==expected);
 }
 assert(fgMenuRestartRequired(true,false,2,1,true) &&
        !fgMenuNativeOffRecovered(true,true,false,1,1,1,0,true));
 assert(encodeGraphicsRecord(intent,bytes)&&decodeGraphicsRecord(bytes,record));
 FgMenuProjection unqualified;assert(projectFgMenu(record,unqualified)&&!unqualified.pairEligible);
 intent.fgStrategy=FgStrategy::Single;intent.requestedMultiplier=2;
 // FSR quality does not select the FG vendor. Both initial startup and later
 // menu projection must admit the independently selected NVIDIA route.
 intent.sr=SrProvider::AmdFsr;intent.fg=FgProvider::Nvidia;
 for(unsigned quality=0;quality<6;++quality)for(unsigned on=0;on<2;++on){
  intent.srPreferences[1].quality=Quality(quality);intent.fgEnabled=on!=0;
  assert(encodeGraphicsRecord(intent,bytes)&&decodeGraphicsRecord(bytes,record));
  FgMenuProjection cross;assert(projectFgMenu(record,cross)&&cross.pairEligible);
  assert(cross.provider==0&&cross.enabled==(on!=0));
  assert(record.bootstrap.requested==(on?PresentationOwner::NvidiaStreamline:PresentationOwner::Native));
  assert(fgMenuMatches(cross,42,0,on));
 }
 // Unimplemented sources, strategies and generated-frame counts stay refused.
 for(auto sr:{SrProvider::Native,SrProvider::IntelXeSs}){
  intent.sr=sr;assert(encodeGraphicsRecord(intent,bytes)&&decodeGraphicsRecord(bytes,record));
  assert(projectFgMenu(record,unqualified)&&!unqualified.pairEligible);
 }
 intent.sr=SrProvider::AmdFsr;
 for(auto strategy:{FgStrategy::NativeMfg,FgStrategy::Hybrid}){
  intent.fgStrategy=strategy;intent.requestedMultiplier=3;
  assert(encodeGraphicsRecord(intent,bytes)&&decodeGraphicsRecord(bytes,record));
  assert(projectFgMenu(record,unqualified)&&unqualified.pairEligible==(strategy==FgStrategy::NativeMfg));
 }
 for(unsigned multiplier=2;multiplier<=7;++multiplier){
  intent.fgStrategy=FgStrategy::NativeMfg;intent.requestedMultiplier=multiplier;
  assert(encodeGraphicsRecord(intent,bytes)&&decodeGraphicsRecord(bytes,record));
  assert(projectFgMenu(record,unqualified)&&unqualified.pairEligible==(multiplier<=6));
  assert(unqualified.frameMode==multiplier-2);
 }
 intent.fgStrategy=static_cast<FgStrategy>(4);intent.requestedMultiplier=2;
 assert(!encodeGraphicsRecord(intent,bytes));
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
