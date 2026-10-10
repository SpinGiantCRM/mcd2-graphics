#pragma once
// Native AA owns its temporal output. This independent consumer converts only
// FG guides and never evaluates an upscaler or changes the game's source scale.
#ifndef MCD2_NATIVE_FG_GUIDES
#define MCD2_NATIVE_FG_GUIDES 0
#endif
#if MCD2_NATIVE_FG_GUIDES
#include "../../experiments/fg-streamline/fg_camera_math.h"
#include "../providers/native_fg_admission.hpp"
namespace native_fg {
namespace p=mcd2::providers;
struct Generation : SrGuideGeneration {
 LeanBorrowCache borrows;a::command_queue *queue=nullptr;
 bool retiring=false,busy=false,gap=true;uint32_t previous=0,world=0;
};
// Final NT process teardown cannot release COM through an unloading renderer.
// Normal gameplay retirement and ordinary unload retain their existing cleanup.
static mcd2::process_exit::Lifetime<std::unique_ptr<Generation>,retain_sr_generation> generationLifetime;
static auto &generation=generationLifetime.get();
static uint64_t conversions=0,skips=0,retirements=0;
using Notify=void(*)(void*,void*,void*,const MCD2FGCamera*);
static Notify notify=nullptr;
static bool eligible=false;
static bool selected(){
 using namespace fsr_runtime;p::DecodedGraphicsRecord r;
 return menuValid&&GetTickCount64()-menuReadAt<=1000&&p::decodeGraphicsRecord(cachedMenu.record,r)&&
  p::nativeFgSelection(r);
}
static void record(a::command_list *cmd){
 if(!eligible||!generation)return;auto &c=*generation;
 p::DecodedGraphicsRecord authority;
 if(!fsr_runtime::menuValid||!p::decodeGraphicsRecord(fsr_runtime::cachedMenu.record,authority)||
  !p::nativeFgContext(authority,fsr_runtime::context,fsr_runtime::cachedMenu.session,GetTickCount64(),fsr_runtime::contextAt)){c.gap=true;return;}
 if(c.retiring||c.busy||c.borrows.blocked)return;
 const auto saved=commands[cmd];const auto slots=resolve(cmd,true);
 const Slot *colour=nullptr,*depth=nullptr,*packed=nullptr,*exposure=nullptr,*pass=nullptr,*view=nullptr,*out=nullptr;
 for(const auto &s:slots)if(s.space==0){
  if(s.binding.type==a::descriptor_type::shader_resource_view){if(s.reg==1)colour=&s;if(s.reg==2)depth=&s;if(s.reg==3)packed=&s;}
  if(s.binding.type==a::descriptor_type::buffer_shader_resource_view&&s.reg==0)exposure=&s;
  if(s.binding.type==a::descriptor_type::constant_buffer){if(s.reg==0)pass=&s;if(s.reg==1)view=&s;}
  if(s.binding.type==a::descriptor_type::unordered_access_view&&s.reg==0)out=&s;
 }
 auto skip=[&](){++skips;c.gap=true;};
 std::array<float,84> pc{};std::array<float,662> vc{};
 if(!colour||!depth||!packed||!exposure||!pass||!view||!out||!saved.pipeline||!saved.compute_layout||saved.dynamic_offsets||
  !root_push_supported(saved.cp)||!root_push_supported(saved.gp)||!read_upload(*pass,sizeof(pc),pc.data())||
  !read_upload(*view,sizeof(vc),vc.data())){skip();return;}
 for(unsigned i:{36u,37u,44u,45u})if(!std::isfinite(pc[i])||pc[i]<1||pc[i]>7680||pc[i]!=std::floor(pc[i])){skip();return;}
 std::array<uint32_t,4> rect{};memcpy(rect.data(),pc.data()+40,sizeof(rect));p::SrViewport viewport;
 if(!p::srViewport(unsigned(pc[36]),unsigned(pc[37]),rect,viewport)||viewport.width!=unsigned(pc[44])||viewport.height!=unsigned(pc[45])||
  !std::isfinite(vc[626])||vc[626]<=0||fsr_runtime::context.observedScaleMicro!=100000000||
  fsr_runtime::context.worldToMetersMilli!=100000){skip();return;}
 uint32_t stamp=0,reset=0;memcpy(&stamp,vc.data()+661,4);memcpy(&reset,pc.data()+12,4);
 MCD2FGCamera camera{};
 if(!mcd2_fg::camera(vc.data(),stamp,viewport.width,viewport.height,c.gap||reset||stamp!=c.previous+1||c.world!=fsr_runtime::context.world,camera)){skip();return;}
 auto res=[&](const Slot *s){return reinterpret_cast<ID3D12Resource*>(from_view(cmd->get_device(),s->binding.view).handle);};
 auto *gc=res(colour),*gd=res(depth),*gv=res(packed),*ge=res(exposure),*go=res(out);
 for(auto *r:{gc,gd,gv,ge,go})if(!r||!live_resources.contains(reinterpret_cast<uint64_t>(r))){skip();return;}
 for(auto *r:{gc,gd,gv}){auto d=r->GetDesc();if(d.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D||d.Width!=pc[36]||d.Height!=pc[37]||d.DepthOrArraySize!=1||d.SampleDesc.Count!=1){skip();return;}}
 const auto ev=view_desc(cmd->get_device(),exposure->binding.view);
 if(ev.buffer.offset>ge->GetDesc().Width||ge->GetDesc().Width-ev.buffer.offset<16||gc->GetDesc().Format!=DXGI_FORMAT_R11G11B10_FLOAT||
  static_cast<unsigned>(view_desc(cmd->get_device(),packed->binding.view).format)!=11||live_fixture||fsr_runtime_owns_source()){skip();return;}
 if(!c.proxy_device){
  auto *device=reinterpret_cast<ID3D12Device*>(cmd->get_device()->get_native());UINT bytes=sizeof(c.proxy_device);
  if(FAILED(device->GetPrivateData(observer_device_proxy,&bytes,&c.proxy_device))||!c.proxy_device||bytes!=sizeof(c.proxy_device)){c.proxy_device=nullptr;skip();return;}
  c.proxy_device->AddRef();c.owned.keep(c.proxy_device);
  if(FAILED(c.proxy_device->QueryInterface(observer_unwrapped,reinterpret_cast<void**>(&c.resource_device)))){c.retiring=true;skip();return;}
  c.owned.keep(c.resource_device);c.width=viewport.width;c.height=viewport.height;
  c.motion=eval_texture(c.resource_device,c.owned,c.width,c.height,DXGI_FORMAT_R16G16_FLOAT,true);
  c.exposure=eval_texture(c.resource_device,c.owned,1,1,DXGI_FORMAT_R32_FLOAT,true);
  if(!c.motion||!c.exposure){c.retiring=true;skip();return;}
 }
 if(c.width!=viewport.width||c.height!=viewport.height){c.retiring=true;skip();return;}
 auto *proxy=validated_game_proxy(cmd);if(!proxy){skip();return;}
 ID3D12Device *device=nullptr;bool matched=SUCCEEDED(proxy->GetDevice(IID_PPV_ARGS(&device)))&&device==c.proxy_device;
 if(device)device->Release();if(!matched){proxy->Release();skip();return;}
 SrGuideInput input{gc,gd,gv,ge,go,*pass,*view,ev,static_cast<unsigned>(view_desc(cmd->get_device(),depth->binding.view).format)};
 input.converter_asset="fsr_dense.cso";
 const auto *error=record_sr_guides(c,c.borrows,cmd,proxy,saved,input);
 if(error){proxy->Release();c.retiring=true;skip();return;}
 // This conversion leaves the native temporal shader to execute normally.
 notify(proxy,c.current_depth,c.motion,&camera);proxy->Release();++conversions;
 c.previous=stamp;c.world=fsr_runtime::context.world;c.gap=false;
}
static void present(a::command_queue *q,std::unique_lock<std::recursive_mutex>& guard){
 auto observer=GetModuleHandleW(L"mcd2-fg-guide-recon.addon64");
 notify=observer?reinterpret_cast<Notify>(GetProcAddress(observer,"mcd2_fg_observe_sr")):nullptr;
 auto wanted=observer?reinterpret_cast<int(*)()>(GetProcAddress(observer,"mcd2_fg_inputs_wanted")):nullptr;
 const bool request=selected();
 eligible=request&&notify&&wanted&&wanted()==1&&fsr_runtime::current&&GetTickCount64()-fsr_runtime::contextAt<=3000;
 if(!generation){if(eligible){generation=std::make_unique<Generation>();generation->queue=q;}return;}
 auto &c=*generation;if(c.busy)return;if(!eligible)c.gap=true;
 if(!request)c.retiring=true;
 if(c.queue!=q){c.borrows.blocked=true;eligible=false;return;}
 auto &cache=c.borrows;const auto pending=cache.reset_pending;
 for(auto *cmd:pending){uint64_t epoch=0;if(lean_reset_epoch(cmd,epoch))cache.confirm_reset(cmd,epoch);}
 auto *queue=reinterpret_cast<ID3D12CommandQueue*>(q->get_native());
 if(cache.fence&&!cache.blocked){bool signal=false;for(const auto &[key,entry]:cache.entries)signal|=entry.awaiting_signal;
  if(signal){auto value=cache.next_fence+1;if(FAILED(queue->Signal(cache.fence,value)))cache.blocked=true;
   else{cache.next_fence=value;for(auto &[key,entry]:cache.entries)if(entry.awaiting_signal){entry.awaiting_signal=false;entry.retirement_fence=value;}}}
  const auto done=cache.fence->GetCompletedValue();if(done==UINT64_MAX)cache.blocked=true;else{cache.completed_fence=done;cache.retire_ready(done,frame);}
 }
 auto released=std::move(cache.ready_to_release);cache.ready_to_release.clear();c.busy=true;guard.unlock();
 for(auto &entry:released)LeanBorrowCache::release(entry);guard.lock();c.busy=false;
 if(c.retiring&&cache.recordings.empty()&&cache.entries.empty()&&!cache.blocked){
  auto retired=std::move(generation);guard.unlock();retired->borrows.clear_after_idle();
  for(auto it=retired->owned.resources.rbegin();it!=retired->owned.resources.rend();++it)if(*it)(*it)->Release();
  retired->owned.resources.clear(); // Manual reverse release must not run twice.
  retired.reset();guard.lock();++retirements;
 }
}
}
#endif
static void native_fg_record(a::command_list *cmd){
#if MCD2_NATIVE_FG_GUIDES
 native_fg::record(cmd);
#endif
}
static void native_fg_present(a::command_queue *q,std::unique_lock<std::recursive_mutex>& guard){
#if MCD2_NATIVE_FG_GUIDES
 native_fg::present(q,guard);
#endif
}
static void native_fg_begin(a::command_list *cmd){
#if MCD2_NATIVE_FG_GUIDES
 if(native_fg::generation)native_fg::generation->borrows.begin_recording(cmd);
#endif
}
static void native_fg_reset(a::command_list *cmd){
#if MCD2_NATIVE_FG_GUIDES
 if(!native_fg::generation)return;auto &cache=native_fg::generation->borrows;
 if(cache.recordings.contains(cmd)){uint64_t epoch=0;bool known=lean_reset_epoch(cmd,epoch,true);cache.note_reset(cmd,known,epoch);}
#endif
}
static void native_fg_forget(a::command_list *cmd){
#if MCD2_NATIVE_FG_GUIDES
 if(native_fg::generation){auto &cache=native_fg::generation->borrows;cache.reset_pending.erase(cmd);cache.reset_epochs.erase(cmd);cache.forget(cmd);}
#endif
}
static void native_fg_submit(a::command_queue *q,a::command_list *cmd){
#if MCD2_NATIVE_FG_GUIDES
 if(native_fg::generation&&native_fg::generation->borrows.recordings.contains(cmd)&&native_fg::generation->queue!=q)native_fg::generation->borrows.blocked=true;
#endif
}
static void native_fg_destroy_queue(a::command_queue *q){
#if MCD2_NATIVE_FG_GUIDES
 if(native_fg::generation&&native_fg::generation->queue==q){native_fg::generation->borrows.blocked=true;native_fg::generation->queue=nullptr;native_fg::eligible=false;}
#endif
}
