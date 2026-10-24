#pragma once
#include "../../src/providers/bootstrap_module.hpp"
#include "fg_ui_protocol.hpp"
#include "fg_configuration.hpp"
#include "../providers/mfg_game_bridge.h"
#include "gdk_cache_clock.hpp"
#include "../../src/providers/menu_snapshot.h"
#include "../../src/providers/fg_menu_projection.hpp"
#include "../../src/providers/fg_capability_cache.hpp"
// Experimental UI bridge: current-process heartbeat, supported rendering LUID,
// explicit opt-in. All file reads/writes are rate limited and all paths heap backed.
namespace fg_controls {
std::filesystem::path settingsPath,runtimePath;
mcd2::fgui::Intent intent{};std::vector<uint8_t> seed,lastPublished;
std::atomic<bool> enabled{false},wanted{false};std::atomic<unsigned> fault{0};
std::atomic<bool> retired{false};
unsigned maximumGeneratedFrames=0,dynamicSupported=0,appliedFrameMode=0;
unsigned softwareMultiplierMask=0,mixedMultiplierMask=0,selectedStrategy=0,selectedChecksum=0,selectedSourceSession=0;
unsigned mfgTimingStatus=0,mfgRefreshMilliHz=0,mfgTargetRealMilliFps=0;
unsigned presenterRoute=0,presenterCanSwitch=0,presenterSettingsRevision=0;
unsigned session=0,available=0,availableProviders=0,startupOwner=0,previousMode=0,sessionMode=0,restartRequired=0;ULONGLONG nextPoll=0,nextSupport=0;
mcd2::providers::FgCapabilityCache amdCapabilities;
void forget_device(a::device* device){amdCapabilities.forget(reinterpret_cast<std::uintptr_t>(device));}
std::vector<uint8_t> read(const std::filesystem::path&p){std::ifstream f(p,std::ios::binary|std::ios::ate);if(!f)return {};auto n=f.tellg();if(n<64||n>8192)return {};std::vector<uint8_t>b(static_cast<size_t>(n));f.seekg(0);if(!f.read(reinterpret_cast<char*>(b.data()),b.size()))return {};return b;}
bool fresh(const std::filesystem::path&p){WIN32_FILE_ATTRIBUTE_DATA a{};if(!GetFileAttributesExW(p.c_str(),GetFileExInfoStandard,&a))return false;FILETIME now{};GetSystemTimeAsFileTime(&now);ULARGE_INTEGER n{},v{};n.LowPart=now.dwLowDateTime;n.HighPart=now.dwHighDateTime;v.LowPart=a.ftLastWriteTime.dwLowDateTime;v.HighPart=a.ftLastWriteTime.dwHighDateTime;return v.QuadPart<=n.QuadPart && n.QuadPart-v.QuadPart<fg_cache_fresh_budget();}
void init(const std::filesystem::path&folder){settingsPath=folder.parent_path()/L"SaveGames"/L"MCD2GraphicsFGSettings.sav";runtimePath=folder.parent_path()/L"SaveGames"/L"MCD2GraphicsFGRuntime.sav";session=(GetCurrentProcessId()^unsigned(GetTickCount64()))&0x7fffffff;if(!session)session=1;}
void publish(unsigned active,bool ready){
 if(!enabled)return;
 auto nativeModule=GetModuleHandleW(L"mcd2-graphics.addon64");
 auto nativeRetired=nativeModule?reinterpret_cast<int(*)()>(GetProcAddress(nativeModule,"mcd2_native_fg_retired_v1")):nullptr;
 const unsigned nativeGuidesRetired=nativeRetired&&nativeRetired()==1?1u:0u;
 const unsigned phase=fault?6:(!intent.mode?0:(!intent.ready?1:(active?3:2)));
 auto bytes=mcd2::fgui::encode(seed,{{"SchemaVersion",1},{"Revision",intent.revision},{"SessionId",session},{"Available",available},{"Active",active},{"Phase",phase},{"RestartRequired",restartRequired},{"AvailableProviders",availableProviders},{"StartupOwner",startupOwner},{"PresenterRoute",presenterRoute},{"PresenterCanSwitch",presenterCanSwitch},{"PresenterSettingsRevision",presenterSettingsRevision},{"NativeGuidesRetired",nativeGuidesRetired},{"MaximumGeneratedFrames",maximumGeneratedFrames},{"DynamicSupported",dynamicSupported},{"ActiveFrameMode",appliedFrameMode},{"SoftwareMultiplierMask",softwareMultiplierMask},{"MixedMultiplierMask",mixedMultiplierMask},{"MfgTimingStatus",mfgTimingStatus},{"MfgRefreshMilliHz",mfgRefreshMilliHz},{"MfgTargetRealMilliFps",mfgTargetRealMilliFps}});
 if(bytes.empty()||bytes==lastPublished)return;auto tmp=runtimePath;tmp+=L".fg.tmp";std::ofstream f(tmp,std::ios::binary|std::ios::trunc);if(!f.write(reinterpret_cast<const char*>(bytes.data()),bytes.size()))return;f.close();if(MoveFileExW(tmp.c_str(),runtimePath.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))lastPublished=std::move(bytes);
}
void poll(a::device*device,const std::filesystem::path&policy,unsigned active){
 if(settingsPath.empty()){wanted=false;return;}
 const auto now=GetTickCount64();if(now<nextPoll)return;nextPoll=now+200;enabled=!policy.empty()&&GetPrivateProfileIntW(L"Capture",L"NativeUIToggle",0,policy.c_str())==1;if(!enabled){wanted=false;return;}
 auto latency=GetModuleHandleW(L"mcd2-display-latency.addon64");
 auto menuEnabled=latency?reinterpret_cast<int(*)()>(GetProcAddress(latency,"mcd2_menu_enabled_v1")):nullptr;
 const bool shared=menuEnabled&&menuEnabled()==1;
 if(now>=nextSupport){nextSupport=now+1000;availableProviders=0;auto bridge=GetModuleHandleW(L"fg-sdk-bridge.dll");using Support=int(*)(void*,unsigned);auto support=bridge?reinterpret_cast<Support>(GetProcAddress(bridge,"mcd2_fg_support")):nullptr;auto native=reinterpret_cast<ID3D12Device*>(device->get_native());if(native&&support){auto luid=native->GetAdapterLuid();if(support(&luid,sizeof(luid))==0)availableProviders|=1;}
  auto amd=shared?GetModuleHandleW(L"mcd2-fsr-fg-game-bridge.dll"):nullptr;
  auto amdSupport=amd?reinterpret_cast<int(*)(void*)>(GetProcAddress(amd,"mcd2_afg_support_v1")):nullptr;
  if(native&&amdSupport&&amdCapabilities.query(reinterpret_cast<std::uintptr_t>(device),reinterpret_cast<std::uintptr_t>(amd),[&]{return amdSupport(native);})==0)availableProviders|=2;
 maximumGeneratedFrames=0;dynamicSupported=0;
 if(availableProviders&1){auto bridge=GetModuleHandleW(L"fg-sdk-bridge.dll");using Capabilities=int(*)(unsigned*,unsigned*);
  auto capabilities=bridge?reinterpret_cast<Capabilities>(GetProcAddress(bridge,"mcd2_fg_capabilities_v1")):nullptr;
  if(capabilities){unsigned maximum=0,dynamic=0;if(!capabilities(&maximum,&dynamic)){maximumGeneratedFrames=std::min(maximum,5u);dynamicSupported=dynamic==1?1:0;}}
 }

  softwareMultiplierMask=mixedMultiplierMask=0;
  if(shared){auto module=GetModuleHandleW(L"mcd2-mfg-game-bridge.dll");auto caps=module?reinterpret_cast<int(*)(void*,MCD2MfgCapabilitiesV1*)>(GetProcAddress(module,"mcd2_mfg_capabilities_v1")):nullptr;
   MCD2MfgCapabilitiesV1 value{};value.size=sizeof(value);
   if(caps&&native&&!caps(native,&value)&&!value.reserved){
    if(value.softwareReady==1&&value.softwareGeneratedMax>=1&&value.softwareGeneratedMax<=5&&(value.softwareMultiplierMask&~124u)==0)softwareMultiplierMask=value.softwareMultiplierMask;
    if(softwareMultiplierMask&&value.nativeReady==1&&value.nativeGeneratedMax>=1&&(value.mixedMultiplierMask&~80u)==0)mixedMultiplierMask=value.mixedMultiplierMask;
   }
  }

 }
 if(seed.empty())seed=read(runtimePath);
 mcd2::fgui::Intent next{};if(!mcd2::fgui::decode(read(settingsPath),next)||!fresh(settingsPath)){intent.ready=0;wanted=false;publish(active,false);return;}
 presenterRoute=presenterCanSwitch=presenterSettingsRevision=0;
 if(shared){auto routeBootstrap=mcd2::providers::bootstrapModule();auto custom=routeBootstrap?reinterpret_cast<unsigned(*)()>(GetProcAddress(routeBootstrap,"mcd2_bootstrap_mfg_fg_session")):nullptr;auto module=GetModuleHandleW(custom&&custom()==1?L"mcd2-mfg-game-bridge.dll":L"mcd2-fsr-fg-game-bridge.dll");using Status=int(*)(unsigned*,unsigned*);auto status=module?reinterpret_cast<Status>(GetProcAddress(module,"mcd2_afg_presenter_switch_state_v1")):nullptr;if(status&&status(&presenterRoute,&presenterCanSwitch)){presenterRoute=presenterCanSwitch=0;}}
 mfgTimingStatus=mfgRefreshMilliHz=mfgTargetRealMilliFps=0;
 if(shared){auto module=GetModuleHandleW(L"mcd2-mfg-game-bridge.dll");auto get=module?reinterpret_cast<int(*)(MCD2MfgTimingV1*)>(GetProcAddress(module,"mcd2_mfg_timing_v1")):nullptr;
  MCD2MfgTimingV1 timing{};timing.size=sizeof(timing);
  if(get&&!get(&timing)&&timing.status<=5&&timing.refreshMilliHz<=2000000){mfgTimingStatus=timing.status;mfgRefreshMilliHz=timing.refreshMilliHz;
   if(timing.scheduledRealPeriodNs>=2000000&&timing.scheduledRealPeriodNs<=250000000)mfgTargetRealMilliFps=unsigned(1000000000000ull/timing.scheduledRealPeriodNs);
  }
 }
 intent=next;
 const bool frameModeSupported=intent.provider==1?intent.frameMode==0:mcd2::fg::supportsFrameMode(intent.frameMode,maximumGeneratedFrames,false);
 available=(availableProviders&(1u<<intent.provider))?1:0;
 if(intent.mode!=previousMode){if(intent.mode)fault=0;previousMode=intent.mode;}
 if(seed.empty())seed=read(runtimePath);
 auto bootstrap=mcd2::providers::bootstrapModule();auto startup=bootstrap?reinterpret_cast<unsigned(*)()>(GetProcAddress(bootstrap,"mcd2_bootstrap_fg_session_mode")):nullptr;
 sessionMode=startup?startup():0;restartRequired=mcd2::fg::restartRequired(available,intent.mode,sessionMode,retired.load())?1:0;
 if(shared){
  auto owner=bootstrap?reinterpret_cast<unsigned(*)()>(GetProcAddress(bootstrap,"mcd2_bootstrap_fg_owner")):nullptr;startupOwner=owner?owner():0;
  auto snapshot=latency?reinterpret_cast<int(*)(MCD2MenuSnapshotV1*)>(GetProcAddress(latency,"mcd2_menu_snapshot_v1")):nullptr;
  MCD2MenuSnapshotV1 current{};current.size=sizeof(current);mcd2::providers::DecodedGraphicsRecord record;mcd2::providers::FgMenuProjection selection;
  const bool currentRequest=snapshot&&snapshot(&current)==0&&!current.load&&!current.reserved&&current.session&&
   mcd2::providers::decodeGraphicsRecord(current.record,record)&&mcd2::providers::projectFgMenu(record,selection)&&
   mcd2::providers::fgMenuMatches(selection,intent.settingsRevision,intent.provider,intent.mode)&&selection.frameMode==intent.frameMode;
  selectedStrategy=currentRequest?selection.strategy:0;selectedChecksum=currentRequest?record.bootstrap.stamp.checksum:0;selectedSourceSession=currentRequest?current.session:0;
  const bool custom=selectedStrategy==2||selectedStrategy==3;
  const auto mask=selectedStrategy==2?mixedMultiplierMask:softwareMultiplierMask;
  const bool admittedFrameMode=custom?(currentRequest&&selection.frameMode<5&&(mask&(1u<<(selection.frameMode+2)))!=0):frameModeSupported;
  if(custom)available=admittedFrameMode?1:0;
  presenterSettingsRevision=currentRequest?record.intent.revision:0;
  restartRequired=mcd2::providers::fgMenuRestartRequired(available,intent.mode!=0,startupOwner,intent.provider,retired.load(),selectedStrategy)?1:0;
  if(mcd2::providers::fgMenuNativeOffRecovered(currentRequest&&intent.session==session,available,intent.mode!=0,
      intent.provider,presenterRoute,presenterCanSwitch,fault.load(),retired.load(),selectedStrategy))restartRequired=0;
  wanted=admittedFrameMode&&currentRequest&&selection.pairEligible&&available&&intent.mode&&intent.ready&&intent.session==session&&
   mcd2::providers::fgOwnerMatches(startupOwner,intent.provider,selectedStrategy)&&!fault&&!retired;
  if(startupOwner==4){auto module=GetModuleHandleW(L"mcd2-mfg-game-bridge.dll");auto configure=module?reinterpret_cast<int(*)(unsigned,unsigned)>(GetProcAddress(module,"mcd2_mfg_configure_v1")):nullptr;
   const bool selectedOn=currentRequest&&custom&&intent.mode&&admittedFrameMode;
   const auto configured=currentRequest&&configure?configure(selectedOn?selectedStrategy:0,selectedOn?selection.frameMode+2:1):0;
   if(configured){wanted=false;fault=1;}if(wanted)appliedFrameMode=selection.frameMode;
  }
  publish(active,false);return;
 }
 wanted=frameModeSupported&&available&&sessionMode==1&&intent.mode&&intent.ready&&intent.session==session&&!fault&&!retired;
 publish(active,false);
}
bool current_selection(){
 if(!selectedSourceSession)return true; // Legacy source has its own heartbeat contract.
 auto latency=GetModuleHandleW(L"mcd2-display-latency.addon64");auto snapshot=latency?reinterpret_cast<int(*)(MCD2MenuSnapshotV1*)>(GetProcAddress(latency,"mcd2_menu_snapshot_v1")):nullptr;
 MCD2MenuSnapshotV1 current{};current.size=sizeof(current);mcd2::providers::DecodedGraphicsRecord record;mcd2::providers::FgMenuProjection p;
 return snapshot&&!snapshot(&current)&&!current.load&&!current.reserved&&current.session==selectedSourceSession&&
  mcd2::providers::decodeGraphicsRecord(current.record,record)&&record.bootstrap.stamp.checksum==selectedChecksum&&
  mcd2::providers::projectFgMenu(record,p)&&mcd2::providers::fgMenuSourceMatches(p,intent.settingsRevision,intent.provider,intent.mode,intent.frameMode,selectedStrategy);
}
}
