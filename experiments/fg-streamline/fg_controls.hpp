#pragma once
#include "fg_ui_protocol.hpp"
#include "fg_configuration.hpp"
// Experimental UI bridge: current-process heartbeat, supported rendering LUID,
// explicit opt-in. All file reads/writes are rate limited and all paths heap backed.
namespace fg_controls {
std::filesystem::path settingsPath,runtimePath;
mcd2::fgui::Intent intent{};std::vector<uint8_t> seed,lastPublished;
std::atomic<bool> enabled{false},wanted{false};std::atomic<unsigned> fault{0};
std::atomic<bool> retired{false};
unsigned session=0,available=0,previousMode=0,sessionMode=0,restartRequired=0;ULONGLONG nextPoll=0,nextSupport=0;
std::vector<uint8_t> read(const std::filesystem::path&p){std::ifstream f(p,std::ios::binary|std::ios::ate);if(!f)return {};auto n=f.tellg();if(n<64||n>8192)return {};std::vector<uint8_t>b(static_cast<size_t>(n));f.seekg(0);if(!f.read(reinterpret_cast<char*>(b.data()),b.size()))return {};return b;}
bool fresh(const std::filesystem::path&p){WIN32_FILE_ATTRIBUTE_DATA a{};if(!GetFileAttributesExW(p.c_str(),GetFileExInfoStandard,&a))return false;FILETIME now{};GetSystemTimeAsFileTime(&now);ULARGE_INTEGER n{},v{};n.LowPart=now.dwLowDateTime;n.HighPart=now.dwHighDateTime;v.LowPart=a.ftLastWriteTime.dwLowDateTime;v.HighPart=a.ftLastWriteTime.dwHighDateTime;return v.QuadPart<=n.QuadPart && n.QuadPart-v.QuadPart<15000000;}
void init(const std::filesystem::path&folder){settingsPath=folder.parent_path()/L"SaveGames"/L"MCD2GraphicsFGSettings.sav";runtimePath=folder.parent_path()/L"SaveGames"/L"MCD2GraphicsFGRuntime.sav";session=(GetCurrentProcessId()^unsigned(GetTickCount64()))&0x7fffffff;if(!session)session=1;}
void publish(unsigned active,bool ready){
 if(!enabled)return;const unsigned phase=fault?6:(!intent.mode?0:(!intent.ready?1:(active?3:2)));
 auto bytes=mcd2::fgui::encode(seed,{{"SchemaVersion",1},{"Revision",intent.revision},{"SessionId",session},{"Available",available},{"Active",active},{"Phase",phase},{"RestartRequired",restartRequired}});
 if(bytes.empty()||bytes==lastPublished)return;auto tmp=runtimePath;tmp+=L".fg.tmp";std::ofstream f(tmp,std::ios::binary|std::ios::trunc);if(!f.write(reinterpret_cast<const char*>(bytes.data()),bytes.size()))return;f.close();if(MoveFileExW(tmp.c_str(),runtimePath.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))lastPublished=std::move(bytes);
}
void poll(a::device*device,const std::filesystem::path&policy,unsigned active){
 const auto now=GetTickCount64();if(now<nextPoll)return;nextPoll=now+200;enabled=!policy.empty()&&GetPrivateProfileIntW(L"Capture",L"NativeUIToggle",0,policy.c_str())==1;if(!enabled){wanted=false;return;}
 if(now>=nextSupport){nextSupport=now+1000;auto bridge=GetModuleHandleW(L"fg-sdk-bridge.dll");using Support=int(*)(void*,unsigned);auto support=bridge?reinterpret_cast<Support>(GetProcAddress(bridge,"mcd2_fg_support")):nullptr;auto native=reinterpret_cast<ID3D12Device*>(device->get_native());if(native&&support){auto luid=native->GetAdapterLuid();available=support(&luid,sizeof(luid))==0?1:0;}}
 mcd2::fgui::Intent next{};if(!mcd2::fgui::decode(read(settingsPath),next)||!fresh(settingsPath)){intent.ready=0;wanted=false;publish(active,false);return;}
 intent=next;if(intent.mode!=previousMode){if(intent.mode)fault=0;previousMode=intent.mode;}
 if(seed.empty())seed=read(runtimePath);
 auto bootstrap=GetModuleHandleW(L"dxgi.dll");auto startup=bootstrap?reinterpret_cast<unsigned(*)()>(GetProcAddress(bootstrap,"mcd2_bootstrap_fg_session_mode")):nullptr;
 sessionMode=startup?startup():0;restartRequired=mcd2::fg::restartRequired(available,intent.mode,sessionMode,retired.load())?1:0;
 wanted=available&&sessionMode==1&&intent.mode&&intent.ready&&intent.session==session&&!fault&&!retired;
 publish(active,false);
}
}
