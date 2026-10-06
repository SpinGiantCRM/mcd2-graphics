#include "fg_configuration.hpp"
#include <cassert>
#include <cstring>
#include <array>
int main() {
 using namespace mcd2::fg;
 for(unsigned available=0;available<2;++available)for(unsigned requested=0;requested<2;++requested)for(unsigned startup=0;startup<2;++startup)for(unsigned retired=0;retired<2;++retired)
  assert(restartRequired(available,requested,startup,retired)==bool(available&&(retired||requested!=startup)));
 for(unsigned mode=0;mode<2;++mode)for(unsigned current=0;current<2;++current)for(unsigned fault=0;fault<2;++fault)
  assert(permanentOff(mode,current,fault)==bool(fault||(current&&mode==0)));
 assert(!permanentOff(0,false,0)); // No UI acknowledgement yet.
 assert(!permanentOff(1,false,0)); // Previous-process saved On.
 Configuration c;
 const MCD2FGConfig on{48,1,3840,2160,2560,1440,24,61,41,34,3,1};
 assert(sameConfiguration(on,on));
 assert(sameConfiguration(configurationForPresent(on,{}),on));
 assert(sameConfiguration(configurationForPresent(on,on),on));
 for(unsigned width:{3840u,656u}) {
  auto resized=on;resized.motionWidth=width;resized.motionHeight=width==3840?2160:368;
  auto suspended=on;suspended.mode=0;
  assert(!sameConfiguration(resized,on));
  assert(sameConfiguration(configurationForPresent(resized,on),suspended));
  assert(sameConfiguration(configurationForPresent(resized,suspended),resized));
 }
 for(unsigned field=0;field<12;++field) {
  auto changed=on;
  // The contract consists exclusively of twelve uint32_t fields.
  std::array<uint32_t,12> values{};std::memcpy(values.data(),&changed,sizeof(changed));
  ++values[field];std::memcpy(&changed,values.data(),sizeof(changed));
  assert(!sameConfiguration(on,changed));
 }
 auto off=on;off.mode=0;off.motionWidth=3840;off.motionHeight=2160;
 auto first=c.prepare(off);assert(std::memcmp(&first,&off,sizeof(off))==0);
 auto prepared=c.prepare(on);assert(std::memcmp(&prepared,&on,sizeof(on))==0);
 assert(c.last.size==0); // A failed SDK call must not commit its configuration.
 c.accepted(prepared);
 auto suspended=c.prepare(off);auto expected=on;expected.mode=0;
 assert(sameConfiguration(configurationForPresent(off,on),expected));
 assert(std::memcmp(&suspended,&expected,sizeof(expected))==0);
 c.accepted(suspended);
 assert(c.prepare(off).motionWidth==2560);
 auto changed=on;changed.width=1920;changed.height=1080;changed.motionWidth=1280;changed.motionHeight=720;changed.backBuffers=2;
 prepared=c.prepare(changed);assert(std::memcmp(&prepared,&changed,sizeof(changed))==0);
 assert(c.prepare(off).motionWidth==2560); // Failed resize retains the accepted configuration.
 c.accepted(prepared);suspended=c.prepare(off);expected=changed;expected.mode=0;
 assert(std::memcmp(&suspended,&expected,sizeof(expected))==0);
 c.clear();assert(c.last.size==0);first=c.prepare(off);assert(std::memcmp(&first,&off,sizeof(off))==0);
}
