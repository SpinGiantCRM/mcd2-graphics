// Independent display/latency addon. Released SR renderer remains unchanged.
// AddonInit runs before native device creation, but after DXGI factory creation:
// this bootstrap remains unqualified for FG until an earlier initialization path exists.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <reshade.hpp>
#include <array>
#include <atomic>
#include <mutex>
#include <fstream>
#include <filesystem>
#include <cstring>
#include <cstdint>
#include <memory>
#include "token_coordinator.hpp"
#include "display_protocol.hpp"
#include "../providers/observed_settings.hpp"
#include <condition_variable>
#include <thread>
#include <chrono>
namespace a=reshade::api;
namespace {
namespace sr=mcd2::streamline_reflex;
struct SlProvider:sr::Provider {
 HMODULE bridge=nullptr;int(*init)(const wchar_t*,const wchar_t*,const wchar_t*)=nullptr;int(*device)(void*)=nullptr;
 int(*setMode)(unsigned)=nullptr;int(*newToken)(unsigned,void**)=nullptr;unsigned(*tokenIndex)(void*)=nullptr;
 int(*sleepToken)(void*)=nullptr;int(*setMarker)(void*,unsigned)=nullptr;int(*getState)(unsigned*,unsigned*,unsigned*)=nullptr;
 int(*reportRange)(uint64_t*,uint64_t*,uint64_t*,uint64_t*)=nullptr;int(*pclMessage)(unsigned*)=nullptr;
 std::mutex configMutex;int(*stopSdk)()=nullptr;
 bool initialized=false;std::atomic<int> lastResult=0,lastError=0;
 template<class T>bool bind(T*&f,const char*name){f=reinterpret_cast<T*>(GetProcAddress(bridge,name));return f!=nullptr;}
 bool load(const std::filesystem::path&folder,const std::filesystem::path&logs){
  if(GetModuleHandleW(L"sl.interposer.dll"))return false;
  bridge=LoadLibraryExW((folder/L"mcd2-streamline-bridge.dll").c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);if(!bridge)return false;
  if(!bind(init,"mcd2_sl_init")||!bind(device,"mcd2_sl_set_device")||!bind(setMode,"mcd2_sl_mode")||!bind(newToken,"mcd2_sl_begin")||!bind(tokenIndex,"mcd2_sl_index")||!bind(sleepToken,"mcd2_sl_sleep")||!bind(setMarker,"mcd2_sl_marker")||!bind(getState,"mcd2_sl_state")||!bind(reportRange,"mcd2_sl_report_range")||!bind(pclMessage,"mcd2_sl_pcl_message")||!bind(stopSdk,"mcd2_sl_shutdown")){shutdown();return false;}
  lastResult=init((folder/L"sl.interposer.dll").c_str(),folder.c_str(),logs.c_str());initialized=lastResult==0;if(!initialized)shutdown();return initialized;
 }
 bool mode(sr::Mode m)override{std::lock_guard lock(configMutex);lastResult=setMode(unsigned(m));return lastResult==0;}
 int state(unsigned*a,unsigned*v,unsigned*c){std::lock_guard lock(configMutex);return initialized?getState(a,v,c):-1;}
 int reports(uint64_t*a,uint64_t*b,uint64_t*c,uint64_t*d){std::lock_guard lock(configMutex);return initialized?reportRange(a,b,c,d):-1;}
 int message(unsigned*out){std::lock_guard lock(configMutex);return initialized?pclMessage(out):-1;}
 void*begin(std::uint32_t id)override{void*t=nullptr;lastResult=newToken(id,&t);return lastResult==0?t:nullptr;}
 std::uint32_t index(void*t)override{return tokenIndex(t);}
 bool sleep(void*t)override{lastResult=sleepToken(t);return lastResult==0;}
 bool marker(void*t,sr::Marker m)override{lastResult=setMarker(t,unsigned(m));if(lastResult)lastError=lastResult.load();return lastResult==0;}
 void release()override{mode(sr::Mode::Off);}
 void shutdown(){std::lock_guard lock(configMutex);if(bridge&&stopSdk)stopSdk();initialized=false;if(bridge){FreeLibrary(bridge);bridge=nullptr;}stopSdk=nullptr;}
} provider;
std::shared_ptr<sr::TokenCoordinator> coordinator;std::atomic<uint64_t> firstFrame=0;uint64_t boundNative=0;
std::atomic<unsigned> currentMode=0,reflexAvailable=0,reflexFault=0,hdrRevision=0,hdrRestart=0;
std::atomic<bool> faultLogged=false;bool capabilityChecked=false;
void settings_loop();void apply_hdr_config();
std::filesystem::path savesPath;std::mutex intentMutex; mcd2::display::Intent latestIntent{};
std::thread settingsWorker;std::mutex workerMutex;std::condition_variable workerWake;std::atomic<bool> workerStop=false;
unsigned sessionId=0;
bool observeProviderSettings=false;
struct Feature {void** main;void** modular;};
std::mutex mutex;std::ofstream log;std::filesystem::path requestPath;unsigned requestId=0,presentCounter=0;uintptr_t base=0;void*registry=nullptr;
uint64_t latencyName=0,pacingName=0;std::atomic<bool> installed=false;
 HMODULE addonModule=nullptr;HHOOK messageHook=nullptr;DWORD messageThread=0;
 std::atomic<unsigned> statsMessage=0,pingsSeen=0,pingsSent=0;std::atomic<bool> pingPending=false;
std::array<void*,48> markerVtable{},pacerVtable{},secondaryVtable{};
Feature marker{},pacer{};
using RegistryMutation=void(*)(void*,uint64_t,void*);
using RegistryCount=int(*)(void*,uint64_t);
RegistryMutation add=nullptr,remove=nullptr;

thread_local bool presentActive=false;
thread_local uint64_t presentIdentity=0;
void record(unsigned,uint64_t,float=0){} // No per-frame diagnostics or copies.
std::shared_ptr<sr::TokenCoordinator> controller(){return std::atomic_load(&coordinator);}
void fault(){reflexFault=1;reflexAvailable=0;currentMode=0;workerWake.notify_all();}
void no_op(void*){}
bool yes(void*){return true;}
bool no(void*){return false;}
float zero(void*){return 0;}
void set_bool(void*,bool){}
void input(void*,uint64_t id){record(6,id);}
void forward(uint64_t id,sr::Marker m){auto c=controller();if(c&&c->active()&&firstFrame&&id>=firstFrame&&!c->mark(id,m))fault();}
void sim_start(void*,uint64_t id){record(0,id);forward(id,sr::Marker::SimulationStart);
 auto c=controller();if(pingPending.exchange(false)&&c&&c->active()){
  // Attribute queued PCL pings to the simulation that consumes the input.
  if(c->with_token(id,[](void*t,unsigned){return provider.setMarker(t,8)==0;}))++pingsSent;
 }
}
void sim_end(void*,uint64_t id){record(1,id);forward(id,sr::Marker::SimulationEnd);}
void render_start(void*,uint64_t id){record(2,id);forward(id,sr::Marker::RenderStart);}
void render_end(void*,uint64_t id){record(3,id);}
void present_start(void*,uint64_t id){presentIdentity=id;presentActive=true;record(4,id);}
void present_end(void*,uint64_t id){auto c=controller();if(c&&c->active()&&id>=firstFrame&&!c->verify_present_end(id))fault();presentActive=false;}
void flash(void*,uint64_t id){record(7,id);}
bool pacing(void*,float){auto id=*reinterpret_cast<uint64_t*>(base+0xbe45f10);auto c=controller();
 if(c&&c->active()&&id){if(!firstFrame)firstFrame=id;c->request_mode(sr::Mode(reflexAvailable?currentMode.load():0));if(!c->pre_simulation(id))fault();}
 // Streamline sleep precedes input/simulation even in Off. Preserve the native FPS limiter.
 return false;
}
unsigned flags(void*){return 0;}
void set_flags(void*,unsigned){}
bool signature(uintptr_t at,std::initializer_list<unsigned char>b){return std::memcmp(reinterpret_cast<void*>(at),b.begin(),b.size())==0;}
LRESULT CALLBACK pcl_messages(int code,WPARAM removed,LPARAM data){
 if(code>=0&&removed==PM_REMOVE&&installed.load()){
  auto*message=reinterpret_cast<MSG*>(data);auto wanted=statsMessage.load();
  // Inspect only the SDK's registered diagnostic message; no input capture.
  if(wanted&&message->message==wanted){++pingsSeen;pingPending=true;}
 }return CallNextHookEx(nullptr,code,removed,data);
}

std::vector<uint8_t> read_slot(const std::filesystem::path&p){std::ifstream f(p,std::ios::binary|std::ios::ate);if(!f)return {};auto n=f.tellg();if(n<64||n>8192)return {};std::vector<uint8_t>b(size_t(n),0);f.seekg(0);f.read(reinterpret_cast<char*>(b.data()),n);return f?b:std::vector<uint8_t>{};}
void settings_loop(){std::map<std::string,unsigned> last;auto nextStatus=std::chrono::steady_clock::now();
 std::ofstream status(savesPath.parent_path()/L"MCD2Graphics"/L"display-latency-status.jsonl",std::ios::trunc);
 mcd2::providers::ObservedSettings observations;
 mcd2::providers::ObservationResult observation;
 const auto mirrorPath=savesPath.parent_path()/L"MCD2Graphics"/L"ProviderMirror";
 mcd2::providers::GraphicsStore mirror(mirrorPath);
 // Reserved experimental directory. Disabled builds perform no mirror IO.
 if(observeProviderSettings){std::error_code error;std::filesystem::create_directory(mirrorPath,error);}
 while(!workerStop){
  if(observeProviderSettings){try{
   mcd2::providers::LegacySnapshot snapshot;
   if(mcd2::providers::readLegacySnapshot(savesPath,snapshot))observation=observations.observe(snapshot,mirror,[&](const auto& expected){
    mcd2::providers::LegacySnapshot fresh;return mcd2::providers::readLegacySnapshot(savesPath,fresh)&&fresh==expected;
   });
   else {observations.invalidate();observation={mcd2::providers::ObservationStatus::Rejected,mcd2::providers::StoreStatus::Invalid,{}};}
  }catch(...){observations.invalidate();observation={mcd2::providers::ObservationStatus::StoreFailure,mcd2::providers::StoreStatus::IoError,{}};}}
  try {auto bytes=read_slot(savesPath/L"MCD2GraphicsDisplaySettings.sav");mcd2::display::Intent incoming;
   if(mcd2::display::decode(bytes,incoming)){std::lock_guard lock(intentMutex);latestIntent=incoming;currentMode=reflexAvailable?incoming.reflex:0;}
   mcd2::display::Intent intent;{std::lock_guard lock(intentMutex);intent=latestIntent;}
   auto c=controller();unsigned applied=c&&c->active()?unsigned(c->mode()):0;
   std::map<std::string,unsigned> state={{"SchemaVersion",1},{"Revision",intent.revision},{"SessionId",sessionId},{"ReflexAvailable",reflexAvailable},{"ReflexMode",applied},{"ReflexFault",reflexFault},{"HDRRevision",hdrRevision},{"HDRRestartRequired",hdrRestart}};
   auto file=savesPath/L"MCD2GraphicsDisplayRuntime.sav";auto seed=read_slot(file);std::map<std::string,unsigned> existing;size_t h=0;
   if(state!=last||!mcd2::display::parse(seed,"DisplayRuntimeSave",existing,h)||existing["SessionId"]!=sessionId){auto data=mcd2::display::encode_runtime(seed,state);if(!data.empty()){
     auto temp=savesPath/L"MCD2GraphicsDisplayRuntime.pending";{std::ofstream f(temp,std::ios::binary|std::ios::trunc);f.write(reinterpret_cast<const char*>(data.data()),data.size());f.flush();if(!f)throw std::runtime_error("Runtime state write failed");}
     if(MoveFileExW(temp.c_str(),file.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))last=state;
   }}
   if(std::chrono::steady_clock::now()>=nextStatus){auto failure=c?c->marker_failure():sr::MarkerFailure{};unsigned available=0,valid=0,complete=0;uint64_t first=0,lastReport=0,sim=0,pre=0;auto result=provider.state(&available,&valid,&complete);if(!result)provider.reports(&first,&lastReport,&sim,&pre);
    if(observeProviderSettings)status<<"{\"kind\":\"provider_mirror\",\"state\":"<<unsigned(observation.status)<<",\"store\":"<<unsigned(observation.store)<<",\"revision\":"<<observation.stamp.revision<<"}\n";
    status<<"{\"session\":"<<sessionId<<",\"reflexAvailable\":"<<reflexAvailable<<",\"reflexFault\":"<<reflexFault<<",\"mode\":"<<applied<<",\"completedFrames\":"<<(c?c->completed():0)<<",\"coordinatorError\":"<<(c?c->error():0)<<",\"sdkMarkerError\":"<<provider.lastError<<",\"faultFrame\":"<<failure.frame<<",\"faultLastFrame\":"<<failure.last<<",\"faultMarker\":"<<failure.marker<<",\"faultSent\":"<<failure.sent<<",\"faultRequired\":"<<failure.required<<",\"faultFound\":"<<failure.found<<",\"faultReady\":"<<failure.ready<<",\"faultComplete\":"<<failure.complete<<",\"faultIdentity\":"<<failure.identity<<",\"sdkResult\":"<<result<<",\"sdkReportsAvailable\":"<<valid<<",\"sdkCompleteReports\":"<<complete<<",\"sdkFirstFrame\":"<<first<<",\"sdkLastFrame\":"<<lastReport<<",\"pingsSeen\":"<<pingsSeen<<",\"pingsSent\":"<<pingsSent<<",\"hdrRevision\":"<<hdrRevision<<",\"hdrRestart\":"<<hdrRestart<<"}\n";status.flush();nextStatus=std::chrono::steady_clock::now()+std::chrono::seconds(10);
   }
  }catch(...){/* Status failure cannot alter rendering or the user's other files. */}
  std::unique_lock lock(workerMutex);workerWake.wait_for(lock,std::chrono::milliseconds(500),[]{return workerStop.load();});
 }
}
void apply_hdr_config(){
 // Pinned RenoDX has no live settings API. Only its own config cache is updated;
 // calibration waits for restart. No private pointers, shader patching or fake live acknowledgement.
 static unsigned seen=0;static bool dirty=false;static unsigned profile=1;
 mcd2::display::Intent intent;{std::lock_guard lock(intentMutex);intent=latestIntent;}
 if(!intent.revision||seen==intent.revision)return;
 if(!GetModuleHandleW(L"renodx-ue-extended.addon64")){hdrRestart=1;hdrRevision=0;return;}
 if(!seen){unsigned selected=1;reshade::get_config_value(nullptr,"renodx","SelectedProfile",selected);if(selected>=1&&selected<=3)profile=selected;}
 auto section=std::string("renodx-preset")+std::to_string(profile);
 const std::array<std::pair<const char*,unsigned>,4> wanted={{{"ToneMapType",intent.hdr?1u:0u},{"ToneMapPeakNits",intent.peak},{"ToneMapGameNits",intent.paper},{"ToneMapUINits",intent.ui}}};
 for(auto&entry:wanted){float current=-1;reshade::get_config_value(nullptr,section.c_str(),entry.first,current);if(current!=float(entry.second)){log<<"{\"kind\":\"hdr_config_change\",\"revision\":"<<intent.revision<<",\"key\":\""<<entry.first<<"\",\"previous\":"<<current<<",\"next\":"<<entry.second<<"}\n";log.flush();dirty=true;reshade::set_config_value(nullptr,section.c_str(),entry.first,entry.second);}}
 // Once a value changes, its in-memory RenoDX binding is stale for this process.
 hdrRestart=dirty?1u:0u;hdrRevision=intent.revision;seen=intent.revision;workerWake.notify_all();
}

void destroy_swapchain(a::swapchain*swapchain,bool){if(swapchain->get_device()->get_native()==boundNative && messageHook){UnhookWindowsHookEx(messageHook);messageHook=nullptr;statsMessage=0;pingPending=false;}}
void init_swapchain(a::swapchain*swapchain,bool){
 if(!installed||messageHook||swapchain->get_device()->get_native()!=boundNative)return;
 auto hwnd=static_cast<HWND>(swapchain->get_hwnd());DWORD process=0;
 auto thread=GetWindowThreadProcessId(hwnd,&process);unsigned message=0;
 if(!thread||process!=GetCurrentProcessId()||provider.message(&message)||!message)return;
 statsMessage=message;messageHook=SetWindowsHookExW(WH_GETMESSAGE,pcl_messages,addonModule,thread);messageThread=messageHook?thread:0;
 log<<"{\"kind\":\"pcl_message_hook\",\"installed\":"<<(messageHook?"true":"false")<<",\"thread\":"<<messageThread<<"}\n";log.flush();
}
void init(a::device*device){
 if(installed||registry||!provider.initialized||device->get_api()!=a::device_api::d3d12)return;
 base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
 auto local=std::make_unique<wchar_t[]>(32768);auto length=GetEnvironmentVariableW(L"LOCALAPPDATA",local.get(),32768);if(!length||length>=32768)return;
 auto dir=std::filesystem::path(local.get())/L"Dungeons2"/L"Saved"/L"MCD2Graphics";std::filesystem::create_directories(dir);log.open(dir/L"display-latency-bootstrap.jsonl",std::ios::trunc);
 // Build 25647713: validate the call wrappers and registry layout before use.
 if(!signature(base+0x45546f0,{0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x74,0x24,0x10,0x57,0x48,0x83,0xec,0x30}) || !signature(base+0x4554769,{0xff,0x50,0x38}) || !signature(base+0x4554679,{0xff,0x50,0x40}) || !signature(base+0x4557b8a,{0xff,0x50,0x30}) || !signature(base+0x12a64d0,{0x48,0x83,0xec,0x28,0x8b,0x0d})){
  log<<"{\"kind\":\"rejected\",\"reason\":\"engine signature mismatch\"}\n";log.flush();return;
 }
 auto get=reinterpret_cast<void*(*)()>(base+0x12a64d0);registry=get();auto vt=*reinterpret_cast<uintptr_t**>(registry);
 if(reinterpret_cast<uintptr_t>(registry)!=base+0xba78270 || vt[3]!=base+0x12a7c80 || vt[5]!=base+0x12b4eb0 || vt[6]!=base+0x12bb990){log<<"{\"kind\":\"rejected\",\"reason\":\"registry ABI mismatch\"}\n";log.flush();return;}
 auto construct=reinterpret_cast<void(*)(uint64_t*,const wchar_t*,unsigned)>(base+0x1461040);
 construct(&latencyName,L"LatencyMarker",1);construct(&pacingName,L"MaxTickRateHandler",1);
 auto count=reinterpret_cast<RegistryCount>(vt[3]);log<<"{\"kind\":\"existing_features\",\"latency\":"<<count(registry,latencyName)<<",\"pacing\":"<<count(registry,pacingName)<<"}\n";
 if(count(registry,latencyName)||count(registry,pacingName)){log<<"{\"kind\":\"rejected\",\"reason\":\"existing modular owner\"}\n";return;}
 boundNative=device->get_native();auto setResult=provider.device(reinterpret_cast<void*>(boundNative));
 log<<"{\"kind\":\"set_device\",\"result\":"<<setResult<<"}\n";log.flush();if(setResult)return;
 std::atomic_store(&coordinator,std::make_shared<sr::TokenCoordinator>(provider));
 markerVtable.fill(reinterpret_cast<void*>(zero));pacerVtable.fill(reinterpret_cast<void*>(no_op));secondaryVtable.fill(reinterpret_cast<void*>(no_op));
 markerVtable[0]=reinterpret_cast<void*>(no_op);markerVtable[1]=reinterpret_cast<void*>(no_op);markerVtable[2]=reinterpret_cast<void*>(set_bool);markerVtable[3]=reinterpret_cast<void*>(yes);markerVtable[4]=reinterpret_cast<void*>(set_bool);markerVtable[5]=reinterpret_cast<void*>(no);
 markerVtable[6]=reinterpret_cast<void*>(input);markerVtable[7]=reinterpret_cast<void*>(sim_start);markerVtable[8]=reinterpret_cast<void*>(sim_end);markerVtable[9]=reinterpret_cast<void*>(present_start);markerVtable[10]=reinterpret_cast<void*>(present_end);markerVtable[11]=reinterpret_cast<void*>(render_start);markerVtable[12]=reinterpret_cast<void*>(render_end);markerVtable[13]=reinterpret_cast<void*>(flash);
 pacerVtable[0]=reinterpret_cast<void*>(no_op);pacerVtable[1]=reinterpret_cast<void*>(no_op);pacerVtable[2]=reinterpret_cast<void*>(set_bool);pacerVtable[3]=reinterpret_cast<void*>(yes);pacerVtable[4]=reinterpret_cast<void*>(set_flags);pacerVtable[5]=reinterpret_cast<void*>(flags);pacerVtable[6]=reinterpret_cast<void*>(pacing);
 marker={markerVtable.data(),secondaryVtable.data()};pacer={pacerVtable.data(),secondaryVtable.data()};add=reinterpret_cast<RegistryMutation>(vt[5]);remove=reinterpret_cast<RegistryMutation>(vt[6]);
 add(registry,latencyName,&marker.modular);add(registry,pacingName,&pacer.modular);installed=true;
 log<<"{\"kind\":\"registered\",\"gameThread\":"<<GetCurrentThreadId()<<"}\n";log.flush();
}
void present(a::command_queue*,a::swapchain*,const a::rect*,const a::rect*,unsigned,const a::rect*){if(installed){record(9,presentActive?presentIdentity:UINT64_MAX);record(11,*reinterpret_cast<uint64_t*>(base+0xbe45f18));if(presentActive){forward(presentIdentity,sr::Marker::RenderEnd);forward(presentIdentity,sr::Marker::PresentStart);}}}
void finish(a::command_queue*,a::swapchain*){
 auto c=controller();if(installed&&c){if(presentActive)forward(presentIdentity,sr::Marker::PresentEnd);
  if(!capabilityChecked){capabilityChecked=true;unsigned available=0,valid=0,complete=0;auto result=provider.state(&available,&valid,&complete);
   if(!result&&c->attach()){reflexAvailable=available?1:0;}else fault();workerWake.notify_all();
  }
 }
 apply_hdr_config();
}
void cleanup(){
 if(installed.exchange(false)){remove(registry,pacingName,&pacer.modular);remove(registry,latencyName,&marker.modular);}
 if(messageHook){UnhookWindowsHookEx(messageHook);messageHook=nullptr;}statsMessage=0;pingPending=false;
 auto c=std::atomic_exchange(&coordinator,std::shared_ptr<sr::TokenCoordinator>{});if(c){c->shutdown();c->drain();}provider.shutdown();reflexAvailable=0;boundNative=0;registry=nullptr;firstFrame=0;capabilityChecked=false;workerWake.notify_all();
}
void destroy(a::device*device){if(device->get_native()==boundNative)cleanup();}

}
extern "C" __declspec(dllexport) const char*NAME="MCD2 Graphics display and latency";
extern "C" __declspec(dllexport) const char*DESCRIPTION="Streamline Reflex and native HDR settings; independent of SR";
BOOL APIENTRY DllMain(HMODULE h,DWORD reason,LPVOID){if(reason==DLL_PROCESS_ATTACH){addonModule=h;if(!reshade::register_addon(h))return FALSE;reshade::register_event<reshade::addon_event::init_device>(init);reshade::register_event<reshade::addon_event::init_swapchain>(init_swapchain);reshade::register_event<reshade::addon_event::destroy_swapchain>(destroy_swapchain);reshade::register_event<reshade::addon_event::present>(present);reshade::register_event<reshade::addon_event::finish_present>(finish);reshade::register_event<reshade::addon_event::destroy_device>(destroy);}else if(reason==DLL_PROCESS_DETACH){reshade::unregister_addon(h);}return TRUE;}

extern "C" __declspec(dllexport) bool AddonInit(HMODULE module,HMODULE){
 try {
 auto filename=std::make_unique<wchar_t[]>(32768),local=std::make_unique<wchar_t[]>(32768);
 auto moduleLength=GetModuleFileNameW(module,filename.get(),32768),localLength=GetEnvironmentVariableW(L"LOCALAPPDATA",local.get(),32768);
 if(!moduleLength||moduleLength>=32768||!localLength||localLength>=32768)return true;
 auto logs=std::filesystem::path(local.get())/L"Dungeons2"/L"Saved"/L"MCD2Graphics"/L"StreamlineLogs";std::filesystem::create_directories(logs);
 auto folder=std::filesystem::path(filename.get()).parent_path()/L"MCD2Graphics"/L"streamline";if(provider.initialized)return true;auto success=provider.load(folder,logs);
 // Opt-in development handoff only; the released UI/renderer is unchanged.
 observeProviderSettings=GetPrivateProfileIntW(L"Providers",L"ObserveLegacySettings",0,(folder/L"FGBootstrap.ini").c_str())==1;
 std::ofstream receipt(logs/L"bootstrap.json");receipt<<"{\"initialized\":"<<(success?"true":"false")<<",\"result\":"<<provider.lastResult<<",\"stage\":\"AddonInit before native D3D12 device; DXGI factory may already exist\",\"fgBootstrapQualified\":false}\n";
 savesPath=std::filesystem::path(local.get())/L"Dungeons2"/L"Saved"/L"SaveGames";
 sessionId=unsigned((GetTickCount64()^(uint64_t(GetCurrentProcessId())<<12))&0x7fffffff);if(!sessionId)sessionId=1;
 workerStop=false;settingsWorker=std::thread(settings_loop);
 }catch(...){reflexAvailable=0;reflexFault=1;}return true;
}
extern "C" __declspec(dllexport) void AddonUninit(HMODULE,HMODULE){
 // ReShade may unload a probing device before init_device claims it. Release
 // our bootstrap outside DllMain as well; shutdown is deliberately idempotent.
 workerStop=true;workerWake.notify_all();if(settingsWorker.joinable())settingsWorker.join();cleanup();
}
