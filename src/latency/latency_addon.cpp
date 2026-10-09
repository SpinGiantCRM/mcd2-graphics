// Independent display/latency addon. Released SR renderer remains unchanged.
// AddonInit runs before native device creation, but after DXGI factory creation:
// this bootstrap remains unqualified for FG until an earlier initialization path exists.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
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
#include "../providers/menu_save_transport.hpp"
#include "../providers/display_projection.hpp"
#include "../providers/menu_snapshot.h"
#include "../providers/sr_runtime_snapshot.h"
#include "../providers/sr_runtime_save_transport.hpp"
#include "engine_layout.hpp"
#include "amd_latency.hpp"
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
struct AmdProvider : mcd2::amd_latency::Provider {
 HMODULE bridge=nullptr;
 int(*init)(void*)=nullptr;int(*updateFrame)(unsigned)=nullptr;
 int(*endRendering)()=nullptr;int(*realFrame)()=nullptr;void(*stop)()=nullptr;
 std::filesystem::path path;std::atomic<int> lastResult=0;
 template<class T>bool bind(T*& f,const char* name){f=reinterpret_cast<T*>(GetProcAddress(bridge,name));return f!=nullptr;}
 bool initialize(uintptr_t device) override {
  if(bridge){if(!init||!updateFrame||!endRendering||!realFrame||!stop)return false;
   lastResult=init(reinterpret_cast<void*>(device));return lastResult==0;}
  bridge=LoadLibraryExW(path.c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
  if(!bridge){lastResult=HRESULT_FROM_WIN32(GetLastError());return false;}
  auto abi=reinterpret_cast<unsigned(*)()>(GetProcAddress(bridge,"mcd2_al2_abi"));
  if(!abi||abi()!=1||!bind(init,"mcd2_al2_init")||!bind(updateFrame,"mcd2_al2_update")||
     !bind(endRendering,"mcd2_al2_end_rendering")||!bind(realFrame,"mcd2_al2_real_frame")||!bind(stop,"mcd2_al2_shutdown")){lastResult=E_NOINTERFACE;return false;}
  lastResult=init(reinterpret_cast<void*>(device));return lastResult==0;
 }
 bool update(bool enabled) override {lastResult=updateFrame(enabled?1:0);return lastResult==0;}
 bool end_rendering() override {lastResult=endRendering();return lastResult==0;}
 bool real_frame() override {lastResult=realFrame();return lastResult==0;}
 // Keep code mapped until callbacks are drained and AddonUninit has joined the
 // status worker. Device teardown releases only the driver context.
 void shutdown() override {if(stop)stop();}
 void unload(){shutdown();if(bridge)FreeLibrary(bridge);bridge=nullptr;init=nullptr;updateFrame=nullptr;endRendering=nullptr;realFrame=nullptr;stop=nullptr;}
} amdProvider;
std::shared_ptr<mcd2::amd_latency::Controller> amdCoordinator;
std::mutex amdRequestMutex;mcd2::amd_latency::Request amdRequest;
bool amdLatencyOptIn=false;
std::atomic<unsigned> amdCandidateRejected=0;
std::shared_ptr<sr::TokenCoordinator> coordinator;std::atomic<uint64_t> firstFrame=0;uint64_t boundNative=0;
std::atomic<unsigned> currentMode=0,reflexAvailable=0,reflexFault=0,hdrRevision=0,hdrRestart=0;
std::atomic<bool> faultLogged=false;bool capabilityChecked=false;
void settings_loop();void apply_hdr_config();
std::filesystem::path savesPath;std::mutex intentMutex; mcd2::display::Intent latestIntent{};
std::thread settingsWorker;std::mutex workerMutex;std::condition_variable workerWake;std::atomic<bool> workerStop=false;
unsigned sessionId=0;
bool observeProviderSettings=false;
bool consolidatedMenuTransport=false;
std::mutex providerSnapshotMutex;
MCD2MenuSnapshotV1 providerSnapshot{sizeof(MCD2MenuSnapshotV1),0,2,0,{}};
MCD2SrContextSnapshotV1 srContextSnapshot{};
struct Feature {void** main;void** modular;};
std::mutex mutex;std::ofstream log;std::filesystem::path requestPath;unsigned requestId=0,presentCounter=0;uintptr_t base=0;const mcd2::engine::Layout*engineLayout=nullptr;void*registry=nullptr;
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
std::shared_ptr<mcd2::amd_latency::Controller> amd_controller(){return std::atomic_load(&amdCoordinator);}
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
bool pacing(void*,float){auto id=*reinterpret_cast<uint64_t*>(base+engineLayout->simulationCounter);auto c=controller();
 if(amdLatencyOptIn){if(auto amd=amd_controller()){mcd2::amd_latency::Request request;{std::lock_guard lock(amdRequestMutex);request=amdRequest;}amd->pre_input(id,request);}}
 if(c&&c->active()&&id){if(!firstFrame)firstFrame=id;c->request_mode(sr::Mode(reflexAvailable?currentMode.load():0));if(!c->pre_simulation(id))fault();}
 // Streamline sleep precedes input/simulation even in Off. Preserve the native FPS limiter.
 return false;
}
unsigned flags(void*){return 0;}
void set_flags(void*,unsigned){}
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
 const auto authorityPath=savesPath.parent_path()/L"MCD2Graphics"/L"ProviderSettings";
 std::unique_ptr<mcd2::providers::MenuSaveTransport> menuTransport;
 if(consolidatedMenuTransport){std::error_code error;std::filesystem::create_directory(authorityPath,error);
  if(!error&&mcd2::providers::store_detail::directoryReady(authorityPath))
   menuTransport=std::make_unique<mcd2::providers::MenuSaveTransport>(savesPath,authorityPath,sessionId);
 }
 std::uint32_t contextSequence=0;
 while(!workerStop){
  mcd2::providers::MenuAuthority shared;
  if(menuTransport){try{shared=menuTransport->poll();}catch(...){/* A transport failure cannot activate a feature. */}}
  if(consolidatedMenuTransport){
   MCD2MenuSnapshotV1 snapshot{sizeof(MCD2MenuSnapshotV1),sessionId,unsigned(shared.load),0,{}};
   mcd2::providers::GraphicsRecord bytes;
   if(shared.load==mcd2::providers::StoreStatus::Ok && mcd2::providers::encodeGraphicsRecord(shared.current.intent,bytes))
    std::copy(bytes.begin(),bytes.end(),snapshot.record);
   else snapshot.load=unsigned(mcd2::providers::StoreStatus::Invalid);
   mcd2::providers::SrRuntimeBytes contextBytes; mcd2::providers::SrContext context;
   bool validContext=mcd2::providers::readSrContextSave(savesPath/L"MCD2GraphicsProviderSrContext.sav",contextBytes)&&
       mcd2::providers::decodeSrContext(contextBytes,context)&&context.session==sessionId&&
       context.intent==shared.current.bootstrap.stamp;
   {std::lock_guard lock(providerSnapshotMutex);providerSnapshot=snapshot;
    if(!validContext){srContextSnapshot={};}
    else if(context.sequence>contextSequence){
     contextSequence=context.sequence;
     srContextSnapshot={};srContextSnapshot.size=sizeof(srContextSnapshot);srContextSnapshot.observedAtMs=GetTickCount64();
     srContextSnapshot.authority=snapshot;std::copy(contextBytes.begin(),contextBytes.end(),srContextSnapshot.context);
    }
   }
   // The worker alone serializes the renderer's volatile response. No save or
   // SDK call runs on a render callback, and this is not a commit receipt.
   auto renderer=GetModuleHandleW(L"mcd2-graphics.addon64");
   auto runtime=renderer?reinterpret_cast<decltype(&mcd2_sr_runtime_v1)>(GetProcAddress(renderer,"mcd2_sr_runtime_v1")):nullptr;
   MCD2SrRuntimeSnapshotV1 response{};response.size=sizeof(response);
   if(runtime&&runtime(&response)==0&&response.reserved==0){
    mcd2::providers::SrRuntimeBytes words;std::copy(std::begin(response.state),std::end(response.state),words.begin());
    mcd2::providers::SrRuntimeState state;
    if(mcd2::providers::decodeSrRuntimeState(words,state)&&state.session==sessionId&&state.intent==shared.current.bootstrap.stamp)
     mcd2::providers::publishSrRuntimeSave(savesPath,words);
   }
  }
  if(amdLatencyOptIn){mcd2::amd_latency::Request request;
   try{mcd2::providers::DecodedGraphicsRecord record;mcd2::providers::GraphicsStore authority(authorityPath);
    if(consolidatedMenuTransport){if(shared.load==mcd2::providers::StoreStatus::Ok)request=mcd2::amd_latency::resolve(shared.current);}
    else if(authority.load(record).status==mcd2::providers::StoreStatus::Ok)request=mcd2::amd_latency::resolve(record);
   }catch(...){/* Missing/corrupt authority resolves to Off, never legacy Reflex. */}
   std::lock_guard lock(amdRequestMutex);amdRequest=request;
  }
  if(observeProviderSettings){try{
   mcd2::providers::LegacySnapshot snapshot;
   if(mcd2::providers::readLegacySnapshot(savesPath,snapshot))observation=observations.observe(snapshot,mirror,[&](const auto& expected){
    mcd2::providers::LegacySnapshot fresh;return mcd2::providers::readLegacySnapshot(savesPath,fresh)&&fresh==expected;
   });
   else {observations.invalidate();observation={mcd2::providers::ObservationStatus::Rejected,mcd2::providers::StoreStatus::Invalid,{}};}
  }catch(...){observations.invalidate();observation={mcd2::providers::ObservationStatus::StoreFailure,mcd2::providers::StoreStatus::IoError,{}};}}
  try {mcd2::display::Intent incoming;bool publish=false;
   if(consolidatedMenuTransport){
    if(shared.load==mcd2::providers::StoreStatus::Ok){auto p=mcd2::providers::projectDisplay(shared.current);
     if(p.valid)incoming={p.revision,p.hdr,p.peak,p.paper,p.ui,p.reflex};
    }
    // Invalid shared state disables latency; revision zero leaves HDR untouched.
    // Never replace committed provider intent with a legacy display preference.
    publish=true;
   }else {auto bytes=read_slot(savesPath/L"MCD2GraphicsDisplaySettings.sav");publish=mcd2::display::decode(bytes,incoming);}
   if(publish){std::lock_guard lock(intentMutex);latestIntent=incoming;currentMode=reflexAvailable?incoming.reflex:0;}
   mcd2::display::Intent intent;{std::lock_guard lock(intentMutex);intent=latestIntent;}
   auto c=controller();unsigned applied=c&&c->active()?unsigned(c->mode()):0;
   std::map<std::string,unsigned> state={{"SchemaVersion",1},{"Revision",intent.revision},{"SessionId",sessionId},{"ReflexAvailable",reflexAvailable},{"ReflexMode",applied},{"ReflexFault",reflexFault},{"HDRRevision",hdrRevision},{"HDRRestartRequired",hdrRestart}};
   // Capability comes from the actual driver interface and installed timing
   // owner, never a requested preference or adapter name in a save.
   auto amd=amd_controller();auto amdState=amd?amd->state():mcd2::amd_latency::State{};
   state["AmdAntiLagAvailable"]=amdLatencyOptIn&&installed&&amdState.available&&!amdState.fault;
   state["AmdAntiLagMode"]=amdState.enabled?1u:0u;
   state["AmdAntiLagFault"]=amdState.fault?1u:0u;
   state["AmdAntiLagRevision"]=amdState.revision;
   auto file=savesPath/L"MCD2GraphicsDisplayRuntime.sav";auto seed=read_slot(file);std::map<std::string,unsigned> existing;size_t h=0;
   if(state!=last||!mcd2::display::parse(seed,"DisplayRuntimeSave",existing,h)||existing["SessionId"]!=sessionId){auto data=mcd2::display::encode_runtime(seed,state);if(!data.empty()){
     auto temp=savesPath/L"MCD2GraphicsDisplayRuntime.pending";{std::ofstream f(temp,std::ios::binary|std::ios::trunc);f.write(reinterpret_cast<const char*>(data.data()),data.size());f.flush();if(!f)throw std::runtime_error("Runtime state write failed");}
     if(MoveFileExW(temp.c_str(),file.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))last=state;
   }}
   if(std::chrono::steady_clock::now()>=nextStatus){auto failure=c?c->marker_failure():sr::MarkerFailure{};unsigned available=0,valid=0,complete=0;uint64_t first=0,lastReport=0,sim=0,pre=0;auto result=provider.state(&available,&valid,&complete);if(!result)provider.reports(&first,&lastReport,&sim,&pre);
    if(amdLatencyOptIn){auto amd=amd_controller();auto s=amd?amd->state():mcd2::amd_latency::State{};
     status<<"{\"kind\":\"amd_antilag2\",\"available\":"<<s.available<<",\"enabled\":"<<s.enabled<<",\"fault\":"<<s.fault<<",\"revision\":"<<s.revision<<",\"checksum\":"<<s.checksum<<",\"inputFrames\":"<<s.inputFrames<<",\"renderedFrames\":"<<s.renderedFrames<<",\"candidateRejected\":"<<amdCandidateRejected<<",\"SDKResult\":"<<amdProvider.lastResult<<"}\n";
    }
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
 if(!installed||amd_controller()||messageHook||swapchain->get_device()->get_native()!=boundNative)return;
 auto hwnd=static_cast<HWND>(swapchain->get_hwnd());DWORD process=0;
 auto thread=GetWindowThreadProcessId(hwnd,&process);unsigned message=0;
 if(!thread||process!=GetCurrentProcessId()||provider.message(&message)||!message)return;
 statsMessage=message;messageHook=SetWindowsHookExW(WH_GETMESSAGE,pcl_messages,addonModule,thread);messageThread=messageHook?thread:0;
 log<<"{\"kind\":\"pcl_message_hook\",\"installed\":"<<(messageHook?"true":"false")<<",\"thread\":"<<messageThread<<"}\n";log.flush();
}
void init(a::device*device){
 auto bootstrap=GetModuleHandleW(L"dxgi.dll");auto amdFg=bootstrap?reinterpret_cast<unsigned(*)()>(GetProcAddress(bootstrap,"mcd2_bootstrap_amd_fg_session")):nullptr;
 const bool frameIdentityOnly=amdFg&&amdFg()==1;
 if(installed||registry||(!provider.initialized&&!amdLatencyOptIn&&!frameIdentityOnly)||device->get_api()!=a::device_api::d3d12)return;
 // The actual rendering device LUID determines the AMD route. Never use the
 // display/default adapter and never attempt to load an AMD driver on Wine.
 bool useAmd=false;
 if(amdLatencyOptIn){unsigned vendor=0;IDXGIFactory4* factory=nullptr;IDXGIAdapter1* adapter=nullptr;
  auto native=reinterpret_cast<ID3D12Device*>(device->get_native());DXGI_ADAPTER_DESC1 desc{};
  if(SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))){
   if(SUCCEEDED(factory->EnumAdapterByLuid(native->GetAdapterLuid(),IID_PPV_ARGS(&adapter)))){
    if(SUCCEEDED(adapter->GetDesc1(&desc))&&!(desc.Flags&DXGI_ADAPTER_FLAG_SOFTWARE))vendor=desc.VendorId;
    adapter->Release();
   }factory->Release();
  }
  const auto ntdll=GetModuleHandleW(L"ntdll.dll");const bool wine=ntdll&&GetProcAddress(ntdll,"wine_get_version");
  useAmd=mcd2::amd_latency::Eligibility{true,!wine,GetModuleHandleW(L"amdxc64.dll")!=nullptr,vendor}.candidate();
  if(!useAmd)amdCandidateRejected=1;
 }
 if(!useAmd&&!provider.initialized&&!frameIdentityOnly)return;
 base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
 auto local=std::make_unique<wchar_t[]>(32768);auto length=GetEnvironmentVariableW(L"LOCALAPPDATA",local.get(),32768);if(!length||length>=32768)return;
 auto dir=std::filesystem::path(local.get())/L"Dungeons2"/L"Saved"/L"MCD2Graphics";std::filesystem::create_directories(dir);log.open(dir/L"display-latency-bootstrap.jsonl",std::ios::trunc);
 // Select only an inspected layout; unsupported builds keep the hooks absent.
 auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
 if(dos->e_magic!=IMAGE_DOS_SIGNATURE||dos->e_lfanew<0||dos->e_lfanew>0x100000)return;
 auto nt=reinterpret_cast<const IMAGE_NT_HEADERS64*>(base+dos->e_lfanew);
 if(nt->Signature!=IMAGE_NT_SIGNATURE||nt->FileHeader.Machine!=IMAGE_FILE_MACHINE_AMD64||nt->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC)return;
 const auto imageSize=nt->OptionalHeader.SizeOfImage;
 auto match=[&](std::uint32_t rva,std::span<const std::uint8_t> bytes){
  return rva<imageSize&&bytes.size()<=imageSize-rva&&std::memcmp(reinterpret_cast<const void*>(base+rva),bytes.data(),bytes.size())==0;
 };
 engineLayout=mcd2::engine::select(nt->FileHeader.TimeDateStamp,imageSize,match);
 if(!engineLayout){log<<"{\"kind\":\"rejected\",\"reason\":\"engine signature mismatch\"}\n";log.flush();return;}
 auto get=reinterpret_cast<void*(*)()>(base+engineLayout->registryGet);registry=get();
 if(reinterpret_cast<uintptr_t>(registry)!=base+engineLayout->registry){log<<"{\"kind\":\"rejected\",\"reason\":\"registry ABI mismatch\"}\n";log.flush();return;}
 auto vt=*reinterpret_cast<uintptr_t**>(registry);
 auto vtAddress=reinterpret_cast<uintptr_t>(vt);
 if(vtAddress<base||vtAddress-base>imageSize||7*sizeof(uintptr_t)>imageSize-(vtAddress-base)||vt[3]!=base+engineLayout->count||vt[5]!=base+engineLayout->add||vt[6]!=base+engineLayout->remove){log<<"{\"kind\":\"rejected\",\"reason\":\"registry ABI mismatch\"}\n";log.flush();return;}
 log<<"{\"kind\":\"engine_layout\",\"steamBuild\":"<<engineLayout->steamBuild<<"}\n";log.flush();
 auto construct=reinterpret_cast<void(*)(uint64_t*,const wchar_t*,unsigned)>(base+engineLayout->nameConstructor);
 construct(&latencyName,L"LatencyMarker",1);construct(&pacingName,L"MaxTickRateHandler",1);
 auto count=reinterpret_cast<RegistryCount>(vt[3]);log<<"{\"kind\":\"existing_features\",\"latency\":"<<count(registry,latencyName)<<",\"pacing\":"<<count(registry,pacingName)<<"}\n";
 if(count(registry,latencyName)||count(registry,pacingName)){log<<"{\"kind\":\"rejected\",\"reason\":\"existing modular owner\"}\n";return;}
 boundNative=device->get_native();
 if(useAmd){auto amd=std::make_shared<mcd2::amd_latency::Controller>(amdProvider);
  if(!amd->attach({true,true,true,0x1002},boundNative)){boundNative=0;registry=nullptr;return;}
  std::atomic_store(&amdCoordinator,amd);reflexAvailable=0;
  log<<"{\"kind\":\"amd_antilag2_device\",\"available\":true}\n";log.flush();
 }else if(provider.initialized){auto setResult=provider.device(reinterpret_cast<void*>(boundNative));
  log<<"{\"kind\":\"set_device\",\"result\":"<<setResult<<"}\n";log.flush();if(setResult&&!frameIdentityOnly)return;
  if(!setResult)std::atomic_store(&coordinator,std::make_shared<sr::TokenCoordinator>(provider));
  else {reflexAvailable=0;log<<"{\"kind\":\"frame_identity_only\"}\n";log.flush();}
 }else{reflexAvailable=0;log<<"{\"kind\":\"frame_identity_only\"}\n";log.flush();
 }
 markerVtable.fill(reinterpret_cast<void*>(zero));pacerVtable.fill(reinterpret_cast<void*>(no_op));secondaryVtable.fill(reinterpret_cast<void*>(no_op));
 markerVtable[0]=reinterpret_cast<void*>(no_op);markerVtable[1]=reinterpret_cast<void*>(no_op);markerVtable[2]=reinterpret_cast<void*>(set_bool);markerVtable[3]=reinterpret_cast<void*>(yes);markerVtable[4]=reinterpret_cast<void*>(set_bool);markerVtable[5]=reinterpret_cast<void*>(no);
 markerVtable[6]=reinterpret_cast<void*>(input);markerVtable[7]=reinterpret_cast<void*>(sim_start);markerVtable[8]=reinterpret_cast<void*>(sim_end);markerVtable[9]=reinterpret_cast<void*>(present_start);markerVtable[10]=reinterpret_cast<void*>(present_end);markerVtable[11]=reinterpret_cast<void*>(render_start);markerVtable[12]=reinterpret_cast<void*>(render_end);markerVtable[13]=reinterpret_cast<void*>(flash);
 pacerVtable[0]=reinterpret_cast<void*>(no_op);pacerVtable[1]=reinterpret_cast<void*>(no_op);pacerVtable[2]=reinterpret_cast<void*>(set_bool);pacerVtable[3]=reinterpret_cast<void*>(yes);pacerVtable[4]=reinterpret_cast<void*>(set_flags);pacerVtable[5]=reinterpret_cast<void*>(flags);pacerVtable[6]=reinterpret_cast<void*>(pacing);
 marker={markerVtable.data(),secondaryVtable.data()};pacer={pacerVtable.data(),secondaryVtable.data()};add=reinterpret_cast<RegistryMutation>(vt[5]);remove=reinterpret_cast<RegistryMutation>(vt[6]);
 add(registry,latencyName,&marker.modular);add(registry,pacingName,&pacer.modular);installed=true;
 log<<"{\"kind\":\"registered\",\"gameThread\":"<<GetCurrentThreadId()<<"}\n";log.flush();
}
void present(a::command_queue*,a::swapchain* swapchain,const a::rect*,const a::rect*,unsigned,const a::rect*){if(installed&&swapchain->get_device()->get_native()==boundNative){record(9,presentActive?presentIdentity:UINT64_MAX);record(11,*reinterpret_cast<uint64_t*>(base+engineLayout->renderCounter));if(presentActive){if(amdLatencyOptIn){if(auto amd=amd_controller())amd->pre_present(presentIdentity);}forward(presentIdentity,sr::Marker::RenderEnd);forward(presentIdentity,sr::Marker::PresentStart);}}}
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
 auto amd=std::atomic_exchange(&amdCoordinator,std::shared_ptr<mcd2::amd_latency::Controller>{});if(amd)amd->shutdown();
 auto c=std::atomic_exchange(&coordinator,std::shared_ptr<sr::TokenCoordinator>{});if(c){c->shutdown();c->drain();}provider.shutdown();reflexAvailable=0;boundNative=0;registry=nullptr;firstFrame=0;capabilityChecked=false;workerWake.notify_all();
}
void destroy(a::device*device){if(device->get_native()==boundNative)cleanup();}

}
extern "C" __declspec(dllexport) int mcd2_menu_snapshot_v1(MCD2MenuSnapshotV1* snapshot){
 if(!snapshot || snapshot->size!=sizeof(MCD2MenuSnapshotV1))return -1;
 std::unique_lock lock(providerSnapshotMutex,std::try_to_lock);if(!lock.owns_lock())return -2;
 if(workerStop || !consolidatedMenuTransport || providerSnapshot.load!=0 || !providerSnapshot.session)return -3;
 *snapshot=providerSnapshot;return 0;
}
extern "C" __declspec(dllexport) int mcd2_menu_enabled_v1(){return consolidatedMenuTransport?1:0;}
extern "C" __declspec(dllexport) int mcd2_sr_context_v1(MCD2SrContextSnapshotV1* snapshot){
 if(!snapshot||snapshot->size!=sizeof(MCD2SrContextSnapshotV1))return -1;
 std::unique_lock lock(providerSnapshotMutex,std::try_to_lock);if(!lock.owns_lock())return -2;
 if(workerStop||!consolidatedMenuTransport||srContextSnapshot.size!=sizeof(srContextSnapshot)||
    GetTickCount64()-srContextSnapshot.observedAtMs>3000)return -3;
 *snapshot=srContextSnapshot;return 0;
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
 observeProviderSettings=GetPrivateProfileIntW(L"Providers",L"ObserveLegacySettings",0,mcd2::providers::observedPolicyPath(filename.get()).c_str())==1;
 consolidatedMenuTransport=GetPrivateProfileIntW(L"Providers",L"ConsolidatedMenuTransport",0,mcd2::providers::observedPolicyPath(filename.get()).c_str())==1;
 amdLatencyOptIn=GetPrivateProfileIntW(L"Providers",L"AmdAntiLag2",0,mcd2::providers::observedPolicyPath(filename.get()).c_str())==1;
 amdProvider.path=std::filesystem::path(filename.get()).parent_path()/L"mcd2-antilag2-bridge.dll";
 std::ofstream receipt(logs/L"bootstrap.json");receipt<<"{\"initialized\":"<<(success?"true":"false")<<",\"result\":"<<provider.lastResult<<",\"stage\":\"AddonInit before native D3D12 device; DXGI factory may already exist\",\"fgBootstrapQualified\":false}\n";
 savesPath=std::filesystem::path(local.get())/L"Dungeons2"/L"Saved"/L"SaveGames";
 sessionId=unsigned((GetTickCount64()^(uint64_t(GetCurrentProcessId())<<12))&0x7fffffff);if(!sessionId)sessionId=1;
 workerStop=false;settingsWorker=std::thread(settings_loop);
 }catch(...){reflexAvailable=0;reflexFault=1;}return true;
}
extern "C" __declspec(dllexport) void AddonUninit(HMODULE,HMODULE){
 // ReShade may unload a probing device before init_device claims it. Release
 // our bootstrap outside DllMain as well; shutdown is deliberately idempotent.
 workerStop=true;workerWake.notify_all();if(settingsWorker.joinable())settingsWorker.join();cleanup();amdProvider.unload();
}
