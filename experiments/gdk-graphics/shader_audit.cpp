// Read-only shader identity probe; not an installer or release payload.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <reshade.hpp>
#include "../../src/latency/process_exit.hpp"
#include <array>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <set>

namespace {
namespace a=reshade::api;
struct State {
    std::ofstream log;std::set<std::uint32_t> seen;std::filesystem::path shaders;
    std::size_t capturedBytes=0;
};
State* state=nullptr;std::mutex mutex;
#if defined(MCD2_SHADER_AUDIT_PIXEL)
constexpr bool pixelAudit=true;
#else
constexpr bool pixelAudit=false;
#endif
std::uint32_t crc(const void* data,std::size_t size){
    static const auto table=[](){std::array<std::uint32_t,256> next{};for(unsigned i=0;i<256;++i){auto value=i;for(unsigned bit=0;bit<8;++bit)value=(value>>1)^(0xEDB88320u&(0u-(value&1u)));next[i]=value;}return next;}();
    auto value=~0u;auto bytes=static_cast<const std::uint8_t*>(data);
    for(std::size_t index=0;index<size;++index)value=(value>>8)^table[(value^bytes[index])&255u];return ~value;
}
void initPipeline(a::device*,a::pipeline_layout,unsigned count,const a::pipeline_subobject* sub,a::pipeline){
    if(mcd2::process_exit::terminating())return;
    std::lock_guard lock(mutex);if(!state||state->seen.size()>=(pixelAudit?16384u:2048u))return;
    for(unsigned index=0;index<count;++index){
        if(sub[index].type!=a::pipeline_subobject_type::compute_shader&&sub[index].type!=a::pipeline_subobject_type::pixel_shader)continue;
        const auto& shader=*static_cast<const a::shader_desc*>(sub[index].data);
        const bool compute=sub[index].type==a::pipeline_subobject_type::compute_shader;
        if(pixelAudit&&compute)continue;
        if(!shader.code||!shader.code_size||shader.code_size>16*1024*1024)continue;
        const auto hash=crc(shader.code,shader.code_size);if(!state->seen.insert(hash).second)continue;
        bool captured=false;
        if((pixelAudit||compute)&&shader.code_size<=2*1024*1024&&state->capturedBytes+shader.code_size<=(pixelAudit?256u:64u)*1024*1024){
            std::ostringstream filename;filename<<std::hex<<std::setw(8)<<std::setfill('0')<<hash<<".dxil";
            std::ofstream file(state->shaders/filename.str(),std::ios::binary);
            file.write(static_cast<const char*>(shader.code),shader.code_size);file.flush();captured=bool(file);
            if(captured)state->capturedBytes+=shader.code_size;
        }
        auto& out=state->log;out<<"{\"crc\":"<<hash<<",\"compute\":"<<(compute?"true":"false")<<",\"bytes\":"<<shader.code_size<<",\"captured\":"<<(captured?"true":"false")<<"}\n";out.flush();
    }
}
}
extern "C" __declspec(dllexport) const char* NAME="MCD2 GDK shader identity audit";
extern "C" __declspec(dllexport) const char* DESCRIPTION="Read-only shader capture for offline analysis; no shaders or resources are replaced";
BOOL APIENTRY DllMain(HMODULE module,DWORD reason,LPVOID reserved){
    if(reason==DLL_PROCESS_ATTACH){if(!reshade::register_addon(module))return FALSE;reshade::register_event<reshade::addon_event::init_pipeline>(initPipeline);}
    else if(reason==DLL_PROCESS_DETACH){if(reserved)mcd2::process_exit::mark_terminating();else reshade::unregister_addon(module);}return TRUE;
}
extern "C" __declspec(dllexport) bool AddonInit(HMODULE module,HMODULE){
    mcd2::process_exit::initialize();
    try{
        if(crc("123456789",9)!=0xcbf43926u)return false;
        auto filename=std::make_unique<wchar_t[]>(32768),local=std::make_unique<wchar_t[]>(32768);
        const auto fileLength=GetModuleFileNameW(module,filename.get(),32768),localLength=GetEnvironmentVariableW(L"LOCALAPPDATA",local.get(),32768);
        if(!fileLength||fileLength>=32768||!localLength||localLength>=32768)return false;
        auto next=std::make_unique<State>();const auto folder=std::filesystem::path(local.get())/L"Dungeons2"/L"Saved"/L"MCD2Graphics";
        next->shaders=folder/(pixelAudit?L"private-gdk-pixel-shaders":L"private-gdk-shaders");std::filesystem::create_directories(next->shaders);
        next->log.open(folder/(pixelAudit?L"gdk-pixel-shader-audit.jsonl":L"gdk-shader-audit.jsonl"));
        if(!next->log)return false;
        std::lock_guard lock(mutex);state=next.release();return true;
    }catch(const std::exception&){return false;}
}
extern "C" __declspec(dllexport) void AddonUninit(HMODULE,HMODULE){
    if(mcd2::process_exit::terminating())return;
    std::lock_guard lock(mutex);delete state;state=nullptr;
}
