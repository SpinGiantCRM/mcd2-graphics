// Bounded developer-only CPU sampling. Does not submit or change GPU resources.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <reshade.hpp>
#include <array>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include "fsr_fg_game_bridge.h"
namespace {
namespace fs=std::filesystem;
std::mutex mutex;fs::path root;
std::array<LONGLONG,1200> counters{};
unsigned count=0,requested=0;std::string label;
struct Outcome {MCD2AmdFgStateV1 amd{};int result=-1;bool module=false;};
Outcome before;
Outcome outcome(){
 Outcome out;auto module=GetModuleHandleW(L"mcd2-fsr-fg-game-bridge.dll");out.module=module!=nullptr;
 auto get=module?reinterpret_cast<decltype(&mcd2_afg_state_v1)>(GetProcAddress(module,"mcd2_afg_state_v1")):nullptr;
 out.amd.size=sizeof(out.amd);out.result=get?get(&out.amd):-1;return out;
}
void write_outcome(std::ostream& out,const char* name,const Outcome& v){
 out<<"{\"kind\":\""<<name<<"\",\"module\":"<<(v.module?"true":"false")<<",\"result\":"<<v.result
 <<",\"active\":"<<v.amd.active<<",\"ready\":"<<v.amd.ready<<",\"fault\":"<<v.amd.fault
 <<",\"errors\":"<<v.amd.errors<<",\"warnings\":"<<v.amd.warnings
 <<",\"engineFrame\":"<<v.amd.engineFrame<<",\"providerFrame\":"<<v.amd.providerFrame
 <<",\"prepared\":"<<v.amd.prepared<<",\"images\":"<<v.amd.images
 <<",\"realPresents\":"<<v.amd.realPresents<<",\"generatedPresents\":"<<v.amd.generatedPresents<<"}\n";
}
void finish(reshade::api::command_queue*,reshade::api::swapchain*){
 std::lock_guard lock(mutex);
 if(root.empty()){
  auto local=std::make_unique<wchar_t[]>(32768);auto n=GetEnvironmentVariableW(L"LOCALAPPDATA",local.get(),32768);
  if(!n||n>=32768)return;root=fs::path(local.get())/L"Dungeons2"/L"Saved"/L"MCD2ProviderBenchmark";
  std::error_code ec;fs::create_directories(root,ec);if(ec){root.clear();return;}
 }
 if(requested){
  if(!count)before=outcome();
  LARGE_INTEGER counter{};QueryPerformanceCounter(&counter);counters[count++]=counter.QuadPart;
  if(count==requested){
   const auto after=outcome();LARGE_INTEGER frequency{};QueryPerformanceFrequency(&frequency);
   std::ofstream out(root/(label+".jsonl"));
   out<<"{\"kind\":\"scope\",\"qpcFrequency\":"<<frequency.QuadPart
      <<",\"applicationPresentIntervals\":true,\"gpuQueries\":false,\"pixelReadbacks\":false,\"physicalDisplayFrames\":false}\n";
   write_outcome(out,"before",before);write_outcome(out,"after",after);
   for(unsigned n=0;n<count;++n)out<<"{\"kind\":\"sample\",\"counter\":"<<counters[n]<<"}\n";
   requested=count=0;
  }
  return;
 }
 static ULONGLONG last=0;auto now=GetTickCount64();if(now-last<500)return;last=now;
 auto file=root/L"request.txt";std::error_code ec;if(!fs::exists(file,ec))return;
 unsigned n=0;std::ifstream in(file);in>>label>>n;in.close();fs::remove(file,ec);
 if(label.empty()||label.size()>48||label.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789-")!=std::string::npos||
    n<30||n>counters.size()||fs::exists(root/(label+".jsonl")))return;
 requested=n;count=0;
}
}
extern "C" __declspec(dllexport) const char* NAME="MCD2 provider CPU sampler (temporary)";
extern "C" __declspec(dllexport) const char* DESCRIPTION="Bounded application intervals and AMD SDK counters; no physical-FPS claim";
BOOL APIENTRY DllMain(HMODULE module,DWORD reason,LPVOID){
 if(reason==DLL_PROCESS_ATTACH){if(!reshade::register_addon(module))return FALSE;reshade::register_event<reshade::addon_event::finish_present>(finish);}
 else if(reason==DLL_PROCESS_DETACH)reshade::unregister_addon(module);
 return TRUE;
}
