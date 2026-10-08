#pragma once
#include <bcrypt.h>
#include "../../experiments/providers/fsr_game_bridge.h"
#include "../providers/sr_runtime_protocol.hpp"
#include "../providers/sr_runtime_snapshot.h"
#ifndef MCD2_FSR_BRIDGE_SHA256
#define MCD2_FSR_BRIDGE_SHA256 ""
#endif
namespace fsr_runtime {
namespace p=mcd2::providers;
static p::SrRuntimeState state;
static p::SrContext context;
static std::uint32_t requestSequence=0;
static bool requested=false,current=false,contextAvailable=false,restoring=false;
static std::uint64_t contextAt=0;
static p::RecordStamp rejected{};
static MCD2MenuSnapshotV1 cachedMenu{};
static MCD2SrContextSnapshotV1 cachedContext{};
static std::uint64_t menuReadAt=0;
struct Generation : SrGuideGeneration {
 LeanBorrowCache borrows;a::command_queue *queue=nullptr;
 HMODULE bridge=nullptr;void *session=nullptr;ID3D12Resource *output=nullptr;
 decltype(&mcd2_fsr_create_v1) create=nullptr;decltype(&mcd2_fsr_dispatch_v1) dispatch=nullptr;
 decltype(&mcd2_fsr_quality_v1) quality=nullptr;decltype(&mcd2_fsr_destroy_v1) destroy=nullptr;
 decltype(&mcd2_fsr_info_v1) info=nullptr;
 bool busy=false,history_dirty=false,readable=false,observed=false,force_reset=false;
 unsigned observedWidth=0,observedHeight=0,outputWidth=0,outputHeight=0;
 std::uint64_t started=0;std::uint32_t previousId=0;LARGE_INTEGER previous{},frequency{};
 float delta=0;bool gap=false;
};
static std::unique_ptr<Generation> generation;
static void phase(p::SrPhase next,unsigned error=0){
 if(state.phase!=next||state.error!=error){state.phase=next;state.error=error;
  if(requestSequence<0x7fffffffu)state.sequence=++requestSequence;else state.phase=p::SrPhase::Failed;
 }
}
static bool hash_matches(const fs::path& file,const char *expected){
 if(!expected||strlen(expected)!=64)return false;
 std::ifstream input(file,std::ios::binary);if(!input)return false;
 BCRYPT_ALG_HANDLE algorithm=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;
 if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)return false;
 DWORD bytes=0,needed=0;bool ok=BCryptGetProperty(algorithm,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&needed),sizeof(needed),&bytes,0)>=0;
 std::vector<unsigned char> object(needed),buffer(65536);std::array<unsigned char,32> digest{};
 ok=ok&&BCryptCreateHash(algorithm,&hash,object.data(),needed,nullptr,0,0)>=0;
 while(ok&&input){input.read(reinterpret_cast<char*>(buffer.data()),buffer.size());auto n=input.gcount();
  if(n>0)ok=BCryptHashData(hash,buffer.data(),ULONG(n),0)>=0;
 }
 ok=ok&&!input.bad()&&BCryptFinishHash(hash,digest.data(),digest.size(),0)>=0;
 if(hash)BCryptDestroyHash(hash);BCryptCloseAlgorithmProvider(algorithm,0);
 if(!ok)return false;
 constexpr char hex[]="0123456789abcdef";
 for(unsigned n=0;n<32;n++)if(expected[n*2]!=hex[digest[n]>>4]||expected[n*2+1]!=hex[digest[n]&15])return false;
 return true;
}
static void stop(unsigned error,bool reject=true){
 if(!generation)return;
 if(state.phase!=p::SrPhase::Retire){if(reject)rejected=state.intent;phase(p::SrPhase::Retire,error);}
}
static bool native_reset(Generation& c,a::command_list *cmd,const Cmd& saved,const Slot& pass,
                         const std::array<float,84>& data,uint32_t x,uint32_t y,uint32_t z){
 if(!c.history_dirty)return false;auto &cache=c.borrows;
 if(!c.resource_device||cache.blocked||cache.entries.size()>=256||!saved.cp.contains(pass.param))return false;
 if(!cache.fence&&FAILED(c.resource_device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&cache.fence))))return false;
 EvalOwned owned;auto *buffer=eval_buffer(c.resource_device,owned,512,D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ);
 if(!buffer)return false;void *mapped=nullptr;D3D12_RANGE empty{0,0};
 if(FAILED(buffer->Map(0,&empty,&mapped))){buffer->Release();owned.resources.clear();return false;}
 memset(mapped,0,512);memcpy(mapped,data.data(),sizeof(data));uint32_t reset=1;memcpy(static_cast<char*>(mapped)+48,&reset,4);buffer->Unmap(0,nullptr);
 LeanBorrowKey key{reinterpret_cast<uint64_t>(buffer),0,UINT_MAX,0,0,0,0,0};LeanBorrowEntry entry;entry.resources[0]=buffer;
 cache.entries.emplace(key,entry);owned.resources.clear();cache.record(cmd,key,frame);
 a::buffer_range range{{reinterpret_cast<uint64_t>(buffer)},0,512};a::descriptor_table_update update{};
 update.binding=pass.rangebinding;update.count=1;update.type=a::descriptor_type::constant_buffer;update.descriptors=&range;
 cmd->push_descriptors(a::shader_stage::all_compute,{saved.compute_layout},pass.param,update);
 reinterpret_cast<ID3D12GraphicsCommandList*>(cmd->get_native())->Dispatch(x,y,z);
 restore_root(cmd,saved,false,true);restore_root(cmd,saved,true,true);cmd->bind_pipeline(a::pipeline_stage::all_compute,{saved.pipeline});commands[cmd]=saved;
 c.history_dirty=false;return true;
}
static bool camera(const std::array<float,84>& pc,const std::array<float,662>& vc){
 std::array<uint32_t,4> rect{};memcpy(rect.data(),pc.data()+40,sizeof(rect));
 if(rect!=std::array<uint32_t,4>{0,0,unsigned(pc[36])-1,unsigned(pc[37])-1})return false;
 for(unsigned i=128;i<144;++i)if(!std::isfinite(vc[i]))return false;
 return vc[128]>0&&vc[133]>0&&std::abs(vc[138])<1e-6&&std::abs(vc[139]-1)<1e-6&&vc[142]>0&&
  std::abs(vc[143])<1e-6&&std::abs(vc[136])<1e-6&&std::abs(vc[137])<1e-6;
}
static bool bind_device(Generation& c,a::command_list *cmd){
 if(c.proxy_device)return true;
 auto *native=reinterpret_cast<ID3D12Device*>(cmd->get_device()->get_native());UINT bytes=sizeof(c.proxy_device);
 if(FAILED(native->GetPrivateData(observer_device_proxy,&bytes,&c.proxy_device))||bytes!=sizeof(c.proxy_device)||!c.proxy_device){c.proxy_device=nullptr;return false;}
 c.proxy_device->AddRef();c.owned.keep(c.proxy_device);
 if(FAILED(c.proxy_device->QueryInterface(observer_unwrapped,reinterpret_cast<void**>(&c.resource_device))))return false;
 c.owned.keep(c.resource_device);return true;
}
static bool copy_output(Generation& c,ID3D12GraphicsCommandList *native,ID3D12Resource *target){
 auto source=c.output->GetDesc(),out=target->GetDesc();
 if(source.Width!=out.Width||source.Height!=out.Height||source.Format!=out.Format||source.DepthOrArraySize!=out.DepthOrArraySize||
  source.MipLevels!=out.MipLevels||source.SampleDesc.Count!=out.SampleDesc.Count||source.SampleDesc.Quality!=out.SampleDesc.Quality||
  source.Dimension!=out.Dimension||target==c.output)return false;
 eval_transition(native,c.output,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_SOURCE);
 eval_transition(native,target,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_DEST);native->CopyResource(target,c.output);
 eval_transition(native,target,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
 eval_transition(native,c.output,D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);c.history_dirty=true;return true;
}
static bool record(a::command_list *cmd,uint32_t x,uint32_t y,uint32_t z){
 if(!generation)return false;auto &c=*generation;if(c.busy)return c.history_dirty;
 const Cmd saved=commands[cmd];const auto slots=resolve(cmd,true);
 const Slot *colour=nullptr,*depth=nullptr,*packed=nullptr,*exposure=nullptr,*pass=nullptr,*view=nullptr,*out=nullptr;
 for(const auto &s:slots)if(s.space==0){
  if(s.binding.type==a::descriptor_type::shader_resource_view){if(s.reg==1)colour=&s;if(s.reg==2)depth=&s;if(s.reg==3)packed=&s;}
  if(s.binding.type==a::descriptor_type::buffer_shader_resource_view&&s.reg==0)exposure=&s;
  if(s.binding.type==a::descriptor_type::constant_buffer){if(s.reg==0)pass=&s;if(s.reg==1)view=&s;}
  if(s.binding.type==a::descriptor_type::unordered_access_view&&s.reg==0)out=&s;
 }
 std::array<float,84> pc{};std::array<float,662> vc{};
 auto fail=[&](unsigned error){stop(error);if(!c.history_dirty)return false;
  if(pass&&saved.pipeline&&saved.compute_layout&&!saved.dynamic_offsets&&root_push_supported(saved.cp)&&root_push_supported(saved.gp)&&
   read_upload(*pass,sizeof(pc),pc.data())&&native_reset(c,cmd,saved,*pass,pc,x,y,z))return true;
  return true; // Retain resources and suppress contaminated Native history.
 };
 auto transient=[&](){
  c.force_reset=true;
  if(state.phase==p::SrPhase::Active){phase(p::SrPhase::Evaluate);c.started=GetTickCount64();}
  if(!c.history_dirty)return false;
  if(pass&&saved.pipeline&&saved.compute_layout&&!saved.dynamic_offsets&&root_push_supported(saved.cp)&&root_push_supported(saved.gp)&&
   read_upload(*pass,sizeof(pc),pc.data())&&native_reset(c,cmd,saved,*pass,pc,x,y,z))return true;
  return fail(29);
 };
 if(!current || GetTickCount64()-contextAt>3000)stop(0,false);
 if(state.phase==p::SrPhase::Retire)return fail(state.error);
 if(!colour||!depth||!packed||!exposure||!pass||!view||!out||!saved.pipeline||!saved.compute_layout||saved.dynamic_offsets||
  !root_push_supported(saved.cp)||!root_push_supported(saved.gp)||!read_upload(*pass,sizeof(pc),pc.data())||!read_upload(*view,sizeof(vc),vc.data()))return fail(2);
 for(unsigned i:{36u,37u,44u,45u})if(!std::isfinite(pc[i])||pc[i]<1||pc[i]>7680||pc[i]!=std::floor(pc[i]))return fail(3);
 if(pc[36]<640||pc[37]<360||pc[36]>pc[44]||pc[37]>pc[45]||!camera(pc,vc)||
  !std::isfinite(vc[626])||vc[626]<=0||!std::isfinite(vc[576])||!std::isfinite(vc[577])||!context.worldToMetersMilli)return fail(4);
 auto res=[&](const Slot *s){return reinterpret_cast<ID3D12Resource*>(from_view(cmd->get_device(),s->binding.view).handle);};
 auto *gc=res(colour),*gd=res(depth),*gv=res(packed),*ge=res(exposure),*go=res(out);
 for(auto *r:{gc,gd,gv,ge,go})if(!r||!live_resources.contains(reinterpret_cast<uint64_t>(r)))return fail(5);
 for(auto *r:{gc,gd,gv}){auto d=r->GetDesc();if(d.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D||
  d.Width!=pc[36]||d.Height!=pc[37]||d.DepthOrArraySize!=1||d.SampleDesc.Count!=1)return transient();}
 const auto ev=view_desc(cmd->get_device(),exposure->binding.view);
 if(ev.buffer.offset+16>ge->GetDesc().Width||gc->GetDesc().Format!=DXGI_FORMAT_R11G11B10_FLOAT||
  static_cast<unsigned>(view_desc(cmd->get_device(),packed->binding.view).format)!=11)return fail(7);
 if(live_fixture)return fail(8); // Two SR owners may never replace this pass.
 if(!bind_device(c,cmd))return fail(9);
 if(state.phase==p::SrPhase::Preflight){c.observedWidth=unsigned(pc[36]);c.observedHeight=unsigned(pc[37]);
  c.outputWidth=unsigned(pc[44]);c.outputHeight=unsigned(pc[45]);c.observed=true;return false;}
 if(state.phase==p::SrPhase::ApplySource){
  if(p::sourceAcknowledged(state,context)&&unsigned(pc[36])==state.renderWidth&&unsigned(pc[37])==state.renderHeight&&
   unsigned(pc[44])==state.outputWidth&&unsigned(pc[45])==state.outputHeight){
   c.width=unsigned(pc[36]);c.height=unsigned(pc[37]);phase(p::SrPhase::Create);c.started=GetTickCount64();
  }return false;
 }
 if(state.phase!=p::SrPhase::Evaluate&&state.phase!=p::SrPhase::Active)return false;
 if(c.width!=pc[36]||c.height!=pc[37]||c.outputWidth!=pc[44]||c.outputHeight!=pc[45]){
  stop(10,false);if(c.history_dirty && native_reset(c,cmd,saved,*pass,pc,x,y,z))return true;return c.history_dirty;
 }
 LARGE_INTEGER now{};if(!QueryPerformanceCounter(&now)||!c.frequency.QuadPart)return fail(11);
 uint32_t stamp=0;memcpy(&stamp,vc.data()+661,4);bool first=!c.previous.QuadPart;
 float delta=first?0.f:float(1000.*double(now.QuadPart-c.previous.QuadPart)/double(c.frequency.QuadPart));
 c.gap=!first&&(stamp!=uint32_t(c.previousId+1)||delta>250.f);c.previous=now;c.previousId=stamp;
 if(first)return false;
 if(!std::isfinite(delta)||delta<=0||delta>250)return transient();
 auto *proxy=validated_game_proxy(cmd);if(!proxy)return fail(13);
 ID3D12Device *device=nullptr;bool matched=SUCCEEDED(proxy->GetDevice(IID_PPV_ARGS(&device)))&&device==c.proxy_device;
 if(device)device->Release();if(!matched){proxy->Release();return fail(14);}
 SrGuideInput input{gc,gd,gv,ge,go,*pass,*view,ev,static_cast<unsigned>(view_desc(cmd->get_device(),depth->binding.view).format)};
 const auto *error=record_sr_guides(c,c.borrows,cmd,proxy,saved,input);if(error){proxy->Release();return fail(15);}
 auto *native=reinterpret_cast<ID3D12GraphicsCommandList*>(cmd->get_native());uint32_t nativeReset=0;memcpy(&nativeReset,pc.data()+12,4);
 MCD2FsrDispatchV1 parameters{sizeof(parameters),c.width,c.height,c.outputWidth,c.outputHeight,
  state.evaluations==0||nativeReset||c.gap||c.force_reset?1u:0u,vc[576]*c.width*.5f,-vc[577]*c.height*.5f,1,1,delta,vc[626],
  vc[142],std::numeric_limits<float>::infinity(),2.f*std::atan(1.f/vc[133]),1000.f/context.worldToMetersMilli};
 if(c.readable)eval_transition(native,c.output,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
 internal_evaluation=true;int result=c.dispatch(c.session,proxy,gc,c.current_depth,c.motion,c.exposure,c.output,&parameters);internal_evaluation=false;proxy->Release();
 D3D12_RESOURCE_BARRIER order{};order.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;order.UAV.pResource=c.output;native->ResourceBarrier(1,&order);
 eval_transition(native,c.output,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);c.readable=true;
 restore_root(cmd,saved,false,true);restore_root(cmd,saved,true,true);cmd->bind_pipeline(a::pipeline_stage::all_compute,{saved.pipeline});commands[cmd]=saved;
 if(result)return fail(16);
 MCD2FsrInfoV1 info{};info.size=sizeof(info);if(c.info(c.session,&info)||info.errors)return fail(28);
 if(!copy_output(c,native,go))return fail(17);
 c.force_reset=false;
 if(state.evaluations<0x7fffffffu)++state.evaluations;phase(p::SrPhase::Active);return true;
}
static int load(Generation& c){
 const auto folder=asset_root/L"fsr";
 if(!hash_matches(folder/L"mcd2-fsr-game-bridge.dll",MCD2_FSR_BRIDGE_SHA256)||
  !hash_matches(folder/L"amd_fidelityfx_upscaler_dx12.dll","d0dcccc74a43c44ba435b7a369b456e0970d8a4464e4bd683119b374f2c9fb46"))return -1;
 c.bridge=LoadLibraryExW((folder/L"mcd2-fsr-game-bridge.dll").c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32);
 if(!c.bridge)return -2;
 c.create=reinterpret_cast<decltype(c.create)>(GetProcAddress(c.bridge,"mcd2_fsr_create_v1"));
 c.dispatch=reinterpret_cast<decltype(c.dispatch)>(GetProcAddress(c.bridge,"mcd2_fsr_dispatch_v1"));
 c.quality=reinterpret_cast<decltype(c.quality)>(GetProcAddress(c.bridge,"mcd2_fsr_quality_v1"));
 c.destroy=reinterpret_cast<decltype(c.destroy)>(GetProcAddress(c.bridge,"mcd2_fsr_destroy_v1"));
 c.info=reinterpret_cast<decltype(c.info)>(GetProcAddress(c.bridge,"mcd2_fsr_info_v1"));
 return c.create&&c.dispatch&&c.quality&&c.destroy&&c.info?0:-3;
}
static void present(a::command_queue *q,std::unique_lock<std::recursive_mutex>& guard){
 auto module=GetModuleHandleW(L"mcd2-display-latency.addon64");
 auto get=module?reinterpret_cast<decltype(&mcd2_menu_snapshot_v1)>(GetProcAddress(module,"mcd2_menu_snapshot_v1")):nullptr;
 auto getContext=module?reinterpret_cast<decltype(&mcd2_sr_context_v1)>(GetProcAddress(module,"mcd2_sr_context_v1")):nullptr;
 MCD2MenuSnapshotV1 menu{};menu.size=sizeof(menu);p::DecodedGraphicsRecord record;
 const auto now=GetTickCount64();const int menuResult=get?get(&menu):-3;
 const bool useCachedMenu=menuResult==-2 && cachedMenu.size==sizeof(menu) && now-menuReadAt<=1000;
 if(useCachedMenu)menu=cachedMenu;
 bool valid=(menuResult==0||useCachedMenu)&&menu.reserved==0&&menu.session!=0&&p::decodeGraphicsRecord(menu.record,record);
 if(valid&&!useCachedMenu){cachedMenu=menu;menuReadAt=now;}
 else if(!valid&&menuResult!=-2)cachedMenu={};
 MCD2SrContextSnapshotV1 packet{};packet.size=sizeof(packet);p::SrContext fresh;
 int contextResult=getContext?getContext(&packet):-3;
 if(contextResult==-2 && cachedContext.size==sizeof(packet) && now-cachedContext.observedAtMs<=3000){packet=cachedContext;contextResult=0;}
 contextAvailable=valid&&contextResult==0&&packet.reserved==0&&packet.authority.session==menu.session&&
  p::decodeSrContext(packet.context,fresh)&&fresh.session==menu.session&&fresh.intent==record.bootstrap.stamp&&
  GetTickCount64()-packet.observedAtMs<=3000;
 if(contextAvailable)cachedContext=packet;else if(contextResult!=-2)cachedContext={};
 if(contextAvailable){context=fresh;contextAt=packet.observedAtMs;}
 current=contextAvailable&&context.ready;
 requested=valid&&record.intent.sr==p::SrProvider::AmdFsr;
 if(!generation){
  if(!valid)return;
  if(state.session!=menu.session||state.intent!=record.bootstrap.stamp){state={};state.session=menu.session;
   if(requestSequence>=0x7fffffffu)return;state.sequence=++requestSequence;
   state.intent=record.bootstrap.stamp;state.world=contextAvailable?context.world:1;state.provider=record.intent.sr;state.quality=record.intent.srPreferences[1].quality;
   if(restoring)state.phase=p::SrPhase::RestoreSource;}
  if(restoring){
   if(contextAvailable){state.world=context.world;if(p::sourceAcknowledged(state,context)){restoring=false;phase(p::SrPhase::Fallback,state.error);}}
   return;
  }
  if(!requested||!current||state.intent==rejected||strlen(MCD2_FSR_BRIDGE_SHA256)!=64)return;
  // Preserve the previous provider until its Native reset and GPU retirement.
  if(live_fixture){lean.wanted=false;lean.rebuild=false;if(lean.history_dirty||lean.pending||!lean.borrow.recordings.empty()||lean.borrow.blocked)return;
   if(!lean_cleanup(q,guard))return;}
  generation=std::make_unique<Generation>();generation->queue=q;generation->started=GetTickCount64();
  state.world=context.world;state.evaluations=0;phase(p::SrPhase::Preflight);return;
 }
 auto &c=*generation;if(c.busy)return;
 if(c.queue!=q){c.borrows.blocked=true;stop(18);return;}
 if(!valid||!requested||!current||state.intent!=record.bootstrap.stamp||state.session!=menu.session||state.world!=context.world)stop(0,false);
 auto &cache=c.borrows;const auto pending=cache.reset_pending;
 for(auto *cmd:pending){uint64_t epoch=0;if(lean_reset_epoch(cmd,epoch))cache.confirm_reset(cmd,epoch);}
 auto *queue=reinterpret_cast<ID3D12CommandQueue*>(q->get_native());
 if(cache.fence&&!cache.blocked){bool signal=false;for(const auto &[key,entry]:cache.entries)signal|=entry.awaiting_signal;
  if(signal){auto value=cache.next_fence+1;if(FAILED(queue->Signal(cache.fence,value))){cache.blocked=true;stop(19);}
   else{cache.next_fence=value;for(auto &[key,entry]:cache.entries)if(entry.awaiting_signal){entry.awaiting_signal=false;entry.retirement_fence=value;}}}
  auto done=cache.fence->GetCompletedValue();if(done==UINT64_MAX){cache.blocked=true;stop(20);}else{cache.completed_fence=done;cache.retire_ready(done,frame);}
 }
 auto released=std::move(cache.ready_to_release);cache.ready_to_release.clear();c.busy=true;guard.unlock();
 for(auto &entry:released)LeanBorrowCache::release(entry);guard.lock();c.busy=false;
 if(state.phase==p::SrPhase::Retire){
  if(c.history_dirty||!cache.recordings.empty()||!cache.entries.empty()||cache.blocked)return;
  c.busy=true;guard.unlock();int result=c.session?c.destroy(c.session,cache.fence,cache.next_fence):0;guard.lock();c.busy=false;
  if(result){cache.blocked=true;phase(p::SrPhase::Failed,21);return;}c.session=nullptr;
  auto retired=std::move(generation);guard.unlock();retired->borrows.clear_after_idle();
  for(auto it=retired->owned.resources.rbegin();it!=retired->owned.resources.rend();++it)if(*it)(*it)->Release();
  if(retired->bridge)FreeLibrary(retired->bridge);retired.reset();guard.lock();
  state.sourceScaleMicro=100000000;if(contextAvailable)state.world=context.world;restoring=true;phase(p::SrPhase::RestoreSource,state.error);return;
 }
 if((state.phase==p::SrPhase::Preflight||state.phase==p::SrPhase::ApplySource||state.phase==p::SrPhase::Create||state.phase==p::SrPhase::Evaluate)&&
    GetTickCount64()-c.started>15000){stop(22);return;}
 if(state.phase==p::SrPhase::Preflight&&c.observed){
  MCD2FsrQualityV1 quality{sizeof(quality),unsigned(state.quality),c.outputWidth,c.outputHeight};
  c.busy=true;guard.unlock();int result=load(c);
  if(!result){internal_evaluation=true;result=c.create(c.proxy_device,(asset_root/L"fsr"/L"amd_fidelityfx_upscaler_dx12.dll").c_str(),
    c.observedWidth,c.observedHeight,c.outputWidth,c.outputHeight,&c.session);internal_evaluation=false;}
  if(!result&&state.quality!=p::Quality::Custom)result=c.quality(c.session,&quality);
  if(!result){result=c.destroy(c.session,nullptr,0);if(!result)c.session=nullptr;}
  guard.lock();c.busy=false;if(result){stop(23);return;}
  unsigned scale=100000000;
  if(state.quality==p::Quality::Custom){scale=record.intent.srPreferences[1].customScaleBasisPoints*10000;
   quality.renderWidth=unsigned(uint64_t(c.outputWidth)*scale/100000000);quality.renderHeight=unsigned(uint64_t(c.outputHeight)*scale/100000000);}
  else{if(!std::isfinite(quality.upscaleRatio)||quality.upscaleRatio<1){stop(24);return;}scale=unsigned(std::llround(100000000./quality.upscaleRatio));}
  state.renderWidth=quality.renderWidth;state.renderHeight=quality.renderHeight;state.outputWidth=c.outputWidth;state.outputHeight=c.outputHeight;
  state.sourceScaleMicro=scale;state.capabilities=1;
  if(!p::validRuntimeState(p::SrRuntimeState{state.session,state.sequence,state.intent,state.world,p::SrPhase::ApplySource,0,
   state.renderWidth,state.renderHeight,state.outputWidth,state.outputHeight,scale,0,state.provider,state.quality,state.capabilities})){stop(25);return;}
  phase(p::SrPhase::ApplySource);c.started=GetTickCount64();return;
 }
 if(state.phase==p::SrPhase::Create){
  c.busy=true;guard.unlock();internal_evaluation=true;
  int result=c.create(c.proxy_device,(asset_root/L"fsr"/L"amd_fidelityfx_upscaler_dx12.dll").c_str(),c.width,c.height,c.outputWidth,c.outputHeight,&c.session);
  internal_evaluation=false;
  if(!result){c.motion=eval_texture(c.resource_device,c.owned,c.width,c.height,DXGI_FORMAT_R16G16_FLOAT,true);
   c.exposure=eval_texture(c.resource_device,c.owned,1,1,DXGI_FORMAT_R32_FLOAT,true);
   c.output=eval_texture(c.resource_device,c.owned,c.outputWidth,c.outputHeight,DXGI_FORMAT_R16G16B16A16_FLOAT,true);
   if(!c.motion||!c.exposure||!c.output||!QueryPerformanceFrequency(&c.frequency)||c.frequency.QuadPart<=0)result=-1;}
  guard.lock();c.busy=false;if(result){stop(26);return;}
  phase(p::SrPhase::Evaluate);c.started=GetTickCount64();
 }
}
} // namespace fsr_runtime
static bool fsr_runtime_record(a::command_list *cmd,uint32_t x,uint32_t y,uint32_t z){return fsr_runtime::record(cmd,x,y,z);}
static void fsr_runtime_present(a::command_queue *q,std::unique_lock<std::recursive_mutex>& guard){fsr_runtime::present(q,guard);}
static bool fsr_runtime_owns_source(){return fsr_runtime::generation||fsr_runtime::requested||fsr_runtime::state.phase==mcd2::providers::SrPhase::RestoreSource;}
static void fsr_runtime_begin(a::command_list *cmd){if(fsr_runtime::generation)fsr_runtime::generation->borrows.begin_recording(cmd);}
static void fsr_runtime_reset(a::command_list *cmd){if(!fsr_runtime::generation)return;auto &cache=fsr_runtime::generation->borrows;
 if(cache.recordings.contains(cmd)){uint64_t epoch=0;bool known=lean_reset_epoch(cmd,epoch,true);cache.note_reset(cmd,known,epoch);}}
static void fsr_runtime_forget(a::command_list *cmd){if(fsr_runtime::generation){auto &cache=fsr_runtime::generation->borrows;
 cache.reset_pending.erase(cmd);cache.reset_epochs.erase(cmd);cache.forget(cmd);}}
static void fsr_runtime_submit(a::command_queue *q,a::command_list *cmd){if(fsr_runtime::generation&&fsr_runtime::generation->borrows.recordings.contains(cmd)&&fsr_runtime::generation->queue!=q){fsr_runtime::generation->borrows.blocked=true;fsr_runtime::stop(18);}}
static void fsr_runtime_destroy_queue(a::command_queue *q){if(fsr_runtime::generation&&fsr_runtime::generation->queue==q){
 fsr_runtime::generation->borrows.blocked=true;fsr_runtime::generation->queue=nullptr;fsr_runtime::stop(27);}}
extern "C" __declspec(dllexport) int mcd2_sr_runtime_v1(MCD2SrRuntimeSnapshotV1* out){
 if(!out||out->size!=sizeof(*out))return -1;std::unique_lock guard(lock,std::try_to_lock);if(!guard.owns_lock())return -2;
 mcd2::providers::SrRuntimeBytes bytes;if(!mcd2::providers::encodeSrRuntimeState(fsr_runtime::state,bytes))return -3;
 out->reserved=0;std::copy(bytes.begin(),bytes.end(),out->state);return 0;
}
