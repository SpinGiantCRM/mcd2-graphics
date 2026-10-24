#pragma once
#include "display_protocol.hpp"
#include "../providers/runtime_diagnostics.h"
#include "../providers/mfg_component_projection.hpp"
#include "../../experiments/providers/mfg_game_bridge.h"
#include "../providers/runtime_diagnostics_projection.hpp"
namespace mcd2::diagnostics {
inline unsigned counter(std::uint64_t value){return unsigned(value&0x7fffffff);}
inline void publish(const std::filesystem::path& saves,unsigned session,unsigned revision,MCD2DiagnosticIdentityV1 identity,const SrWorkerSample& srWorker,unsigned reflexMode,std::uint64_t reflexFrames,unsigned amdRevision,unsigned amdChecksum,unsigned amdMode,std::uint64_t amdFrames,const std::filesystem::path& ini){
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
 const int srResult=get?get(&sr):-3;
 if(srResult)sr={sizeof(sr),0,0,0,0,0,0};
 static SrMetadataCache srMetadata;srMetadata.observe(identity,srResult,sr,GetTickCount64());
 MCD2FgDiagnosticsV1 fg{sizeof(fg),0,0,0,0,0,0,0};
 module=GetModuleHandleW(L"mcd2-fg-guide-recon.addon64");using Fg=int(*)(MCD2FgDiagnosticsV1*);
 auto getFg=module?reinterpret_cast<Fg>(GetProcAddress(module,"mcd2_fg_diagnostics_v1")):nullptr;
 if(!getFg||getFg(&fg))fg={sizeof(fg),0,0,0,0,0,0,0};
 MCD2FgDiagnosticsV2 attributed{};attributed.size=sizeof(attributed);
 auto getAttributed=module?reinterpret_cast<int(*)(MCD2FgDiagnosticsV2*)>(GetProcAddress(module,"mcd2_fg_diagnostics_v2")):nullptr;
 const int fgResult=getAttributed?getAttributed(&attributed):-3;
 static FgWorkerCache fgSamples;const auto fgReadAt=GetTickCount64();
 fgSamples.observe(identity,fgResult,fgResult==0?&attributed:nullptr,fgReadAt,fgReadAt);attributed=fgSamples.snapshot();
 // The V1 getter supplies separately verified metadata/DLSS activity. FSR
 // activity comes only from the already decoded runtime sample in this worker.
 const auto projected=project(identity,sr,attributed,amdRevision,amdChecksum,amdMode,amdFrames,GetTickCount64(),&srWorker);
 MfgComponents components{};
 auto mfg=GetModuleHandleW(L"mcd2-mfg-game-bridge.dll");auto mfgState=mfg?reinterpret_cast<int(*)(MCD2MfgStateV1*)>(GetProcAddress(mfg,"mcd2_mfg_state_v1")):nullptr;
 MCD2MfgStateV1 live{};live.size=sizeof(live);
 if(mfgState&&!mfgState(&live)&&!live.reserved)components=mfgComponents(identity,{live.identity,live.observedAtMs,live.strategy,live.multiplier,live.active,live.fault,live.softwarePresented,live.nativePresented},GetTickCount64());
 std::map<std::string,unsigned> fields={{"SchemaVersion",1},{"SessionId",session},{"Revision",revision},{"Sample",++sample},
  {"WorldGeneration",projected.world},{"SrProvider",projected.sr.provider},{"SrFrames",counter(projected.sr.evaluations)},{"FsrSrVersion",sr.implementation},{"FsrHardware",sr.capabilities},
  {"FgStrategy",components.strategy},{"FgSoftwareGenerated",components.software},{"FgNativeGenerated",components.native},{"FgProvider",projected.fg.provider},{"FgActive",projected.fg.active},{"FgFault",projected.fg.fault},{"FgRendered",counter(projected.fg.rendered)},{"FgGenerated",counter(projected.fg.generated)},
  {"FsrFgVersion",fg.implementation},{"FgPresetStatus",GetPrivateProfileIntW(L"Runtime",L"FrameGenerationPresetStatus",0,ini.c_str())},{"ReflexMode",valid(identity)?reflexMode:0},{"ReflexFrames",valid(identity)?counter(reflexFrames):0},{"AmdMode",projected.amdMode},{"AmdFrames",counter(projected.amdFrames)}};
 const auto path=saves/L"MCD2GraphicsDiagnosticsState.sav";auto seed=read(path);std::map<std::string,unsigned> old;
 if(!display::parse(seed,"DiagnosticsStateSave",old,header))return;
 // Reuse the bounded named-int serializer with an explicit class identity.
 auto bytes=display::encode_runtime(seed,fields,"DiagnosticsStateSave");if(bytes.empty())return;
 const auto pending=saves/L"MCD2GraphicsDiagnosticsState.pending";
 {std::ofstream file(pending,std::ios::binary|std::ios::trunc);file.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());file.flush();if(!file)return;}
 MoveFileExW(pending.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);
}
}
