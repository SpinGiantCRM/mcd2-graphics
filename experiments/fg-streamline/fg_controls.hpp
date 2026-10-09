#pragma once
#include "fg_ui_protocol.hpp"
#include "fg_configuration.hpp"
#include "../../src/providers/menu_snapshot.h"
#include "../../src/providers/fg_menu_projection.hpp"
// Experimental UI bridge: current-process heartbeat, supported rendering LUID,
// explicit opt-in. All file reads/writes are rate limited and all paths heap backed.
namespace fg_controls {
std::filesystem::path settingsPath,runtimePath;
mcd2::fgui::Intent intent{};std::vector<uint8_t> seed,lastPublished;
std::atomic<bool> enabled{false},wanted{false};std::atomic<unsigned> fault{0};
std::atomic<bool> retired{false};
unsigned session=0,available=0,availableProviders=0,startupOwner=0,previousMode=0,sessionMode=0,restartRequired=0;ULONGLONG nextPoll=0,nextSupport=0;
std::vector<uint8_t> read(const std::filesystem::path&p){std::ifstream f(p,std::ios::binary|std::ios::ate);if(!f)return {};auto n=f.tellg();if(n<64||n>8192)return {};std::vector<uint8_t>b(static_cast<size_t>(n));f.seekg(0);if(!f.read(reinterpret_cast<char*>(b.data()),b.size()))return {};return b;}
bool fresh(const std::filesystem::path&p){WIN32_FILE_ATTRIBUTE_DATA a{};if(!GetFileAttributesExW(p.c_str(),GetFileExInfoStandard,&a))return false;FILETIME now{};GetSystemTimeAsFileTime(&now);ULARGE_INTEGER n{},v{};n.LowPart=now.dwLowDateTime;n.HighPart=now.dwHighDateTime;v.LowPart=a.ftLastWriteTime.dwLowDateTime;v.HighPart=a.ftLastWriteTime.dwHighDateTime;return v.QuadPart<=n.QuadPart && n.QuadPart-v.QuadPart<15000000;}
void init(const std::filesystem::path&folder){settingsPath=folder.parent_path()/L"SaveGames"/L"MCD2GraphicsFGSettings.sav";runtimePath=folder.parent_path()/L"SaveGames"/L"MCD2GraphicsFGRuntime.sav";session=(GetCurrentProcessId()^unsigned(GetTickCount64()))&0x7fffffff;if(!session)session=1;}
void publish(unsigned active,bool ready){
 if(!enabled)return;const unsigned phase=fault?6:(!intent.mode?0:(!intent.ready?1:(active?3:2)));
 auto bytes=mcd2::fgui::encode(seed,{{"SchemaVersion",1},{"Revision",intent.revision},{"SessionId",session},{"Available",available},{"Active",active},{"Phase",phase},{"RestartRequired",restartRequired},{"AvailableProviders",availableProviders},{"StartupOwner",startupOwner}});
 if(bytes.empty()||bytes==lastPublished)return;auto tmp=runtimePath;tmp+=L".fg.tmp";std::ofstream f(tmp,std::ios::binary|std::ios::trunc);if(!f.write(reinterpret_cast<const char*>(bytes.data()),bytes.size()))return;f.close();if(MoveFileExW(tmp.c_str(),runtimePath.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))lastPublished=std::move(bytes);
}
void poll(a::device*device,const std::filesystem::path&policy,unsigned active){
 const auto now=GetTickCount64();if(now<nextPoll)return;nextPoll=now+200;enabled=!policy.empty()&&GetPrivateProfileIntW(L"Capture",L"NativeUIToggle",0,policy.c_str())==1;if(!enabled){wanted=false;return;}
 auto latency=GetModuleHandleW(L"mcd2-display-latency.addon64");
 auto menuEnabled=latency?reinterpret_cast<int(*)()>(GetProcAddress(latency,"mcd2_menu_enabled_v1")):nullptr;
 const bool shared=menuEnabled&&menuEnabled()==1;
 if(now>=nextSupport){nextSupport=now+1000;availableProviders=0;auto bridge=GetModuleHandleW(L"fg-sdk-bridge.dll");using Support=int(*)(void*,unsigned);auto support=bridge?reinterpret_cast<Support>(GetProcAddress(bridge,"mcd2_fg_support")):nullptr;auto native=reinterpret_cast<ID3D12Device*>(device->get_native());if(native&&support){auto luid=native->GetAdapterLuid();if(support(&luid,sizeof(luid))==0)availableProviders|=1;}
  auto amd=shared?GetModuleHandleW(L"mcd2-fsr-fg-game-bridge.dll"):nullptr;
  auto amdSupport=amd?reinterpret_cast<int(*)(void*)>(GetProcAddress(amd,"mcd2_afg_support_v1")):nullptr;
  if(native&&amdSupport&&amdSupport(native)==0)availableProviders|=2;
 }
 mcd2::fgui::Intent next{};if(!mcd2::fgui::decode(read(settingsPath),next)||!fresh(settingsPath)){intent.ready=0;wanted=false;publish(active,false);return;}
 intent=next;available=(availableProviders&(1u<<intent.provider))?1:0;
 if(intent.mode!=previousMode){if(intent.mode)fault=0;previousMode=intent.mode;}
 if(seed.empty())seed=read(runtimePath);
 auto bootstrap=GetModuleHandleW(L"dxgi.dll");auto startup=bootstrap?reinterpret_cast<unsigned(*)()>(GetProcAddress(bootstrap,"mcd2_bootstrap_fg_session_mode")):nullptr;
 sessionMode=startup?startup():0;restartRequired=mcd2::fg::restartRequired(available,intent.mode,sessionMode,retired.load())?1:0;
 if(shared){
  auto owner=bootstrap?reinterpret_cast<unsigned(*)()>(GetProcAddress(bootstrap,"mcd2_bootstrap_fg_owner")):nullptr;startupOwner=owner?owner():0;
  auto snapshot=latency?reinterpret_cast<int(*)(MCD2MenuSnapshotV1*)>(GetProcAddress(latency,"mcd2_menu_snapshot_v1")):nullptr;
  MCD2MenuSnapshotV1 current{};current.size=sizeof(current);mcd2::providers::DecodedGraphicsRecord record;mcd2::providers::FgMenuProjection selection;
  const bool currentRequest=snapshot&&snapshot(&current)==0&&!current.load&&!current.reserved&&current.session&&
   mcd2::providers::decodeGraphicsRecord(current.record,record)&&mcd2::providers::projectFgMenu(record,selection)&&
   mcd2::providers::fgMenuMatches(selection,intent.settingsRevision,intent.provider,intent.mode);
  restartRequired=available&&((intent.mode&&!mcd2::providers::fgOwnerMatches(startupOwner,intent.provider))||(!intent.mode&&startupOwner!=0))?1:0;
  wanted=currentRequest&&selection.pairEligible&&available&&intent.mode&&intent.ready&&intent.session==session&&
   mcd2::providers::fgOwnerMatches(startupOwner,intent.provider)&&!fault&&!retired;
  publish(active,false);return;
 }
 wanted=available&&sessionMode==1&&intent.mode&&intent.ready&&intent.session==session&&!fault&&!retired;
 publish(active,false);
}
}
