#include "fg_configuration.hpp"
#include "fg_frame_inputs.hpp"
#include "streamline_feature_policy.hpp"
#include <cassert>
#include <cstring>
#include <array>
int main() {
 using namespace mcd2::fg;
 for(unsigned maximum=0;maximum<=5;++maximum)for(bool dynamic:{false,true}){
  for(unsigned mode=0;mode<=6;++mode){
   assert(supportsFrameMode(mode,maximum,dynamic)==(mode<5?mode+1<=maximum:mode==5&&dynamic));
   if(mode<=5)assert(generatedFrames(mode)==(mode==5?0:mode+1));
  }
  for(unsigned generated=0;generated<=6;++generated)
   assert(supportsGeneratedFrames(generated,maximum,dynamic)==(generated?generated<=5&&generated<=maximum:dynamic));
 }

 assert(loadNvidiaFg(0)&&loadNvidiaFg(1)&&!loadNvidiaFg(2));
 NvidiaFgCapabilities capabilities;
 assert(capabilities.find({1,2})==-1);
 assert(capabilities.remember({1,2},0));
 assert(capabilities.find({1,2})==0&&capabilities.find({1,3})==-1);
 assert(capabilities.remember({1,2},-17));
 assert(capabilities.find({1,2})==-17);
 for(unsigned i=2;i<=16;++i)assert(capabilities.remember({i,-1},int(i)));
 assert(!capabilities.remember({17,-1},0));
 assert(capabilities.find({16,-1})==16&&capabilities.find({17,-1})==-1);
 capabilities.clear();assert(capabilities.find({1,2})==-1);
 for(unsigned available=0;available<2;++available)for(unsigned requested=0;requested<2;++requested)for(unsigned startup=0;startup<2;++startup)for(unsigned retired=0;retired<2;++retired)
  assert(restartRequired(available,requested,startup,retired)==bool(available&&(retired||(requested&&!startup))));
 assert(!permanentOff(0));
 assert(permanentOff(1));
 assert(!restartRequired(true,0,1,false)); // User Off preserves the routed session.
 assert(!restartRequired(true,1,1,false)); // It can resume without a restart.
 assert(restartRequired(true,1,0,false)); // No presentation owner cannot enable FG.
 assert(restartRequired(true,1,1,true)); // Fault retirement remains terminal.
 Configuration c;
 assert(!nvidiaInputsReady(false,false,false));
 assert(!nvidiaInputsReady(false,true,true));
 assert(!nvidiaInputsReady(true,false,false));
 assert(nvidiaInputsReady(true,true,false));
 assert(nvidiaInputsReady(true,false,true));
 MCD2FGCamera camera{};camera.frame=42;camera.width=1920;camera.height=1080;
 FrameInputs colorFirst{};colorFirst.sdrFinalColor=true;
 assert(!nvidiaInputsReady(colorFirst.guides,colorFirst.images,colorFirst.sdrFinalColor));
 colorFirst.setGuides(camera,true);
 assert(nvidiaInputsReady(colorFirst.guides,colorFirst.images,colorFirst.sdrFinalColor)&&colorFirst.camera.frame==42);
 FrameInputs guidesFirst{};guidesFirst.setGuides(camera,true);
 assert(!nvidiaInputsReady(guidesFirst.guides,guidesFirst.images,guidesFirst.sdrFinalColor));
 guidesFirst.sdrFinalColor=true;
 assert(nvidiaInputsReady(guidesFirst.guides,guidesFirst.images,guidesFirst.sdrFinalColor));
 colorFirst.images=true;colorFirst.setGuides(camera,false);
 assert(colorFirst.images&&colorFirst.sdrFinalColor&&!nvidiaInputsReady(colorFirst.guides,colorFirst.images,colorFirst.sdrFinalColor));
 colorFirst.setGuides(camera,true);
 assert(colorFirst.images&&colorFirst.sdrFinalColor&&nvidiaInputsReady(colorFirst.guides,colorFirst.images,colorFirst.sdrFinalColor));
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
 for(unsigned frames=0;frames<=5;++frames){
  auto changed=on;changed.generatedFrames=frames;
  auto suspended=configurationForPresent(changed,on);
  assert(suspended.mode==(frames==1?1u:0u));
  assert(sameConfiguration(configurationForPresent(changed,suspended),changed));
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
 for(unsigned cycle=0;cycle<3;++cycle){
  const auto resumed=configurationForPresent(on,c.last);
  assert(sameConfiguration(resumed,on));c.accepted(resumed);
  const auto paused=configurationForPresent(off,c.last);
  assert(!paused.mode&&paused.motionWidth==on.motionWidth);c.accepted(paused);
 }

 auto changed=on;changed.width=1920;changed.height=1080;changed.motionWidth=1280;changed.motionHeight=720;changed.backBuffers=2;
 prepared=c.prepare(changed);assert(std::memcmp(&prepared,&changed,sizeof(changed))==0);
 assert(c.prepare(off).motionWidth==2560); // Failed resize retains the accepted configuration.
 c.accepted(prepared);suspended=c.prepare(off);expected=changed;expected.mode=0;
 assert(std::memcmp(&suspended,&expected,sizeof(expected))==0);
 c.clear();assert(c.last.size==0);first=c.prepare(off);assert(std::memcmp(&first,&off,sizeof(off))==0);
}
