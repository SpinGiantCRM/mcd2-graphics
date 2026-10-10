// Private, bounded FG experiment. Disabled by default; own UI-alpha output only.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <reshade.hpp>
#include <array>
#include <map>
#include <set>
#include <mutex>
#include <memory>
#include <fstream>
#include <filesystem>
#include <cstring>
#include <cmath>
#include <atomic>
#include "fg_camera_contract.h"
#include "fg_bridge_contract.h"
#include "../providers/fsr_fg_game_bridge.h"
#include "../../src/latency/process_exit.hpp"
namespace a=reshade::api;
#include "fg_alpha_copy.h"
#include "fg_controls.hpp"
namespace {
std::recursive_mutex mutex;
std::ofstream privateLog;
unsigned frame=0,samples=0;
std::filesystem::path capturePolicy;
struct Binding {a::descriptor_type type{};a::resource_view view{};a::buffer_range cb{};};
struct Root {uint64_t layout=0;std::map<unsigned,uint64_t> tables;std::map<unsigned,std::map<unsigned,Binding>> pushed;std::map<unsigned,std::map<unsigned,uint32_t>> constants;bool dynamicOffsets=false;};
struct Cmd {unsigned ps=0,cs=0;uint64_t graphicsPipeline=0,computePipeline=0;Root graphics,compute;bool backbuffer=false,usesAlpha=false;uint32_t imageFrame=UINT32_MAX;std::map<uint64_t,a::resource_usage> states;};
struct Pipe {unsigned ps=0,cs=0;};
std::map<uint64_t,Pipe> pipelines;
std::map<uint64_t,std::vector<std::vector<a::descriptor_range>>> layouts;
std::map<std::pair<uint64_t,unsigned>,Binding> descriptors;
std::map<a::command_list*,Cmd> commands;
std::map<uint64_t,a::resource_usage> initial,submitted;
std::set<uint64_t> backbuffers;
using Frame=uint64_t(*)();
using Visit=int(*)(uint64_t,int(*)(void*,unsigned,void*),void*);
Frame renderFrame=nullptr,presentFrame=nullptr,renderMarker=nullptr,threadRenderMarker=nullptr;Visit visitFrame=nullptr;
struct FrameData{MCD2FGCamera camera{};bool guides=false,images=false;};
std::map<uint32_t,FrameData> gameFrames;
unsigned imageSamples=0,fgTrialFrames=0;std::atomic<unsigned> activeFGMode{0};uint32_t presentedFrame=UINT32_MAX;
bool releasedWhileOff=false;
// Keep one SDK status query per host present. Diagnostic consumers read this
// receipt instead of querying the SDK's presentation counter a second time.
uint64_t outcomeSample=0;
int outcomeResult=-1;
unsigned outcomeStatus=0,outcomePresents=0;
std::atomic<unsigned> guideSamples{0};unsigned trialRevision=0;
unsigned outputWidth=0,outputHeight=0,outputBuffers=0,outputFormat=0;HWND outputWindow=nullptr;
thread_local bool internalGPU=false;
bool retiring=false,ownerClosing=false;
std::atomic<bool> captureEnabled{false},guideTags{false},imageTags{false},legacyEnable{false};unsigned cachedRevision=0;
bool amd_owner(){auto bootstrap=GetModuleHandleW(L"dxgi.dll");auto owner=bootstrap?reinterpret_cast<unsigned(*)()>(GetProcAddress(bootstrap,"mcd2_bootstrap_amd_fg_session")):nullptr;return owner&&owner()==1;}
HMODULE amd_bridge(){return amd_owner()?GetModuleHandleW(L"mcd2-fsr-fg-game-bridge.dll"):nullptr;}
ID3D12GraphicsCommandList* native_command(void* proxy){
 constexpr GUID unwrap={0x7f2c9a11,0x3b4e,0x4d6a,{0x81,0x2f,0x5e,0x9c,0xd3,0x7a,0x1b,0x42}};
 ID3D12GraphicsCommandList* out=nullptr;
 return proxy&&SUCCEEDED(static_cast<ID3D12GraphicsCommandList*>(proxy)->QueryInterface(unwrap,reinterpret_cast<void**>(&out)))?out:nullptr;
}
void poll_policy(){static ULONGLONG next=0;auto now=GetTickCount64();if(now<next||capturePolicy.empty())return;next=now+200;
 captureEnabled=GetPrivateProfileIntW(L"Capture",L"Enabled",0,capturePolicy.c_str())!=0;
 guideTags=GetPrivateProfileIntW(L"Capture",L"GuideTags",0,capturePolicy.c_str())!=0;
 imageTags=GetPrivateProfileIntW(L"Capture",L"ImageTags",0,capturePolicy.c_str())!=0;
 legacyEnable=GetPrivateProfileIntW(L"Capture",L"EnableFG",0,capturePolicy.c_str())!=0;
 cachedRevision=GetPrivateProfileIntW(L"Capture",L"TrialRevision",0,capturePolicy.c_str());
}
bool tracking(){return captureEnabled || (fg_controls::enabled?fg_controls::wanted.load():guideTags.load()&&guideSamples<900);}

void try_retirement(){if(mcd2::process_exit::terminating())return;
 if(auto module=amd_bridge()){
  bool close=false;{std::lock_guard lock(mutex);close=retiring;}if(!close)return;
  using Retire=int(*)(void*,uint64_t);auto retire=reinterpret_cast<Retire>(GetProcAddress(module,"mcd2_afg_retire_v1"));
  const int result=retire?retire(nullptr,0):-1;
  std::lock_guard lock(mutex);privateLog<<"{\"kind\":\"amd_retirement\",\"SDKResult\":"<<result<<"}\n";privateLog.flush();if(!result)retiring=false;return;
 }
 bool ready=false;unsigned recordings=0;
 {std::lock_guard lock(mutex);for(auto&[cmd,state]:commands)recordings+=state.usesAlpha;ready=retiring&&!recordings;}
 if(!ready)return;
 const bool complete=fg_alpha::cleanup();
 std::lock_guard lock(mutex);privateLog<<"{\"kind\":\"alpha_retirement\",\"recordedCommandLeases\":"<<recordings<<",\"GPUCompletionVerified\":"<<(complete?"true":"false")<<"}\n";privateLog.flush();
 if(complete)retiring=false;
}
int token_identity(void*,unsigned index,void* out){*static_cast<unsigned*>(out)=index;return 0;}
void identity_receipt(const char* stage){
 auto module=GetModuleHandleW(L"mcd2-display-latency.addon64");
 if(module&&!renderFrame){renderFrame=reinterpret_cast<Frame>(GetProcAddress(module,"mcd2_fg_render_frame"));presentFrame=reinterpret_cast<Frame>(GetProcAddress(module,"mcd2_fg_present_frame"));visitFrame=reinterpret_cast<Visit>(GetProcAddress(module,"mcd2_fg_visit_frame"));renderMarker=reinterpret_cast<Frame>(GetProcAddress(module,"mcd2_fg_render_marker"));threadRenderMarker=reinterpret_cast<Frame>(GetProcAddress(module,"mcd2_fg_thread_render_marker"));}
 auto id=renderFrame?renderFrame():UINT64_MAX;unsigned index=0;auto result=visitFrame&&id!=UINT64_MAX?visitFrame(id,token_identity,&index):-1;
 privateLog<<"{\"kind\":\"frame_identity\",\"stage\":\""<<stage<<"\",\"frame\":"<<frame<<",\"thread\":"<<GetCurrentThreadId()<<",\"renderIdentity\":"<<id<<",\"presentIdentity\":"<<(presentFrame?presentFrame():UINT64_MAX)<<",\"latestRenderMarker\":"<<(renderMarker?renderMarker():UINT64_MAX)<<",\"threadRenderMarker\":"<<(threadRenderMarker?threadRenderMarker():UINT64_MAX)<<",\"tokenResult\":"<<result<<",\"SDKIndex\":"<<index<<"}\n";
}
bool capturing(){return privateLog.is_open()&&samples<120&&frame%240==0&&captureEnabled;}
void open(){
 if(privateLog.is_open())return;
 auto local=std::make_unique<wchar_t[]>(32768);auto n=GetEnvironmentVariableW(L"LOCALAPPDATA",local.get(),32768);
 if(!n||n>=32768)return;
 auto folder=std::filesystem::path(local.get())/L"Dungeons2"/L"Saved"/L"MCD2Graphics";
 auto exe=std::make_unique<wchar_t[]>(32768);auto length=GetModuleFileNameW(nullptr,exe.get(),32768);if(!length||length>=32768)return;
 capturePolicy=std::filesystem::path(exe.get()).parent_path()/L"FGGuideCapture.ini";
 fg_controls::init(folder);std::filesystem::create_directories(folder);privateLog.open(folder/L"FGGuideRecon-private.jsonl",std::ios::trunc);
 privateLog<<"{\"kind\":\"observer\",\"gameResourcesReadOnly\":true,\"FGPolicyOptIn\":true,\"maximumSamples\":120}\n";privateLog.flush();
}
unsigned crc(const void*data,size_t n){
 static auto table=[](){std::array<unsigned,256> t{};for(unsigned i=0;i<256;++i){auto v=i;for(unsigned k=0;k<8;++k)v=(v>>1)^(0xedb88320u&(0u-(v&1)));t[i]=v;}return t;}();
 auto c=~0u;auto p=static_cast<const unsigned char*>(data);for(size_t i=0;i<n;++i)c=(c>>8)^table[(c^p[i])&255];return ~c;
}
bool compute(a::shader_stage stage){return unsigned(stage&a::shader_stage::compute)!=0;}
void layout(Root& root,uint64_t next){if(root.layout!=next){root.tables.clear();root.pushed.clear();root.constants.clear();root.dynamicOffsets=false;root.layout=next;}}
Binding unpack(const a::descriptor_table_update& update,unsigned i){
 Binding out;out.type=update.type;
 if(update.type==a::descriptor_type::constant_buffer)out.cb=static_cast<const a::buffer_range*>(update.descriptors)[i];
 else if(update.type==a::descriptor_type::shader_resource_view||update.type==a::descriptor_type::buffer_shader_resource_view||update.type==a::descriptor_type::unordered_access_view||update.type==a::descriptor_type::buffer_unordered_access_view)out.view=static_cast<const a::resource_view*>(update.descriptors)[i];
 return out;
}
std::pair<uint64_t,unsigned> key(a::device*device,a::descriptor_table table,unsigned binding){a::descriptor_heap heap{};unsigned offset=0;device->get_descriptor_heap_offset(table,binding,0,&heap,&offset);return {heap.handle,offset};}
void init_device(a::device*){mcd2::process_exit::initialize();if(mcd2::process_exit::terminating())return;std::lock_guard lock(mutex);open();}
void init_pipeline(a::device*,a::pipeline_layout,unsigned count,const a::pipeline_subobject* sub,a::pipeline p){if(mcd2::process_exit::terminating())return;
 Pipe pipe;for(unsigned i=0;i<count;++i)if(sub[i].type==a::pipeline_subobject_type::pixel_shader||sub[i].type==a::pipeline_subobject_type::compute_shader){auto& shader=*static_cast<const a::shader_desc*>(sub[i].data);auto hash=crc(shader.code,shader.code_size);if(sub[i].type==a::pipeline_subobject_type::pixel_shader)pipe.ps=hash;else pipe.cs=hash;}
 std::lock_guard lock(mutex);pipelines[p.handle]=pipe;
}
void destroy_pipeline(a::device*,a::pipeline p){if(mcd2::process_exit::terminating())return;std::lock_guard lock(mutex);pipelines.erase(p.handle);}
void bind_pipeline(a::command_list* cmd,a::pipeline_stage stages,a::pipeline p){if(mcd2::process_exit::terminating())return;if(!tracking())return;std::lock_guard lock(mutex);auto it=pipelines.find(p.handle);auto& state=commands[cmd];if(unsigned(stages&a::pipeline_stage::pixel_shader)){state.graphicsPipeline=p.handle;state.ps=it==pipelines.end()?0:it->second.ps;}if(unsigned(stages&a::pipeline_stage::compute_shader)){state.computePipeline=p.handle;state.cs=it==pipelines.end()?0:it->second.cs;}}
void init_layout(a::device*,unsigned count,const a::pipeline_layout_param* param,a::pipeline_layout handle){if(mcd2::process_exit::terminating())return;
 std::lock_guard lock(mutex);auto& out=layouts[handle.handle];out.resize(count);
 for(unsigned i=0;i<count;++i){auto& p=param[i];if(p.type==a::pipeline_layout_param_type::push_descriptors)out[i].push_back(p.push_descriptors);
 else if(p.type==a::pipeline_layout_param_type::descriptor_table||p.type==a::pipeline_layout_param_type::push_descriptors_with_ranges)for(unsigned j=0;j<p.descriptor_table.count;++j)out[i].push_back(p.descriptor_table.ranges[j]);
 else if(p.type==a::pipeline_layout_param_type::descriptor_table_with_flags||p.type==a::pipeline_layout_param_type::push_descriptors_with_ranges_and_flags)for(unsigned j=0;j<p.descriptor_table_with_flags.count;++j)out[i].push_back(p.descriptor_table_with_flags.ranges[j]);}
}
void destroy_layout(a::device*,a::pipeline_layout handle){if(mcd2::process_exit::terminating())return;std::lock_guard lock(mutex);layouts.erase(handle.handle);}
void push(a::command_list*cmd,a::shader_stage stage,a::pipeline_layout handle,unsigned param,const a::descriptor_table_update& update){if(mcd2::process_exit::terminating())return;if(!tracking())return;std::lock_guard lock(mutex);auto& root=compute(stage)?commands[cmd].compute:commands[cmd].graphics;layout(root,handle.handle);root.tables.erase(param);for(unsigned i=0;i<update.count;++i)root.pushed[param][update.binding+i]=unpack(update,i);}
void tables(a::command_list*cmd,a::shader_stage stage,a::pipeline_layout handle,unsigned first,unsigned count,const a::descriptor_table* table,unsigned dynamicCount,const unsigned*){if(mcd2::process_exit::terminating())return;if(!tracking())return;std::lock_guard lock(mutex);auto& root=compute(stage)?commands[cmd].compute:commands[cmd].graphics;layout(root,handle.handle);root.dynamicOffsets|=dynamicCount!=0;for(unsigned i=0;i<count;++i){root.tables[first+i]=table[i].handle;root.pushed.erase(first+i);}}
void constants(a::command_list*cmd,a::shader_stage stage,a::pipeline_layout handle,unsigned param,unsigned first,unsigned count,const void*data){if(mcd2::process_exit::terminating())return;if(!tracking())return;std::lock_guard lock(mutex);auto&root=compute(stage)?commands[cmd].compute:commands[cmd].graphics;layout(root,handle.handle);auto values=static_cast<const uint32_t*>(data);for(unsigned i=0;i<count;++i)root.constants[param][first+i]=values[i];}
bool update(a::device*device,unsigned count,const a::descriptor_table_update* updates){if(mcd2::process_exit::terminating())return false;std::lock_guard lock(mutex);for(unsigned j=0;j<count;++j)for(unsigned i=0;i<updates[j].count;++i)descriptors[key(device,updates[j].table,updates[j].binding+i)]=unpack(updates[j],i);return false;}
bool copy(a::device*device,unsigned count,const a::descriptor_table_copy* copies){if(mcd2::process_exit::terminating())return false;std::lock_guard lock(mutex);for(unsigned j=0;j<count;++j){auto& c=copies[j];std::vector<Binding> snapshot(c.count);for(unsigned i=0;i<c.count;++i){auto it=descriptors.find(key(device,c.source_table,c.source_binding+i));if(it!=descriptors.end())snapshot[i]=it->second;}for(unsigned i=0;i<c.count;++i)descriptors[key(device,c.dest_table,c.dest_binding+i)]=snapshot[i];}return false;}
void resource_init(a::device*,const a::resource_desc&,const a::subresource_data*,a::resource_usage state,a::resource resource){if(mcd2::process_exit::terminating())return;std::lock_guard lock(mutex);initial[resource.handle]=state;}
void resource_destroy(a::device*,a::resource resource){if(mcd2::process_exit::terminating())return;std::lock_guard lock(mutex);initial.erase(resource.handle);submitted.erase(resource.handle);for(auto& [cmd,state]:commands)state.states.erase(resource.handle);}
void barrier(a::command_list*cmd,unsigned count,const a::resource*resources,const a::resource_usage*,const a::resource_usage*after){if(mcd2::process_exit::terminating())return;if(!captureEnabled)return;std::lock_guard lock(mutex);for(unsigned i=0;i<count;++i)commands[cmd].states[resources[i].handle]=after[i];}
void execute(a::command_queue*q,a::command_list*cmd){if(mcd2::process_exit::terminating())return;bool tagged=false;{std::lock_guard lock(mutex);if(captureEnabled)for(auto& [resource,state]:commands[cmd].states)submitted[resource]=state;tagged=commands[cmd].imageFrame!=UINT32_MAX;}if(tagged)fg_alpha::submission_queue(q);}
void reset(a::command_list*cmd){if(mcd2::process_exit::terminating())return;{std::lock_guard lock(mutex);commands.erase(cmd);}try_retirement();}
void swap_init(a::swapchain*swap,bool resize){if(mcd2::process_exit::terminating())return;std::lock_guard lock(mutex);open();for(unsigned i=0;i<swap->get_back_buffer_count();++i)backbuffers.insert(swap->get_back_buffer(i).handle);auto d=swap->get_device()->get_resource_desc(swap->get_current_back_buffer());outputWidth=d.texture.width;outputHeight=d.texture.height;outputFormat=unsigned(d.texture.format);outputBuffers=swap->get_back_buffer_count();outputWindow=static_cast<HWND>(swap->get_hwnd());privateLog<<"{\"kind\":\"swapchain\",\"resize\":"<<(resize?"true":"false")<<",\"width\":"<<d.texture.width<<",\"height\":"<<d.texture.height<<",\"format\":"<<unsigned(d.texture.format)<<",\"buffers\":"<<swap->get_back_buffer_count()<<"}\n";privateLog.flush();}
void swap_destroy(a::swapchain*swap,bool resize){if(mcd2::process_exit::terminating())return;
 bool owner=false;unsigned recordings=0;
 {std::lock_guard lock(mutex);owner=swap->get_hwnd()==outputWindow;for(unsigned i=0;i<swap->get_back_buffer_count();++i)backbuffers.erase(swap->get_back_buffer(i).handle);
  if(owner){retiring=true;ownerClosing=!resize;gameFrames.clear();for(auto&[cmd,state]:commands)recordings+=state.usesAlpha;
   privateLog<<"{\"kind\":\"alpha_retirement_requested\",\"resize\":"<<(resize?"true":"false")<<",\"recordedCommandLeases\":"<<recordings<<"}\n";privateLog.flush();}}
 if(!owner)return;
 if(auto module=amd_bridge()){
  using Present=int(*)(void*,uint64_t,uint32_t);auto disable=reinterpret_cast<Present>(GetProcAddress(module,"mcd2_afg_present_v2"));
  if(disable)disable(nullptr,UINT64_MAX,0);activeFGMode=0;
  // ReShade recreates its wrapper on ResizeBuffers; the SDK presenter remains
  // the same native object. Do not destroy it beneath the game's live COM ref.
  if(resize){std::lock_guard lock(mutex);retiring=false;return;}
  try_retirement();return;
 }
 auto bridge=GetModuleHandleW(L"fg-sdk-bridge.dll");using Configure=int(*)(const MCD2FGConfig*);auto configure=bridge?reinterpret_cast<Configure>(GetProcAddress(bridge,"mcd2_fg_configure")):nullptr;
 if(activeFGMode&&configure){MCD2FGConfig c{sizeof(c),0,outputWidth,outputHeight,outputWidth,outputHeight,outputFormat,61,41,34,outputBuffers,1};if(!configure(&c))activeFGMode=0;}
 // SR owns a feature in the same NGX context. Its queue-retirement callback
 // acknowledges feature release before the shared SDK may be shut down.
 try_retirement();
}
void targets(a::command_list*cmd,unsigned count,const a::resource_view*views,a::resource_view){if(mcd2::process_exit::terminating())return;if(!tracking())return;std::lock_guard lock(mutex);auto& state=commands[cmd];state.backbuffer=false;for(unsigned i=0;i<count;++i)if(backbuffers.count(cmd->get_device()->get_resource_from_view(views[i]).handle))state.backbuffer=true;}
bool pass(a::command_list*cmd,unsigned count,const a::render_pass_render_target_desc*rt,const a::render_pass_depth_stencil_desc*,a::render_pass_flags){if(mcd2::process_exit::terminating())return false;std::vector<a::resource_view> views;for(unsigned i=0;i<count;++i)views.push_back(rt[i].view);targets(cmd,count,views.data(),{});return false;}
void observe(a::command_list*cmd,bool cs){
 if(!capturing())return;auto& state=commands[cmd];auto& root=cs?state.compute:state.graphics;auto found=layouts.find(root.layout);if(found==layouts.end())return;
 identity_receipt(cs?"temporal":"compositor");
 privateLog<<"{\"kind\":\""<<(cs?"temporal_inputs":"compositor_inputs")<<"\",\"frame\":"<<frame<<",\"shader\":"<<(cs?state.cs:state.ps)<<",\"backbufferTarget\":"<<(state.backbuffer?"true":"false")<<",\"bindings\":[";bool first=true;
 for(unsigned param=0;param<found->second.size();++param)for(auto& range:found->second[param]){
  if(range.type==a::descriptor_type::sampler||range.count>=128||range.dx_register_space!=0||!(unsigned(range.visibility&(cs?a::shader_stage::compute:a::shader_stage::pixel))))continue;
  for(unsigned i=0;i<std::min(range.count,64u);++i){Binding b;bool present=false;auto p=root.pushed.find(param);if(p!=root.pushed.end()){auto v=p->second.find(range.binding+i);if(v!=p->second.end()){b=v->second;present=true;}}
   if(!present){auto t=root.tables.find(param);if(t!=root.tables.end()){auto v=descriptors.find(key(cmd->get_device(),{t->second},range.binding+i));if(v!=descriptors.end()){b=v->second;present=true;}}}
   if(!present||b.type==a::descriptor_type::sampler)continue;
   const bool compatible=b.type==range.type||((range.type==a::descriptor_type::shader_resource_view||range.type==a::descriptor_type::buffer_shader_resource_view)&&(b.type==a::descriptor_type::shader_resource_view||b.type==a::descriptor_type::buffer_shader_resource_view))||((range.type==a::descriptor_type::unordered_access_view||range.type==a::descriptor_type::buffer_unordered_access_view)&&(b.type==a::descriptor_type::unordered_access_view||b.type==a::descriptor_type::buffer_unordered_access_view));
   if(!compatible)continue;
   auto resource=b.type==a::descriptor_type::constant_buffer?b.cb.buffer:cmd->get_device()->get_resource_from_view(b.view);if(!resource.handle)continue;auto desc=cmd->get_device()->get_resource_desc(resource);
   if(!first)privateLog<<',';first=false;privateLog<<"{\"slot\":"<<range.dx_register_index+i<<",\"type\":"<<unsigned(b.type)<<",\"id\":"<<resource.handle;
   if(desc.type==a::resource_type::buffer)privateLog<<",\"bytes\":"<<desc.buffer.size;else privateLog<<",\"width\":"<<desc.texture.width<<",\"height\":"<<desc.texture.height<<",\"format\":"<<unsigned(desc.texture.format);
   auto known=state.states.find(resource.handle);auto earlier=submitted.find(resource.handle);auto init=initial.find(resource.handle);if(known!=state.states.end())privateLog<<",\"state\":"<<unsigned(known->second);else if(earlier!=submitted.end())privateLog<<",\"state\":"<<unsigned(earlier->second);else if(init!=initial.end())privateLog<<",\"state\":"<<unsigned(init->second);
   if(cs&&b.type==a::descriptor_type::constant_buffer&&range.dx_register_index+i==1){auto native=reinterpret_cast<ID3D12Resource*>(resource.handle);D3D12_HEAP_PROPERTIES hp{};D3D12_HEAP_FLAGS flags{};if(SUCCEEDED(native->GetHeapProperties(&hp,&flags))&&hp.Type==D3D12_HEAP_TYPE_UPLOAD&&b.cb.offset+2528<=native->GetDesc().Width&&(b.cb.size==UINT64_MAX||b.cb.size>=2528)){void*data=nullptr;D3D12_RANGE read{SIZE_T(b.cb.offset),SIZE_T(b.cb.offset+2528)};if(SUCCEEDED(native->Map(0,&read,&data))){std::array<float,632> values{};std::memcpy(values.data(),static_cast<char*>(data)+b.cb.offset,2528);D3D12_RANGE written{0,0};native->Unmap(0,&written);unsigned finite=0;for(float v:values)finite+=std::isfinite(v);privateLog<<",\"viewFloatCount\":632,\"finiteViewFloats\":"<<finite<<",\"jitter\":["<<values[576]<<','<<values[577]<<"],\"preExposure\":"<<values[626];}}}
   if(cs&&b.type==a::descriptor_type::constant_buffer&&range.dx_register_index+i==1&&samples<24){auto native=reinterpret_cast<ID3D12Resource*>(resource.handle);D3D12_HEAP_PROPERTIES hp{};D3D12_HEAP_FLAGS flags{};if(SUCCEEDED(native->GetHeapProperties(&hp,&flags))&&hp.Type==D3D12_HEAP_TYPE_UPLOAD&&b.cb.offset+2528<=native->GetDesc().Width&&(b.cb.size==UINT64_MAX||b.cb.size>=2528)){void*data=nullptr;D3D12_RANGE read{SIZE_T(b.cb.offset),SIZE_T(b.cb.offset+2528)};if(SUCCEEDED(native->Map(0,&read,&data))){auto values=std::make_unique<std::array<float,632>>();std::memcpy(values->data(),static_cast<char*>(data)+b.cb.offset,2528);D3D12_RANGE written{0,0};native->Unmap(0,&written);privateLog<<",\"cameraMatrixPrefix\":[";for(unsigned v=0;v<288;++v){if(v)privateLog<<',';privateLog<<(*values)[v];}privateLog<<"],\"cameraVectorPrefix\":[";for(unsigned v=288;v<336;++v){if(v!=288)privateLog<<',';privateLog<<(*values)[v];}privateLog<<"],\"clipToPrevious\":[";for(unsigned v=544;v<560;++v){if(v!=544)privateLog<<',';privateLog<<(*values)[v];}privateLog<<']';}}}
   if(cs&&b.type==a::descriptor_type::constant_buffer&&range.dx_register_index+i==1){
    // Search only numerical frame stamps; never dump the full game constant buffer.
    constexpr size_t extent=6044;auto native=reinterpret_cast<ID3D12Resource*>(resource.handle);D3D12_HEAP_PROPERTIES hp{};D3D12_HEAP_FLAGS flags{};
    auto reference=renderFrame?renderFrame():UINT64_MAX;
    if(reference>16&&reference<UINT32_MAX-16&&SUCCEEDED(native->GetHeapProperties(&hp,&flags))&&hp.Type==D3D12_HEAP_TYPE_UPLOAD&&b.cb.offset+extent<=native->GetDesc().Width&&(b.cb.size==UINT64_MAX||b.cb.size>=extent)){
     void*data=nullptr;D3D12_RANGE read{SIZE_T(b.cb.offset),SIZE_T(b.cb.offset+extent)};
     if(SUCCEEDED(native->Map(0,&read,&data))){privateLog<<",\"candidateFrameStamps\":[";bool stampFirst=true;
      for(size_t offset=0;offset+4<=extent;offset+=4){uint32_t value=0;std::memcpy(&value,static_cast<char*>(data)+b.cb.offset+offset,4);if(value>=reference-8&&value<=reference+8){if(!stampFirst)privateLog<<',';stampFirst=false;privateLog<<"["<<offset<<','<<value<<']';}}
      privateLog<<']';D3D12_RANGE written{0,0};native->Unmap(0,&written);
     }
    }
   }
   privateLog<<'}';
  }
 }
 privateLog<<"]}\n";privateLog.flush();++samples;
}
Binding binding(a::command_list*cmd,const Root&root,unsigned slot){
 auto found=layouts.find(root.layout);if(found==layouts.end())return {};
 for(unsigned param=0;param<found->second.size();++param)for(auto&range:found->second[param]){
  if(range.dx_register_space||range.count>=128||range.type!=a::descriptor_type::shader_resource_view||!(unsigned(range.visibility&a::shader_stage::pixel))||slot<range.dx_register_index||slot>=range.dx_register_index+range.count)continue;
  auto index=range.binding+slot-range.dx_register_index;auto p=root.pushed.find(param);if(p!=root.pushed.end()){auto v=p->second.find(index);if(v!=p->second.end()&&v->second.type==range.type)return v->second;}
  auto t=root.tables.find(param);if(t!=root.tables.end()){auto v=descriptors.find(key(cmd->get_device(),{t->second},index));if(v!=descriptors.end()&&v->second.type==range.type)return v->second;}
 }
 return {};
}
void restore_root(a::command_list*cmd,const Root&root,bool cs){
 if(!root.layout)return;auto stages=cs?a::shader_stage::all_compute:a::shader_stage::all_graphics;a::pipeline_layout handle{root.layout};cmd->bind_descriptor_tables(stages,handle,0,0,nullptr);
 for(auto&[param,value]:root.tables){a::descriptor_table table{value};cmd->bind_descriptor_tables(stages,handle,param,1,&table);}
 for(auto&[param,bindings]:root.pushed)for(auto&[index,b]:bindings){a::descriptor_table_update update{};update.binding=index;update.count=1;update.type=b.type;update.descriptors=b.type==a::descriptor_type::constant_buffer?static_cast<const void*>(&b.cb):static_cast<const void*>(&b.view);cmd->push_descriptors(stages,handle,param,update);}
 for(auto&[param,values]:root.constants)for(auto&[index,value]:values)cmd->push_constants(stages,handle,param,index,1,&value);
}
void compositor(a::command_list*cmd){
 if(internalGPU||!imageTags||(fg_controls::enabled&&!fg_controls::wanted))return;Cmd saved;Binding ui,world;uint32_t stamp=UINT32_MAX;unsigned sample=0;
 {
  std::lock_guard lock(mutex);if(retiring||ownerClosing||capturePolicy.empty()||!imageTags||(!fg_controls::enabled&&imageSamples>=900)|| (fg_controls::enabled&&!fg_controls::wanted))return;
  saved=commands[cmd];if(saved.ps!=0x378df900||!saved.backbuffer||saved.graphics.dynamicOffsets||saved.compute.dynamicOffsets||!saved.graphicsPipeline)return;
  auto id=renderMarker?renderMarker():UINT64_MAX;if(id>=UINT32_MAX)return;stamp=uint32_t(id);auto f=gameFrames.find(stamp);if(f==gameFrames.end()||!f->second.guides)return;
  ui=binding(cmd,saved.graphics,0);world=binding(cmd,saved.graphics,1);if(!ui.view.handle||!world.view.handle)return;sample=imageSamples++;
 }
 auto wd=cmd->get_device()->get_resource_desc(cmd->get_device()->get_resource_from_view(world.view));auto ud=cmd->get_device()->get_resource_desc(cmd->get_device()->get_resource_from_view(ui.view));
 if(wd.texture.width!=outputWidth||wd.texture.height!=outputHeight||ud.texture.width!=outputWidth||ud.texture.height!=outputHeight||unsigned(wd.texture.format)!=24)return;
 auto proxy=fg_alpha::proxy(cmd);if(!proxy)return;int tokenResult=-1,sdkResult=-100;unsigned sdkIndex=0;
 auto uiResource=reinterpret_cast<ID3D12Resource*>(cmd->get_device()->get_resource_from_view(ui.view).handle);auto worldResource=reinterpret_cast<ID3D12Resource*>(cmd->get_device()->get_resource_from_view(world.view).handle);
 internalGPU=true;
 if(auto module=amd_bridge()){
  using World=int(*)(void*,void*,uint64_t);auto copy=reinterpret_cast<World>(GetProcAddress(module,"mcd2_afg_world_v2"));auto raw=native_command(proxy);
  sdkResult=raw&&copy?copy(raw,worldResource,stamp):-100;tokenResult=sdkResult;sdkIndex=stamp;if(raw)raw->Release();
  restore_root(cmd,saved.compute,true);if(saved.computePipeline)cmd->bind_pipeline(a::pipeline_stage::all_compute,{saved.computePipeline});restore_root(cmd,saved.graphics,false);cmd->bind_pipeline(a::pipeline_stage::all_graphics,{saved.graphicsPipeline});internalGPU=false;proxy->Release();
  std::lock_guard lock(mutex);commands[cmd]=saved;
  if(!sdkResult){commands[cmd].imageFrame=stamp;gameFrames[stamp].images=true;}
  if(sample<120||sample%60==0)privateLog<<"{\"kind\":\"amd_world_copy\",\"frameStamp\":"<<stamp<<",\"SDKResult\":"<<sdkResult<<"}\n";privateLog.flush();return;
 }
 auto alpha=fg_alpha::copy(proxy,uiResource,DXGI_FORMAT(cmd->get_device()->get_resource_view_desc(ui.view).format),outputWidth,outputHeight);
 if(alpha){
  auto bridge=GetModuleHandleW(L"fg-sdk-bridge.dll");using Images=int(*)(void*,void*,void*,void*,unsigned,unsigned);auto images=bridge?reinterpret_cast<Images>(GetProcAddress(bridge,"mcd2_fg_game_images")):nullptr;
  struct Context{Images images;void*world,*alpha,*cmd;unsigned width,height,index=0;int result=-100;};Context context{images,worldResource,alpha,proxy,outputWidth,outputHeight};
  if(visitFrame&&images)tokenResult=visitFrame(stamp,[](void*token,unsigned index,void*data){auto&c=*static_cast<Context*>(data);c.index=index;c.result=c.images(token,c.world,c.alpha,c.cmd,c.width,c.height);return c.result;},&context);
  sdkResult=context.result;sdkIndex=context.index;
 }
 restore_root(cmd,saved.compute,true);if(saved.computePipeline)cmd->bind_pipeline(a::pipeline_stage::all_compute,{saved.computePipeline});restore_root(cmd,saved.graphics,false);cmd->bind_pipeline(a::pipeline_stage::all_graphics,{saved.graphicsPipeline});internalGPU=false;proxy->Release();
 std::lock_guard lock(mutex);commands[cmd]=saved;commands[cmd].usesAlpha|=alpha!=nullptr;if(!tokenResult&&!sdkResult){commands[cmd].imageFrame=stamp;gameFrames[stamp].images=true;}
 if(!fg_controls::enabled||sample<120||sample%60==0)privateLog<<"{\"kind\":\"game_image_tags\",\"sample\":"<<sample<<",\"frameStamp\":"<<stamp<<",\"SDKIndex\":"<<sdkIndex<<",\"tokenResult\":"<<tokenResult<<",\"SDKResult\":"<<sdkResult<<",\"alphaReady\":"<<(alpha?"true":"false")<<",\"alphaStage\":"<<fg_alpha::stage.load()<<",\"alphaHRESULT\":"<<uint32_t(fg_alpha::lastHR.load())<<",\"UIViewFormat\":"<<unsigned(cmd->get_device()->get_resource_view_desc(ui.view).format)<<"}\n";privateLog.flush();
}
bool dispatch(a::command_list*cmd,unsigned,unsigned,unsigned){if(mcd2::process_exit::terminating())return false;if(internalGPU||!captureEnabled)return false;std::lock_guard lock(mutex);auto cs=commands[cmd].cs;if(cs==0x03645dc9||cs==0xf56e10f9)observe(cmd,true);return false;}
bool draw(a::command_list*cmd,unsigned,unsigned,unsigned,unsigned){if(mcd2::process_exit::terminating())return false;if(!tracking())return false;{std::lock_guard lock(mutex);auto& state=commands[cmd];if(!internalGPU&&(state.backbuffer||state.ps==0x378df900))observe(cmd,false);}compositor(cmd);return false;}
bool indexed(a::command_list*cmd,unsigned,unsigned,unsigned,int,unsigned){if(mcd2::process_exit::terminating())return false;return draw(cmd,0,0,0,0);}
void finish(a::command_queue*,a::swapchain*){if(mcd2::process_exit::terminating())return;
 auto bridge=GetModuleHandleW(L"fg-sdk-bridge.dll");using State=int(*)(unsigned*,unsigned*,unsigned*,unsigned*);auto state=bridge?reinterpret_cast<State>(GetProcAddress(bridge,"mcd2_fg_state")):nullptr;
 if(auto module=amd_bridge()){
  using State=int(*)(MCD2AmdFgStateV1*);auto get=reinterpret_cast<State>(GetProcAddress(module,"mcd2_afg_state_v1"));MCD2AmdFgStateV1 state{};state.size=sizeof(state);auto result=get?get(&state):-1;
  std::lock_guard lock(mutex);++outcomeSample;outcomeResult=result;outcomeStatus=state.fault;outcomePresents=0;
  if(frame<120||frame%60==0)privateLog<<"{\"kind\":\"amd_fg_outcome\",\"SDKResult\":"<<result<<",\"active\":"<<state.active<<",\"fault\":"<<state.fault<<",\"engineFrame\":"<<state.engineFrame<<",\"providerFrame\":"<<state.providerFrame<<",\"prepared\":"<<state.prepared<<",\"images\":"<<state.images<<",\"realPresents\":"<<state.realPresents<<",\"generatedPresents\":"<<state.generatedPresents<<",\"errors\":"<<state.errors<<",\"warnings\":"<<state.warnings<<"}\n";privateLog.flush();++frame;return;
 }
 unsigned max=0,status=0,min=0,presents=0;int result=activeFGMode&&state?state(&max,&status,&min,&presents):-1;
 if(fg_controls::enabled && activeFGMode && (result!=0||status!=0))fg_controls::fault=1;
 std::lock_guard lock(mutex);++outcomeSample;
 outcomeResult=activeFGMode?result:0;outcomeStatus=activeFGMode?status:0;outcomePresents=activeFGMode?presents:1;
 if(activeFGMode && (!fg_controls::enabled||fgTrialFrames<120||fgTrialFrames%60==0)){privateLog<<"{\"kind\":\"fg_present_outcome\",\"frameStamp\":"<<presentedFrame<<",\"SDKResult\":"<<result<<",\"status\":"<<status<<",\"actualPresents\":"<<presents<<"}\n";privateLog.flush();}++frame;
}
void present(a::command_queue*q,a::swapchain*swap,const a::rect*,const a::rect*,unsigned,const a::rect*){if(mcd2::process_exit::terminating())return;
 poll_policy();fg_controls::poll(swap->get_device(),capturePolicy,activeFGMode);
 FrameData data{};bool ready=false,wanted=false;uint64_t id=UINT64_MAX;
 {
  std::lock_guard lock(mutex);if(capturing()){identity_receipt("present");privateLog.flush();}id=presentFrame?presentFrame():UINT64_MAX;
  const unsigned revision=cachedRevision;
  if(revision&&revision!=trialRevision&&!activeFGMode){guideSamples=0;imageSamples=0;fgTrialFrames=0;gameFrames.clear();trialRevision=revision;privateLog<<"{\"kind\":\"trial_revision\",\"revision\":"<<revision<<"}\n";privateLog.flush();}
  if(id<UINT32_MAX){auto f=gameFrames.find(uint32_t(id));if(f!=gameFrames.end()){data=f->second;ready=data.guides&&data.images;}}
  wanted=fg_controls::enabled?fg_controls::wanted.load()&&GetForegroundWindow()==outputWindow:legacyEnable.load()&&fgTrialFrames<600&&GetForegroundWindow()==outputWindow;
 }
 const unsigned mode=ready&&wanted&&fg_alpha::submission_queue(q)?1:0;
 if(auto module=amd_bridge()){
  using Present=int(*)(void*,uint64_t,uint32_t);auto configure=reinterpret_cast<Present>(GetProcAddress(module,"mcd2_afg_present_v2"));auto result=configure?configure(reinterpret_cast<void*>(q->get_native()),id,ready&&wanted?1:0):-1;
  activeFGMode=!result&&ready&&wanted?1:0;
  if(fg_controls::enabled&&result&&result!=MCD2_AFG_BUSY_V2)fg_controls::fault=1;
  if(fg_controls::enabled)fg_controls::publish(activeFGMode,ready);
  if(activeFGMode){++fgTrialFrames;presentedFrame=uint32_t(id);}return;
 }

 auto bridge=GetModuleHandleW(L"fg-sdk-bridge.dll");using LastConfig=int(*)(MCD2FGConfig*);
 auto last=bridge?reinterpret_cast<LastConfig>(GetProcAddress(bridge,"mcd2_fg_last_config")):nullptr;
 MCD2FGConfig effective{};if(last)last(&effective);
 const MCD2FGConfig requested{sizeof(MCD2FGConfig),mode,outputWidth,outputHeight,data.camera.width?data.camera.width:outputWidth,data.camera.height?data.camera.height:outputHeight,outputFormat,61,41,34,outputBuffers,1};
 const auto config=mcd2::fg::configurationForPresent(requested,effective);
 if((mode||activeFGMode) && !mcd2::fg::sameConfiguration(config,effective)){using Configure=int(*)(const MCD2FGConfig*);auto configure=bridge?reinterpret_cast<Configure>(GetProcAddress(bridge,"mcd2_fg_configure")):nullptr;
  auto result=configure?configure(&config):-1;
  if(last)last(&effective);
  std::lock_guard lock(mutex);privateLog<<"{\"kind\":\"game_fg_mode\",\"frameStamp\":"<<id<<",\"requestedMode\":"<<config.mode<<",\"SDKResult\":"<<result<<",\"matchingInputs\":"<<(ready?"true":"false")<<",\"guideWidth\":"<<effective.motionWidth<<",\"guideHeight\":"<<effective.motionHeight<<"}\n";privateLog.flush();if(!result){activeFGMode=config.mode;if(config.mode)releasedWhileOff=false;}else if(fg_controls::enabled)fg_controls::fault=1;
 }
 const bool permanentOff=fg_controls::enabled?mcd2::fg::permanentOff(fg_controls::intent.mode,fg_controls::intent.session==fg_controls::session,fg_controls::fault.load()):(!legacyEnable.load()||fgTrialFrames>=600);
 if(!activeFGMode && permanentOff && !releasedWhileOff){
  auto bridge=GetModuleHandleW(L"fg-sdk-bridge.dll");using Release=int(*)();auto release=bridge?reinterpret_cast<Release>(GetProcAddress(bridge,"mcd2_fg_release")):nullptr;auto configured=bridge?reinterpret_cast<Release>(GetProcAddress(bridge,"mcd2_fg_configured")):nullptr;
  // A full release ends this process's FG session. Re-enabling requires the
  // same restart as changing its swapchain route; temporary menus do not.
  if(fg_controls::enabled && configured && configured()>0)fg_controls::retired=true;
  auto result=release?release():-1;releasedWhileOff=true;
  std::lock_guard lock(mutex);privateLog<<"{\"kind\":\"fg_permanent_release\",\"SDKResult\":"<<result<<"}\n";privateLog.flush();if(result&&fg_controls::enabled)fg_controls::fault=1;
 }
 if(fg_controls::enabled && (fgTrialFrames%30==0 || mode!=activeFGMode))fg_controls::publish(activeFGMode,ready);
 if(activeFGMode){++fgTrialFrames;presentedFrame=uint32_t(id);}
}
}
extern "C" __declspec(dllexport) int mcd2_fg_active(){if(mcd2::process_exit::terminating())return 0;return activeFGMode.load()?1:0;}
extern "C" __declspec(dllexport) int mcd2_fg_last_present(uint64_t* sample,unsigned* status,unsigned* presents){if(mcd2::process_exit::terminating())return -1;
 if(!sample||!status||!presents)return -1;
 std::lock_guard lock(mutex);*sample=outcomeSample;*status=outcomeStatus;*presents=outcomePresents;return outcomeResult;
}
extern "C" __declspec(dllexport) int mcd2_fg_inputs_wanted(){if(mcd2::process_exit::terminating())return 0;return tracking()?1:0;}
extern "C" __declspec(dllexport) void mcd2_fg_observe_sr(void*cmd,void*depth,void*motion,const MCD2FGCamera*camera){if(mcd2::process_exit::terminating())return;
 if(!camera||camera->size!=sizeof(*camera)||!cmd||!depth||!motion)return;
 if(fg_controls::enabled&&!fg_controls::wanted)return;
 {std::lock_guard lock(mutex);if(retiring||ownerClosing||!guideTags)return;}
 const auto sample=guideSamples.fetch_add(1);if((!fg_controls::enabled&&sample>=900)||(fg_controls::enabled&&!fg_controls::wanted))return;
 if(auto module=amd_bridge()){
  auto latency=GetModuleHandleW(L"mcd2-display-latency.addon64");renderMarker=latency?reinterpret_cast<Frame>(GetProcAddress(latency,"mcd2_fg_render_marker")):nullptr;presentFrame=latency?reinterpret_cast<Frame>(GetProcAddress(latency,"mcd2_fg_present_frame")):nullptr;
  using Guides=int(*)(void*,void*,void*,const MCD2AmdFgGuidesV1*);auto prepare=reinterpret_cast<Guides>(GetProcAddress(module,"mcd2_afg_guides_v2"));
  static LARGE_INTEGER frequency{},previous{};static uint32_t previousFrame=UINT32_MAX;
  LARGE_INTEGER now{};if(!frequency.QuadPart)QueryPerformanceFrequency(&frequency);QueryPerformanceCounter(&now);
  if(previousFrame!=UINT32_MAX&&camera->frame<=previousFrame)return;
  MCD2AmdFgGuidesV1 p{};p.size=sizeof(p);p.camera=*camera;p.worldToMeters=.01f;
  p.frameTimeMs=previous.QuadPart&&frequency.QuadPart?float(double(now.QuadPart-previous.QuadPart)*1000./frequency.QuadPart):0.f;previous=now;previousFrame=camera->frame;
  auto raw=native_command(cmd);const auto result=raw&&prepare&&p.frameTimeMs>0?prepare(raw,depth,motion,&p):-101;if(raw)raw->Release();
  std::lock_guard lock(mutex);gameFrames[camera->frame]={*camera,result==0,false};while(gameFrames.size()>16)gameFrames.erase(gameFrames.begin());
  if(sample<120||sample%60==0)privateLog<<"{\"kind\":\"amd_guides\",\"frameStamp\":"<<camera->frame<<",\"SDKResult\":"<<result<<",\"frameTimeMs\":"<<p.frameTimeMs<<",\"width\":"<<camera->width<<",\"height\":"<<camera->height<<",\"cameraBytes\":"<<camera->size<<",\"reset\":"<<camera->reset<<"}\n";privateLog.flush();return;
 }
 auto latency=GetModuleHandleW(L"mcd2-display-latency.addon64"),bridge=GetModuleHandleW(L"fg-sdk-bridge.dll");
 auto visit=latency?reinterpret_cast<Visit>(GetProcAddress(latency,"mcd2_fg_visit_frame")):nullptr;
 {std::lock_guard lock(mutex);if(visit){visitFrame=visit;renderMarker=reinterpret_cast<Frame>(GetProcAddress(latency,"mcd2_fg_render_marker"));presentFrame=reinterpret_cast<Frame>(GetProcAddress(latency,"mcd2_fg_present_frame"));}}
 using Guides=int(*)(void*,void*,void*,void*,const MCD2FGCamera*);
 auto guides=bridge?reinterpret_cast<Guides>(GetProcAddress(bridge,"mcd2_fg_game_guides")):nullptr;
 struct Context{Guides guides;void*cmd,*depth,*motion;const MCD2FGCamera*camera;int result=-100;unsigned index=0;};
 // Inputs are tagged before Present chooses the mode. Reset while Off so the
 // first generated frame after a toggle or missing-input gap has fresh history.
 auto measured=*camera;if(sample==0||!activeFGMode.load())measured.reset=1;
 Context context{guides,cmd,depth,motion,&measured};
 const int tokenResult=visit&&guides?visit(camera->frame,[](void*token,unsigned index,void*data){auto&c=*static_cast<Context*>(data);c.index=index;c.result=c.guides(token,c.depth,c.motion,c.cmd,c.camera);return c.result;},&context):-1;
 // Do not hold the observer mutex while SDK calls invoke ReShade callbacks.
 std::lock_guard lock(mutex);gameFrames[camera->frame]={*camera,!tokenResult&&!context.result,false};while(gameFrames.size()>16)gameFrames.erase(gameFrames.begin());
 if(!fg_controls::enabled||sample<120||sample%60==0)privateLog<<"{\"kind\":\"game_guide_tags\",\"sample\":"<<sample<<",\"frameStamp\":"<<camera->frame<<",\"SDKIndex\":"<<context.index<<",\"tokenResult\":"<<tokenResult<<",\"SDKResult\":"<<context.result<<",\"reset\":"<<measured.reset<<",\"width\":"<<camera->width<<",\"height\":"<<camera->height<<"}\n";privateLog.flush();
}
extern "C" __declspec(dllexport) const char*NAME="MCD2 FG bounded guide reconnaissance";
extern "C" __declspec(dllexport) void mcd2_fg_sr_retired(){if(mcd2::process_exit::terminating())return;
 {std::lock_guard lock(mutex);if(!ownerClosing)return;}
 auto latency=GetModuleHandleW(L"mcd2-display-latency.addon64");auto quiesce=latency?reinterpret_cast<int(*)()>(GetProcAddress(latency,"mcd2_fg_quiesce")):nullptr;
 const int result=quiesce?quiesce():-1;
 std::lock_guard lock(mutex);privateLog<<"{\"kind\":\"shared_sdk_retired\",\"SRFeatureRetired\":true,\"quiesceAvailable\":"<<(quiesce?"true":"false")<<",\"SDKResult\":"<<result<<"}\n";privateLog.flush();
}
extern "C" __declspec(dllexport) const char*DESCRIPTION="Private bounded game-input and frame-generation experiment; disabled by default";
BOOL APIENTRY DllMain(HMODULE module,DWORD reason,LPVOID reserved){if(reason==DLL_PROCESS_ATTACH){if(!reshade::register_addon(module))return FALSE;
#define E(e,f) reshade::register_event<reshade::addon_event::e>(f)
 E(init_device,init_device);E(init_pipeline,init_pipeline);E(destroy_pipeline,destroy_pipeline);E(bind_pipeline,bind_pipeline);E(init_pipeline_layout,init_layout);E(destroy_pipeline_layout,destroy_layout);E(push_descriptors,push);E(push_constants,constants);E(bind_descriptor_tables,tables);E(update_descriptor_tables,update);E(copy_descriptor_tables,copy);E(init_resource,resource_init);E(destroy_resource,resource_destroy);E(barrier,barrier);E(execute_command_list,execute);E(reset_command_list,reset);E(destroy_command_list,reset);E(init_swapchain,swap_init);E(destroy_swapchain,swap_destroy);E(bind_render_targets_and_depth_stencil,targets);E(begin_render_pass,pass);E(dispatch,dispatch);E(draw,draw);E(draw_indexed,indexed);E(present,present);E(finish_present,finish);
#undef E
 }else if(reason==DLL_PROCESS_DETACH){if(reserved)mcd2::process_exit::mark_terminating();else reshade::unregister_addon(module);}return TRUE;}
extern "C" __declspec(dllexport) bool AddonInit(HMODULE,HMODULE){mcd2::process_exit::initialize();return true;}
extern "C" __declspec(dllexport) void AddonUninit(HMODULE,HMODULE){
 if(mcd2::process_exit::terminating())return;
 {std::lock_guard lock(mutex);retiring=true;ownerClosing=true;}try_retirement();
}
