#include "../src/providers/fg_output_contract.hpp"
#include <cassert>
#include <limits>
#include <initializer_list>
using namespace mcd2::providers;
int main(){
 FgSceneReset lease{};assert(!lease.confirmed(true,1));
 for(bool known:{false,true})for(auto before:{0ull,1ull,999ull}){
  lease.request(known,before);assert(!lease.confirmed(false,before+1));assert(!lease.confirmed(true,before));assert(!lease.confirmed(true,UINT64_MAX));assert(lease.confirmed(true,before+1)==known);
 }
 FgOutputContract sdr{3840,2160,24,24,FgOutputColour::Srgb,FgSceneStage::SdrOpaqueSnapshot};
 auto pq=sdr;pq.colour=FgOutputColour::Pq2020;pq.stage=FgSceneStage::HdrCompositor;
 assert(validFgOutput(sdr)&&validFgOutput(pq)&&!fgOutputHdr(sdr)&&fgOutputHdr(pq));
 for(unsigned back:{24u,28u,87u}){auto p=sdr;p.backbufferFormat=back;p.sceneFormat=back==87?28:back;assert(validFgOutput(p));}
 for(unsigned colour=0;colour<9;++colour)for(unsigned format:{0u,10u,24u,28u,87u,UINT32_MAX})for(unsigned scene:{0u,24u,28u,87u})for(unsigned stage=0;stage<5;++stage){
  auto p=sdr;p.colour=FgOutputColour(colour);p.backbufferFormat=format;p.sceneFormat=scene;p.stage=FgSceneStage(stage);
  const bool valid=(colour==3&&stage==1&&format==24&&scene==24)||(colour==1&&stage==2&&((format==scene&&(scene==24||scene==28))||(format==87&&scene==28)));
  assert(validFgOutput(p)==valid);assert(fgOutputHdr(p)==(valid&&colour==3));
 }
 for(auto dimension:{0u,UINT32_MAX}){auto p=sdr;p.width=dimension;assert(!validFgOutput(p));p=sdr;p.height=dimension;assert(!validFgOutput(p));}
 auto edge=sdr;edge.width=7680;edge.height=4320;assert(validFgOutput(edge));++edge.width;assert(!validFgOutput(edge));--edge.width;++edge.height;assert(!validFgOutput(edge));
 SdrSceneDraw d{123,0x088c6818,3,1,7,true,true,false,true,true,true,0,0,3840,2160,0,0,3840,2160};
 assert(admitSdrSceneDraw(sdr,d));assert(!admitSdrSceneDraw(pq,d));
 for(unsigned mask=0;mask<16;++mask){auto bad=d;bad.writeMask=mask;assert(admitSdrSceneDraw(sdr,bad)==((mask&7)==7));}
 auto bad=d;bad.frame=UINT32_MAX;assert(!admitSdrSceneDraw(sdr,bad));bad=d;bad.shader^=1;assert(!admitSdrSceneDraw(sdr,bad));
 for(auto member:{&SdrSceneDraw::indexed,&SdrSceneDraw::blendKnown,&SdrSceneDraw::primaryBackbuffer,&SdrSceneDraw::viewportKnown,&SdrSceneDraw::scissorKnown}){bad=d;bad.*member=false;assert(!admitSdrSceneDraw(sdr,bad));}
 bad=d;bad.blendEnabled=true;assert(!admitSdrSceneDraw(sdr,bad));
 for(auto member:{&SdrSceneDraw::x,&SdrSceneDraw::y,&SdrSceneDraw::width,&SdrSceneDraw::height}){bad=d;bad.*member+=1;assert(!admitSdrSceneDraw(sdr,bad));bad.*member=std::numeric_limits<float>::quiet_NaN();assert(!admitSdrSceneDraw(sdr,bad));}
 for(auto member:{&SdrSceneDraw::left,&SdrSceneDraw::top,&SdrSceneDraw::right,&SdrSceneDraw::bottom}){bad=d;bad.*member+=1;assert(!admitSdrSceneDraw(sdr,bad));}
 bad=d;bad.elements=6;assert(!admitSdrSceneDraw(sdr,bad));bad=d;bad.instances=2;assert(!admitSdrSceneDraw(sdr,bad));
}
