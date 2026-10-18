#pragma once
#include "display_protocol.hpp"
#include "../providers/runtime_diagnostics.h"
namespace mcd2::diagnostics {
inline unsigned counter(std::uint64_t value){return unsigned(value&0x7fffffff);}
inline void publish(const std::filesystem::path& saves,unsigned session,unsigned revision,unsigned reflexMode,std::uint64_t reflexFrames,unsigned amdMode,std::uint64_t amdFrames,const std::filesystem::path& ini){
 static ULONGLONG next=0;static unsigned sample=0;
 const auto now=GetTickCount64();if(now<next)return;next=now+500;
 auto read=[](const std::filesystem::path& path){std::error_code error;const auto size=std::filesystem::file_size(path,error);if(error||size>8192)return std::vector<std::uint8_t>{};std::ifstream file(path,std::ios::binary);return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(file),{});};
 std::map<std::string,unsigned> request;std::size_t header=0;
 if(!display::parse(read(saves/L"MCD2GraphicsDiagnosticsRequest.sav"),"DiagnosticsRequestSave",request,header)||
    request.size()>3||request["SessionId"]!=session||request["Enabled"]!=1)return;
 for(const auto& [key,value]:request)if(key!="SchemaVersion"&&key!="SessionId"&&key!="Enabled")return;
 MCD2SrDiagnosticsV1 sr{sizeof(sr),0,0,0,0,0,0};
 auto module=GetModuleHandleW(L"mcd2-graphics.addon64");
 using Sr=int(*)(MCD2SrDiagnosticsV1*);auto get=module?reinterpret_cast<Sr>(GetProcAddress(module,"mcd2_sr_diagnostics_v1")):nullptr;
 if(!get||get(&sr))sr={sizeof(sr),0,0,0,0,0,0};
 else if(sr.revision!=revision){sr.provider=0;sr.evaluations=0;sr.world=0;}
 MCD2FgDiagnosticsV1 fg{sizeof(fg),0,0,0,0,0,0,0};
 module=GetModuleHandleW(L"mcd2-fg-guide-recon.addon64");using Fg=int(*)(MCD2FgDiagnosticsV1*);
 auto getFg=module?reinterpret_cast<Fg>(GetProcAddress(module,"mcd2_fg_diagnostics_v1")):nullptr;
 if(!getFg||getFg(&fg))fg={sizeof(fg),0,0,0,0,0,0,0};
 std::map<std::string,unsigned> fields={{"SchemaVersion",1},{"SessionId",session},{"Revision",revision},{"Sample",++sample},
  {"WorldGeneration",sr.world},{"SrProvider",sr.provider},{"SrFrames",counter(sr.evaluations)},{"FsrSrVersion",sr.implementation},
  {"FgProvider",fg.provider},{"FgActive",fg.active},{"FgFault",fg.fault},{"FgRendered",counter(fg.rendered)},{"FgGenerated",counter(fg.generated)},
  {"FsrFgVersion",fg.implementation},{"FgPresetStatus",GetPrivateProfileIntW(L"Runtime",L"FrameGenerationPresetStatus",0,ini.c_str())},{"ReflexMode",reflexMode},{"ReflexFrames",counter(reflexFrames)},{"AmdMode",amdMode},{"AmdFrames",counter(amdFrames)}};
 const auto path=saves/L"MCD2GraphicsDiagnosticsState.sav";auto seed=read(path);std::map<std::string,unsigned> old;
 if(!display::parse(seed,"DiagnosticsStateSave",old,header))return;
 // Reuse the bounded named-int serializer with an explicit class identity.
 auto bytes=display::encode_runtime(seed,fields,"DiagnosticsStateSave");if(bytes.empty())return;
 const auto pending=saves/L"MCD2GraphicsDiagnosticsState.pending";
 {std::ofstream file(pending,std::ios::binary|std::ios::trunc);file.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());file.flush();if(!file)return;}
 MoveFileExW(pending.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);
}
}
