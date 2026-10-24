#pragma once
#include <windows.h>
#include <d3d12.h>
#include "mfg_ngx_parameters.hpp"

// Isolated compute experiment, never a game proxy. The process lifetime owns
// the external module; no live unload is attempted on an unqualified backend.
struct MfgHeadless {
    HMODULE module{};
    NVSDK_NGX_Handle* feature{};
    using Init=NVSDK_NGX_Result (NVSDK_CONV*)(unsigned long long,const wchar_t*,ID3D12Device*,NVSDK_NGX_Version,const NVSDK_NGX_FeatureCommonInfo*);
    using Populate=NVSDK_NGX_Result (NVSDK_CONV*)(NVSDK_NGX_Parameter*);
    using Scratch=NVSDK_NGX_Result (NVSDK_CONV*)(NVSDK_NGX_Feature,const NVSDK_NGX_Parameter*,size_t*);
    using Create=NVSDK_NGX_Result (NVSDK_CONV*)(ID3D12GraphicsCommandList*,NVSDK_NGX_Feature,NVSDK_NGX_Parameter*,NVSDK_NGX_Handle**);
    using Evaluate=NVSDK_NGX_Result (NVSDK_CONV*)(ID3D12GraphicsCommandList*,const NVSDK_NGX_Handle*,NVSDK_NGX_Parameter*,PFN_NVSDK_NGX_ProgressCallback);
    using Release=NVSDK_NGX_Result (NVSDK_CONV*)(NVSDK_NGX_Handle*);
    using Shutdown=NVSDK_NGX_Result (NVSDK_CONV*)(ID3D12Device*);
    Init init{}; Populate populate{}; Scratch scratch{}; Create create{};
    Evaluate evaluate{}; Release release{}; Shutdown shutdown{};
    NVSDK_NGX_Result evaluate_recording(ID3D12GraphicsCommandList* cmd,NVSDK_NGX_Parameter* params) {
        // This probe owns an already-open command list and its submission.
        // Omitting this flag lets the backend Reset/Close that same list.
        params->Set("DLSSG.IsRecording",1u);
        return evaluate(cmd,feature,params,nullptr);
    }
    bool load(const wchar_t* path) {
        module=LoadLibraryExW(path,nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32);
        if(!module) return false;
#define MCD2_MFG_EXPORT(member,type,suffix) member=reinterpret_cast<type>(GetProcAddress(module,"DLSSG_NVSDK_NGX_D3D12_" suffix)); if(!member) return false
        MCD2_MFG_EXPORT(init,Init,"Init_Ext");
        MCD2_MFG_EXPORT(populate,Populate,"PopulateParameters_Impl");
        MCD2_MFG_EXPORT(scratch,Scratch,"GetScratchBufferSize");
        MCD2_MFG_EXPORT(create,Create,"CreateFeature");
        MCD2_MFG_EXPORT(evaluate,Evaluate,"EvaluateFeature");
        MCD2_MFG_EXPORT(release,Release,"ReleaseFeature");
        MCD2_MFG_EXPORT(shutdown,Shutdown,"Shutdown1");
#undef MCD2_MFG_EXPORT
        return true;
    }
};
