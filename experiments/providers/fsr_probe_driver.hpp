#pragma once
#include "fsr_game_bridge.h"
// Only copied into an explicitly built developer probe. Not a released provider.
struct FsrProbeState {
 HMODULE bridge=nullptr;void *session=nullptr;
 decltype(&mcd2_fsr_create_v1) create=nullptr;
 decltype(&mcd2_fsr_dispatch_v1) dispatch=nullptr;
 decltype(&mcd2_fsr_info_v1) info=nullptr;
 decltype(&mcd2_fsr_destroy_v1) destroy=nullptr;
 ID3D12Resource *output=nullptr;
 unsigned output_width=0,output_height=0;
 bool pending=false,ready=false,readable=false;
 LARGE_INTEGER previous{},frequency{};
};
static bool fsr_probe_camera(const std::array<float,84> &pc,const std::array<float,632> &vc){
 std::array<uint32_t,4> rect{};memcpy(rect.data(),pc.data()+40,sizeof(rect));
 if(rect!=std::array<uint32_t,4>{0,0,unsigned(pc[36])-1,unsigned(pc[37])-1})return false;
 const auto *p=vc.data()+128;
 for(unsigned i=0;i<16;++i)if(!std::isfinite(p[i]))return false;
 return p[0]>0 && p[5]>0 && std::abs(p[10])<1e-6 && std::abs(p[11]-1)<1e-6
  && p[14]>0 && std::abs(p[15])<1e-6 && std::abs(p[8])<1e-6 && std::abs(p[9])<1e-6;
}
template<class Capture>
static bool fsr_probe_prepare(Capture &c,const std::array<float,84> &pc){
 if(c.fsr.ready)return true;
 if(c.fsr.pending)return false;
 c.fsr.output_width=unsigned(pc[44]);c.fsr.output_height=unsigned(pc[45]);
 c.fsr.pending=true;return false;
}
template<class Capture>
static int fsr_probe_create(Capture &c){
 auto &s=c.fsr;const auto folder=asset_root/"FSRGameProbe";
 s.bridge=LoadLibraryExW((folder/"mcd2-fsr-game-bridge.dll").c_str(),nullptr,
  LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32);
 if(!s.bridge)return -1100;
 s.create=reinterpret_cast<decltype(s.create)>(GetProcAddress(s.bridge,"mcd2_fsr_create_v1"));
 s.dispatch=reinterpret_cast<decltype(s.dispatch)>(GetProcAddress(s.bridge,"mcd2_fsr_dispatch_v1"));
 s.info=reinterpret_cast<decltype(s.info)>(GetProcAddress(s.bridge,"mcd2_fsr_info_v1"));
 s.destroy=reinterpret_cast<decltype(s.destroy)>(GetProcAddress(s.bridge,"mcd2_fsr_destroy_v1"));
 if(!s.create||!s.dispatch||!s.info||!s.destroy)return -1101;
 internal_evaluation=true;
 const int code=s.create(c.proxy_device,(folder/"amd_fidelityfx_upscaler_dx12.dll").c_str(),
  c.width,c.height,s.output_width,s.output_height,&s.session);
 internal_evaluation=false;
 if(code)return code;
 s.output=eval_texture(c.resource_device,c.owned,s.output_width,s.output_height,DXGI_FORMAT_R16G16B16A16_FLOAT,true);
 if(!s.output)return -1102;
 if(!QueryPerformanceFrequency(&s.frequency)||s.frequency.QuadPart<=0)return -1103;
 QueryPerformanceCounter(&s.previous);s.ready=true;return 0;
}
template<class Capture>
static int fsr_probe_evaluate(Capture &c,ID3D12GraphicsCommandList *proxy,ID3D12GraphicsCommandList *native,
 ID3D12Resource *colour,const std::array<float,84> &pc,const std::array<float,632> &vc){
 auto &s=c.fsr;LARGE_INTEGER now{};QueryPerformanceCounter(&now);
 float delta=float(1000.*double(now.QuadPart-s.previous.QuadPart)/double(s.frequency.QuadPart));s.previous=now;
 // Trial-only centimetre assumption is recorded explicitly and must be verified
 // before this becomes a supported rendering path. Near plane itself is captured.
 MCD2FsrDispatchV1 p{sizeof(p),c.width,c.height,s.output_width,s.output_height,c.samples.empty()?1u:0u,
  vc[576]*c.width*.5f,-vc[577]*c.height*.5f,1.f,1.f,delta,vc[626],
  vc[142],std::numeric_limits<float>::infinity(),2.f*std::atan(1.f/vc[133]),.01f};
 if(s.readable)eval_transition(native,s.output,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
 internal_evaluation=true;const int code=s.dispatch(s.session,proxy,colour,c.current_depth,c.motion,c.exposure,s.output,&p);internal_evaluation=false;
 c.log<<"{\"stage\":\"fsr_dispatch\",\"code\":"<<code<<",\"sample\":"<<c.samples.size()
  <<",\"frameTimeMs\":"<<delta<<",\"reset\":"<<p.reset<<",\"preExposure\":"<<p.preExposure
  <<",\"cameraNear\":"<<p.cameraNear<<",\"worldUnitsToMetersTrial\":0.01,\"worldUnitsVerified\":false,\"NGXContextPresent\":"<<(live_fixture?"true":"false")
  <<",\"outputReplaced\":false}\n";c.log.flush();
 D3D12_RESOURCE_BARRIER order{};order.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;order.UAV.pResource=s.output;native->ResourceBarrier(1,&order);
 eval_transition(native,s.output,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);s.readable=true;
 return code;
}
template<class Capture>
static void fsr_probe_log(Capture &c,const char *stage,int result){
 MCD2FsrInfoV1 info{};info.size=sizeof(info);if(c.fsr.session&&c.fsr.info)c.fsr.info(c.fsr.session,&info);
 c.log<<"{\"stage\":\""<<stage<<"\",\"code\":"<<result<<",\"providerId\":"<<info.providerId
  <<",\"SDKStage\":"<<info.reserved<<",\"dispatches\":"<<info.dispatches<<",\"errors\":"<<info.errors
  <<",\"warnings\":"<<info.warnings<<",\"requiredInputs\":"<<info.requiredResources<<",\"optionalInputs\":"<<info.optionalResources<<"}\n";c.log.flush();
}
template<class Capture>
static int fsr_probe_retire(Capture &c){
 auto &s=c.fsr;
 if(s.session){const int code=s.destroy(s.session,c.borrows.fence,c.borrows.next_fence);if(code)return code;s.session=nullptr;}
 if(s.bridge){FreeLibrary(s.bridge);s.bridge=nullptr;}return 0;
}
