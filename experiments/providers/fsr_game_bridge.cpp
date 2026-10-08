// Experimental analytical FSR SR bridge; does not install or fetch vendor DLLs.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <atomic>
#include <cmath>
#include <cstring>
#include <new>
#include <vector>
#include "fsr_game_bridge.h"
#include "api/include/dx12/ffx_api_dx12.h"
#include "upscalers/include/ffx_upscale.h"

namespace {
std::atomic<uint32_t> errors{0},warnings{0};
void message(uint32_t type,const wchar_t*) {
 if(type==FFX_API_MESSAGE_TYPE_ERROR)++errors;else ++warnings;
}
struct Session {
 HMODULE module=nullptr;ffxContext context=nullptr;
 PfnFfxCreateContext create=nullptr;PfnFfxDestroyContext destroy=nullptr;
 PfnFfxQuery query=nullptr;PfnFfxDispatch dispatch=nullptr;
 uint32_t rw=0,rh=0,ow=0,oh=0;uint64_t calls=0,provider=0,required=0,optional=0;
 bool recorded=false;uint32_t stage=0;
};
bool dimensions(uint32_t w,uint32_t h){return w&&h&&w<=3840&&h<=2160;}
bool texture(void *p,uint32_t w,uint32_t h,DXGI_FORMAT format=DXGI_FORMAT_UNKNOWN) {
 if(!p)return false;auto d=static_cast<ID3D12Resource*>(p)->GetDesc();
 return d.Dimension==D3D12_RESOURCE_DIMENSION_TEXTURE2D&&d.Width==w&&d.Height==h
  &&d.DepthOrArraySize==1&&d.SampleDesc.Count==1&&(format==DXGI_FORMAT_UNKNOWN||d.Format==format);
}
}
extern "C" int mcd2_fsr_create_v1(void *device,const wchar_t *runtime,
 uint32_t rw,uint32_t rh,uint32_t ow,uint32_t oh,void **out) {
 if(!out||*out||!device||!runtime||!dimensions(rw,rh)||!dimensions(ow,oh)||ow<rw||oh<rh)return -1001;
 // The experiment controller verifies pinned bytes and requires an absolute path.
 // Never fall back to a DLL basename or a current-directory search.
 if(!((runtime[0]&&runtime[1]==L':'&&(runtime[2]==L'\\'||runtime[2]==L'/'))
  ||(runtime[0]==L'\\'&&runtime[1]==L'\\')))return -1002;
 auto *s=new(std::nothrow) Session;if(!s)return -1003;
 s->module=LoadLibraryExW(runtime,nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32);
 if(!s->module){delete s;return -1004;}
 s->create=reinterpret_cast<PfnFfxCreateContext>(GetProcAddress(s->module,"ffxCreateContext"));
 s->destroy=reinterpret_cast<PfnFfxDestroyContext>(GetProcAddress(s->module,"ffxDestroyContext"));
 s->query=reinterpret_cast<PfnFfxQuery>(GetProcAddress(s->module,"ffxQuery"));
 s->dispatch=reinterpret_cast<PfnFfxDispatch>(GetProcAddress(s->module,"ffxDispatch"));
 if(!s->create||!s->destroy||!s->query||!s->dispatch){FreeLibrary(s->module);delete s;return -1005;}
 // From here the caller owns the session even on an SDK error. A context handle
 // returned on failure must never be abandoned or destroyed twice.
 *out=s;s->rw=rw;s->rh=rh;s->ow=ow;s->oh=oh;s->stage=1;
 ffxQueryDescGetVersions versions{};versions.header.type=FFX_API_QUERY_DESC_TYPE_GET_VERSIONS;
 versions.createDescType=FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE;versions.device=device;
 uint64_t count=0;versions.outputCount=&count;auto code=s->query(nullptr,&versions.header);
 if(code)return int(code);if(!count||count>32)return -1006;
 std::vector<uint64_t> ids(count);std::vector<const char*> names(count);
 versions.versionIds=ids.data();versions.versionNames=names.data();code=s->query(nullptr,&versions.header);
 if(code)return int(code);if(count>ids.size())return -1006;
 for(size_t i=0;i<count;++i)if(names[i]&&std::strstr(names[i],"3.1.5"))s->provider=ids[i];
 if(!s->provider)return -1007;s->stage=2;
 ffxOverrideVersion selected{};selected.header.type=FFX_API_DESC_TYPE_OVERRIDE_VERSION;selected.versionId=s->provider;
 ffxCreateBackendDX12Desc backend{};backend.header.type=FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12;
 backend.device=static_cast<ID3D12Device*>(device);backend.header.pNext=&selected.header;
 ffxCreateContextDescUpscale create{};create.header.type=FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE;
 create.header.pNext=&backend.header;create.maxRenderSize={rw,rh};create.maxUpscaleSize={ow,oh};
 create.flags=FFX_UPSCALE_ENABLE_HIGH_DYNAMIC_RANGE|FFX_UPSCALE_ENABLE_DEPTH_INVERTED
  |FFX_UPSCALE_ENABLE_DEPTH_INFINITE|FFX_UPSCALE_ENABLE_DEBUG_CHECKING;create.fpMessage=message;
 code=s->create(&s->context,&create.header,nullptr);if(code)return int(code);if(!s->context)return -1008;s->stage=3;
 ffxQueryGetProviderVersion active{};active.header.type=FFX_API_QUERY_DESC_TYPE_GET_PROVIDER_VERSION;
 code=s->query(&s->context,&active.header);if(code)return int(code);if(active.versionId!=s->provider)return -1009;s->stage=4;
 ffxQueryDescUpscaleGetResourceRequirements inputs{};inputs.header.type=FFX_API_QUERY_DESC_TYPE_UPSCALE_GET_RESOURCE_REQUIREMENTS;
 code=s->query(&s->context,&inputs.header);if(code)return int(code);
 s->required=inputs.required_resources;s->optional=inputs.optional_resources;
 constexpr uint64_t supplied=FFX_API_QUERY_RESOURCE_INPUT_COLOR|FFX_API_QUERY_RESOURCE_INPUT_DEPTH
  |FFX_API_QUERY_RESOURCE_INPUT_MV|FFX_API_QUERY_RESOURCE_INPUT_EXPOSURE;
 if(s->required&~supplied)return -1010;s->stage=5;
 return 0;
}
extern "C" int mcd2_fsr_dispatch_v1(void *opaque,void *cmd,void *colour,void *depth,
 void *motion,void *exposure,void *output,const MCD2FsrDispatchV1 *p) {
 auto *s=static_cast<Session*>(opaque);
 if(!s||!s->context||!cmd||!p||p->size!=sizeof(*p)||p->reset>1
  ||p->renderWidth!=s->rw||p->renderHeight!=s->rh||p->outputWidth!=s->ow||p->outputHeight!=s->oh)return -1011;
 const float values[]={p->jitterX,p->jitterY,p->motionScaleX,p->motionScaleY,p->frameTimeMs,
  p->preExposure,p->cameraNear,p->verticalFov,p->viewSpaceToMeters};
 for(float v:values)if(!std::isfinite(v))return -1012;
 if(p->preExposure<=0||p->frameTimeMs<=0||p->frameTimeMs>1000||p->cameraNear<=0
  ||!std::isinf(p->cameraFar)||p->cameraFar<0||p->verticalFov<=0||p->verticalFov>=3.1415927f||p->viewSpaceToMeters<=0)return -1012;
 if(!texture(colour,s->rw,s->rh)||!texture(depth,s->rw,s->rh,DXGI_FORMAT_R32_FLOAT)
  ||!texture(motion,s->rw,s->rh,DXGI_FORMAT_R16G16_FLOAT)||!texture(exposure,1,1,DXGI_FORMAT_R32_FLOAT)
  ||!texture(output,s->ow,s->oh,DXGI_FORMAT_R16G16B16A16_FLOAT))return -1013;
 ffxDispatchDescUpscale d{};d.header.type=FFX_API_DISPATCH_DESC_TYPE_UPSCALE;d.commandList=cmd;
 d.color=ffxApiGetResourceDX12(static_cast<ID3D12Resource*>(colour));
 d.depth=ffxApiGetResourceDX12(static_cast<ID3D12Resource*>(depth));
 d.motionVectors=ffxApiGetResourceDX12(static_cast<ID3D12Resource*>(motion));
 d.exposure=ffxApiGetResourceDX12(static_cast<ID3D12Resource*>(exposure));
 d.output=ffxApiGetResourceDX12(static_cast<ID3D12Resource*>(output),FFX_API_RESOURCE_STATE_UNORDERED_ACCESS);
 d.jitterOffset={p->jitterX,p->jitterY};d.motionVectorScale={p->motionScaleX,p->motionScaleY};
 d.renderSize={s->rw,s->rh};d.upscaleSize={s->ow,s->oh};d.frameTimeDelta=p->frameTimeMs;
 d.preExposure=p->preExposure;d.reset=p->reset!=0;d.cameraNear=p->cameraNear;d.cameraFar=p->cameraFar;
 d.cameraFovAngleVertical=p->verticalFov;d.viewSpaceToMetersFactor=p->viewSpaceToMeters;
 s->recorded=true;auto code=s->dispatch(&s->context,&d.header);if(!code)++s->calls;return int(code);
}
extern "C" int mcd2_fsr_info_v1(void *opaque,MCD2FsrInfoV1 *p) {
 auto *s=static_cast<Session*>(opaque);if(!s||!p||p->size!=sizeof(*p))return -1014;
 p->errors=errors.load();p->warnings=warnings.load();p->reserved=s->stage;p->providerId=s->provider;
 p->requiredResources=s->required;p->optionalResources=s->optional;p->dispatches=s->calls;return 0;
}
extern "C" int mcd2_fsr_quality_v1(void *opaque,MCD2FsrQualityV1 *p) {
 auto *s=static_cast<Session*>(opaque);
 if(!s||!s->context||s->stage!=5||!p||p->size!=sizeof(*p)||p->qualityMode>4
  ||!dimensions(p->displayWidth,p->displayHeight))return -1017;
 uint32_t rw=0,rh=0;float ratio=0;
 ffxQueryDescUpscaleGetUpscaleRatioFromQualityMode scale{};
 scale.header.type=FFX_API_QUERY_DESC_TYPE_UPSCALE_GETUPSCALERATIOFROMQUALITYMODE;
 scale.qualityMode=p->qualityMode;scale.pOutUpscaleRatio=&ratio;
 auto code=s->query(&s->context,&scale.header);if(code)return int(code);
 ffxQueryDescUpscaleGetRenderResolutionFromQualityMode render{};
 render.header.type=FFX_API_QUERY_DESC_TYPE_UPSCALE_GETRENDERRESOLUTIONFROMQUALITYMODE;
 render.displayWidth=p->displayWidth;render.displayHeight=p->displayHeight;
 render.qualityMode=p->qualityMode;render.pOutRenderWidth=&rw;render.pOutRenderHeight=&rh;
 code=s->query(&s->context,&render.header);if(code)return int(code);
 if(!std::isfinite(ratio)||ratio<1||!dimensions(rw,rh)
  ||rw>p->displayWidth||rh>p->displayHeight)return -1018;
 p->renderWidth=rw;p->renderHeight=rh;p->upscaleRatio=ratio;p->reserved=0;p->providerId=s->provider;
 return 0;
}
extern "C" int mcd2_fsr_destroy_v1(void *opaque,void *fence,uint64_t required) {
 auto *s=static_cast<Session*>(opaque);if(!s)return -1015;
 if(s->recorded){if(!fence||!required)return -1016;
  const auto completed=static_cast<ID3D12Fence*>(fence)->GetCompletedValue();
  if(completed==UINT64_MAX||completed<required)return -1016;
 }
 if(s->context){auto code=s->destroy(&s->context,nullptr);if(code)return int(code);s->context=nullptr;}
 FreeLibrary(s->module);delete s;return 0;
}
