// Compiled with the Microsoft C++ ABI; only a plain C interface crosses to the host.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <wintrust.h>
#include <softpub.h>
#include <d3d12.h>
#include <sl.h>
#include <sl_reflex.h>
#include <sl_pcl.h>
#include <sl_dlss_g.h>
#include <atomic>
#include <filesystem>
#include "fg_bridge_contract.h"
#include "fg_camera_contract.h"
namespace {
HMODULE dll=nullptr;bool initialized=false;
void* boundDevice=nullptr;
bool fgConfigured=false;
PFun_slInit*init=nullptr;PFun_slShutdown*shutdown=nullptr;
PFun_slGetFeatureFunction*feature=nullptr;PFun_slSetD3DDevice*setDevice=nullptr;PFun_slGetNewFrameToken*newToken=nullptr;
PFun_slReflexGetState*getState=nullptr;PFun_slReflexSetOptions*setOptions=nullptr;PFun_slReflexSleep*sleepFrame=nullptr;PFun_slPCLSetMarker*markFrame=nullptr;
PFun_slPCLGetState*getPclState=nullptr;
template<class T>bool bind(T*&out,const char*name){out=reinterpret_cast<T*>(GetProcAddress(dll,name));return out!=nullptr;}
template<class T>int bindFeature(T*&out,sl::Feature id,const char*name){void*p=nullptr;auto r=feature(id,name,p);out=reinterpret_cast<T*>(p);return int(r);}
}
extern "C" {
int mcd2_fg_initialized(){return initialized?1:0;}
long mcd2_sl_verify(const wchar_t*path){
 WINTRUST_FILE_INFO file{};file.cbStruct=sizeof(file);file.pcwszFilePath=path;
 WINTRUST_DATA trust{};trust.cbStruct=sizeof(trust);trust.dwUIChoice=WTD_UI_NONE;trust.fdwRevocationChecks=WTD_REVOKE_NONE;trust.dwUnionChoice=WTD_CHOICE_FILE;trust.pFile=&file;trust.dwStateAction=WTD_STATEACTION_VERIFY;trust.dwProvFlags=WTD_CACHE_ONLY_URL_RETRIEVAL;
 GUID action=WINTRUST_ACTION_GENERIC_VERIFY_V2;auto r=WinVerifyTrust(nullptr,&action,&trust);trust.dwStateAction=WTD_STATEACTION_CLOSE;WinVerifyTrust(nullptr,&action,&trust);return r;
}
int mcd2_sl_init(const wchar_t*path,const wchar_t*folder,const wchar_t*logFolder){
 if(initialized||dll)return -1;if(mcd2_sl_verify(path)!=0)return -2;
 dll=LoadLibraryExW(path,nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);if(!dll)return -3;
 if(!bind(init,"slInit")||!bind(shutdown,"slShutdown")||!bind(feature,"slGetFeatureFunction")||!bind(setDevice,"slSetD3DDevice")||!bind(newToken,"slGetNewFrameToken"))return -4;
 // NGX is shared per device. The first initializer must retain the existing
 // SR feature-library path as well as FG; no vendor file is copied or replaced.
 const auto srFolder=(std::filesystem::path(folder)/L"MCD2Graphics"/L"ngx-runtime").wstring();
 const bool hasSR=GetFileAttributesW((std::filesystem::path(srFolder)/L"nvngx_dlss.dll").c_str())!=INVALID_FILE_ATTRIBUTES;
 sl::Feature features[]={sl::kFeatureReflex,sl::kFeaturePCL,sl::kFeatureDLSS_G};const wchar_t*paths[]={folder,srFolder.c_str()};sl::Preferences p{};p.pathsToPlugins=paths;p.numPathsToPlugins=hasSR?2:1;p.pathToLogsAndData=logFolder;p.featuresToLoad=features;p.numFeaturesToLoad=3;p.engine=sl::EngineType::eCustom;p.engineVersion="MCD2Graphics-FG-Experiment";p.projectId="2d523377-ba23-4a19-8161-ab7ed2152777";p.flags=sl::PreferenceFlags::eUseManualHooking|sl::PreferenceFlags::eDisableDebugText|sl::PreferenceFlags::eUseFrameBasedResourceTagging;
 auto r=init(p,sl::kSDKVersion);if(r!=sl::Result::eOk)return int(r);initialized=true;
 return 0;
}
int mcd2_sl_set_device(void*device){
 if(!initialized||!device)return -1;
 // SR initializes NGX on ReShade's device wrapper. NGX is shared: the first
 // initializer must use that same wrapper, or its native descriptor heaps
 // can later be consumed by wrapped SR commands. Never synthesize a wrapper.
 constexpr GUID proxyId={0x2523aff4,0x978b,0x4939,{0xba,0x16,0x8e,0xe8,0x76,0xa4,0xcb,0x2a}};
 auto actual=static_cast<ID3D12Device*>(device);ID3D12Device*proxy=nullptr;UINT bytes=sizeof(proxy);
 if(SUCCEEDED(actual->GetPrivateData(proxyId,&bytes,&proxy))&&proxy&&bytes==sizeof(proxy))device=proxy;
 if(boundDevice==device)return 0;
 if(boundDevice)return -20; // Device recreation requires an explicit owner lifecycle.
 auto r=setDevice(device);if(r!=sl::Result::eOk)return int(r);
 // Feature exports become available only after the graphics device initializes the plugins.
 if((r=sl::Result(bindFeature(getState,sl::kFeatureReflex,"slReflexGetState")))!=sl::Result::eOk)return int(r);
 if((r=sl::Result(bindFeature(setOptions,sl::kFeatureReflex,"slReflexSetOptions")))!=sl::Result::eOk)return int(r);
 if((r=sl::Result(bindFeature(sleepFrame,sl::kFeatureReflex,"slReflexSleep")))!=sl::Result::eOk)return int(r);
 if((r=sl::Result(bindFeature(markFrame,sl::kFeaturePCL,"slPCLSetMarker")))!=sl::Result::eOk)return int(r);
 auto result=bindFeature(getPclState,sl::kFeaturePCL,"slPCLGetState");if(!result)boundDevice=device;return result;
}
int mcd2_sl_mode_limit(unsigned mode,unsigned limitUs){if(!initialized||!setOptions||mode>2)return -1;sl::ReflexOptions p{};p.mode=sl::ReflexMode(mode);p.frameLimitUs=limitUs;p.useMarkersToOptimize=false;return int(setOptions(p));}
int mcd2_sl_mode(unsigned mode){return mcd2_sl_mode_limit(mode,0);}
int mcd2_sl_begin(unsigned index,void**out){if(!initialized||!newToken||!out)return -1;sl::FrameToken*token=nullptr;auto r=newToken(token,&index);*out=token;return int(r);}
unsigned mcd2_sl_index(void*token){return uint32_t(*static_cast<sl::FrameToken*>(token));}
int mcd2_sl_sleep(void*token){return sleepFrame?int(sleepFrame(*static_cast<sl::FrameToken*>(token))):-1;}
int mcd2_sl_marker(void*token,unsigned marker){return markFrame?int(markFrame(sl::PCLMarker(marker),*static_cast<sl::FrameToken*>(token))):-1;}
int mcd2_sl_state(unsigned*available,unsigned*valid,unsigned*complete){if(!getState)return -1;sl::ReflexState s{};auto r=getState(s);*available=s.lowLatencyAvailable;*valid=s.latencyReportAvailable;*complete=0;for(auto&f:s.frameReport)if(f.simStartTime&&f.simEndTime&&f.renderSubmitStartTime&&f.renderSubmitEndTime&&f.presentStartTime&&f.presentEndTime)++*complete;return int(r);}
// Plain C diagnostics prevent opaque SDK structures crossing the ABI boundary.
int mcd2_sl_report_range(uint64_t*first,uint64_t*last,uint64_t*sim,uint64_t*present){
 if(!getState||!first||!last||!sim||!present)return -1;
 sl::ReflexState s{};auto r=getState(s);*first=*last=*sim=*present=0;
 if(r!=sl::Result::eOk||!s.latencyReportAvailable)return int(r);
 for(auto&f:s.frameReport){if(!f.simStartTime||!f.simEndTime||!f.renderSubmitStartTime||!f.renderSubmitEndTime||!f.presentStartTime||!f.presentEndTime)continue;
  if(!*first||f.frameID<*first)*first=f.frameID;
  if(f.frameID>*last){*last=f.frameID;*sim=f.simStartTime;*present=f.presentEndTime;}
 }return 0;
}
int mcd2_sl_pcl_message(unsigned*message){if(!getPclState||!message)return -1;sl::PCLState s{};auto r=getPclState(s);*message=s.statsWindowMessage;return int(r);}
int mcd2_sl_shutdown(){int r=initialized?int(shutdown()):0;if(r)return r;initialized=false;boundDevice=nullptr;fgConfigured=false;getState=nullptr;getPclState=nullptr;setOptions=nullptr;sleepFrame=nullptr;markFrame=nullptr;init=nullptr;shutdown=nullptr;feature=nullptr;setDevice=nullptr;newToken=nullptr;return r;}
}

// Isolated presentation experiment only. Never linked into the released addon.
extern "C" int mcd2_fg_support(void* luid,unsigned bytes){
 PFun_slIsFeatureSupported* supported=nullptr;
 if(!initialized||!luid||!bytes||!bind(supported,"slIsFeatureSupported"))return -1;
 sl::AdapterInfo a{};a.deviceLUID=static_cast<uint8_t*>(luid);a.deviceLUIDSizeInBytes=bytes;
 return int(supported(sl::kFeatureDLSS_G,a));
}
extern "C" int mcd2_fg_upgrade(void** object){
 PFun_slUpgradeInterface* upgrade=nullptr;
 if(!initialized||!object||!*object||!bind(upgrade,"slUpgradeInterface"))return -1;
 return int(upgrade(object));
}
extern "C" int mcd2_fg_state(unsigned* maximum,unsigned* status,unsigned* minimum,unsigned* presented){
 PFun_slDLSSGGetState* get=nullptr;
 auto r=bindFeature(get,sl::kFeatureDLSS_G,"slDLSSGGetState");if(r||!get)return r?r:-1;
 sl::DLSSGState s{};
 auto result=get(sl::ViewportHandle(0),s,nullptr);
 *maximum=s.numFramesToGenerateMax;*status=unsigned(s.status);*minimum=s.minWidthOrHeight;*presented=s.numFramesActuallyPresented;
 return int(result);
}

// Release only after proxy COM interfaces have been destroyed.
extern "C" void mcd2_fg_unload(){if(!initialized && dll){FreeLibrary(dll);dll=nullptr;}}

namespace {
std::atomic<long> lastApiError=0;
void apiError(const sl::APIError& e){lastApiError=e.hres;}
void identity(sl::float4x4& m){for(unsigned i=0;i<4;++i)m[i]=sl::float4(i==0?1.f:0.f,i==1?1.f:0.f,i==2?1.f:0.f,i==3?1.f:0.f);}
}
extern "C" long mcd2_fg_api_error(){return lastApiError.load();}
extern "C" int mcd2_fg_configured(){return fgConfigured?1:0;}
extern "C" int mcd2_fg_configure(const MCD2FGConfig* config){
 if(!initialized||!config||config->size!=sizeof(*config)||config->mode>1||!config->width||!config->height||!config->motionWidth||!config->motionHeight||!config->backBuffers||config->backBuffers>16||config->generatedFrames!=1)return -1;
 PFun_slDLSSGSetOptions* set=nullptr;auto r=bindFeature(set,sl::kFeatureDLSS_G,"slDLSSGSetOptions");if(r||!set)return r?r:-1;
 sl::DLSSGOptions o{};o.mode=config->mode?sl::DLSSGMode::eOn:sl::DLSSGMode::eOff;o.numFramesToGenerate=config->generatedFrames;
 o.numBackBuffers=config->backBuffers;o.colorWidth=config->width;o.colorHeight=config->height;o.mvecDepthWidth=config->motionWidth;o.mvecDepthHeight=config->motionHeight;
 o.colorBufferFormat=o.hudLessBufferFormat=config->colorFormat;o.uiBufferFormat=config->uiFormat;
 o.depthBufferFormat=config->depthFormat;o.mvecBufferFormat=config->motionFormat;
 o.onErrorCallback=apiError;
 auto result=set(sl::ViewportHandle(0),o);if(result==sl::Result::eOk)fgConfigured=true;return int(result);
}
extern "C" int mcd2_fg_mode(unsigned mode,unsigned width,unsigned height){
 const bool hdr=GetEnvironmentVariableW(L"MCD2_FG_HDR",nullptr,0)!=0;
 const MCD2FGConfig c{sizeof(MCD2FGConfig),mode,width,height,width,height,hdr?24u:28u,hdr?10u:28u,41,34,3,1};
 return mcd2_fg_configure(&c);
}
// Game inputs use measured camera data and the actual engine frame token.
// Transient inputs are copied by SL into its own resources at this command.
extern "C" int mcd2_fg_game_guides(void* token,void* depth,void* motion,void* cmd,const MCD2FGCamera* camera){
 if(!initialized||!token||!depth||!motion||!cmd||!camera||camera->size!=sizeof(*camera)||!camera->width||!camera->height||mcd2_sl_index(token)!=camera->frame)return -1;
 PFun_slSetConstants* constants=nullptr;PFun_slSetTagForFrame* tags=nullptr;
 if(!bind(constants,"slSetConstants")||!bind(tags,"slSetTagForFrame"))return -2;
 sl::Constants c{};
 auto matrix=[](sl::float4x4& dst,const float* src){for(unsigned r=0;r<4;++r)dst[r]={src[r*4],src[r*4+1],src[r*4+2],src[r*4+3]};};
 matrix(c.cameraViewToClip,camera->viewToClip);matrix(c.clipToCameraView,camera->clipToView);matrix(c.clipToPrevClip,camera->clipToPrevious);matrix(c.prevClipToClip,camera->previousToClip);identity(c.clipToLensClip);
 c.jitterOffset={camera->jitter[0],camera->jitter[1]};c.mvecScale={1.f/camera->width,1.f/camera->height};c.cameraPinholeOffset={0,0};
 auto vec=[](const float* v){return sl::float3{v[0],v[1],v[2]};};c.cameraPos=vec(camera->position);c.cameraUp=vec(camera->up);c.cameraRight=vec(camera->right);c.cameraFwd=vec(camera->forward);
 c.cameraNear=camera->nearPlane;c.cameraFar=camera->farPlane;c.cameraFOV=camera->verticalFOV;c.cameraAspectRatio=camera->aspectRatio;
 c.depthInverted=sl::eTrue;c.cameraMotionIncluded=sl::eTrue;c.motionVectors3D=sl::eFalse;c.reset=camera->reset?sl::eTrue:sl::eFalse;c.orthographicProjection=sl::eFalse;c.motionVectorsDilated=sl::eFalse;c.motionVectorsJittered=sl::eFalse;
 auto r=constants(c,*static_cast<sl::FrameToken*>(token),sl::ViewportHandle(0));if(r!=sl::Result::eOk)return int(r);
 sl::Resource d(sl::ResourceType::eTex2d,depth,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),m(sl::ResourceType::eTex2d,motion,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
 sl::Extent extent{0,0,camera->width,camera->height};sl::ResourceTag inputs[]={
  {&d,sl::kBufferTypeDepth,sl::ResourceLifecycle::eOnlyValidNow,&extent},
  {&m,sl::kBufferTypeMotionVectors,sl::ResourceLifecycle::eOnlyValidNow,&extent}};
 return int(tags(*static_cast<sl::FrameToken*>(token),sl::ViewportHandle(0),inputs,2,cmd));
}
extern "C" int mcd2_fg_game_images(void*token,void*world,void*alpha,void*cmd,unsigned width,unsigned height){
 if(!initialized||!token||!world||!alpha||!cmd||!width||!height)return -1;
 PFun_slSetTagForFrame*tags=nullptr;if(!bind(tags,"slSetTagForFrame"))return -2;
 sl::Resource w(sl::ResourceType::eTex2d,world,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),u(sl::ResourceType::eTex2d,alpha,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
 sl::Extent extent{0,0,width,height};sl::ResourceTag inputs[]={
  {&w,sl::kBufferTypeHUDLessColor,sl::ResourceLifecycle::eOnlyValidNow,&extent},
  {&u,sl::kBufferTypeUIAlpha,sl::ResourceLifecycle::eOnlyValidNow,&extent}};
 return int(tags(*static_cast<sl::FrameToken*>(token),sl::ViewportHandle(0),inputs,2,cmd));
}
// Static, entirely owned scene: zero motion is correct, alpha-zero UI is correct.
// This gate tests provider execution/presentation, not MCD2 image quality.
extern "C" int mcd2_fg_inputs(void* token,void* world,void* depth,void* motion,void* ui,void* cmd,unsigned width,unsigned height,unsigned reset){
 if(!initialized||!token||!world||!depth||!motion||!ui||!cmd||!width||!height)return -1;
 PFun_slSetConstants* constants=nullptr;PFun_slSetTagForFrame* tags=nullptr;
 if(!bind(constants,"slSetConstants")||!bind(tags,"slSetTagForFrame"))return -1;
 sl::Constants c{};identity(c.cameraViewToClip);identity(c.clipToCameraView);identity(c.clipToLensClip);identity(c.clipToPrevClip);identity(c.prevClipToClip);
 const float aspect=float(width)/float(height),sy=1.7320508075688772f,sx=sy/aspect,a=100.f/99.9f,b=-.1f*a;
 c.cameraViewToClip[0]={sx,0,0,0};c.cameraViewToClip[1]={0,sy,0,0};c.cameraViewToClip[2]={0,0,a,1};c.cameraViewToClip[3]={0,0,b,0};
 c.clipToCameraView[0]={1/sx,0,0,0};c.clipToCameraView[1]={0,1/sy,0,0};c.clipToCameraView[2]={0,0,0,1/b};c.clipToCameraView[3]={0,0,1,-a/b};
 c.jitterOffset={0,0};c.mvecScale={1,1};c.cameraPinholeOffset={0,0};c.cameraPos={0,0,0};c.cameraUp={0,1,0};c.cameraRight={1,0,0};c.cameraFwd={0,0,1};
 c.cameraNear=.1f;c.cameraFar=100;c.cameraFOV=1.0471975512f;c.cameraAspectRatio=aspect;
 c.depthInverted=sl::eFalse;c.cameraMotionIncluded=sl::eTrue;c.motionVectors3D=sl::eFalse;c.reset=reset?sl::eTrue:sl::eFalse;
 auto r=constants(c,*static_cast<sl::FrameToken*>(token),sl::ViewportHandle(0));if(r!=sl::Result::eOk)return int(r);
 constexpr unsigned shaderRead=0xc0; // D3D12 PIXEL | NON_PIXEL_SHADER_RESOURCE
 sl::Resource w(sl::ResourceType::eTex2d,world,shaderRead),d(sl::ResourceType::eTex2d,depth,shaderRead),m(sl::ResourceType::eTex2d,motion,shaderRead),u(sl::ResourceType::eTex2d,ui,shaderRead);
 sl::Extent extent{0,0,width,height};sl::ResourceTag input[]={
  {&d,sl::kBufferTypeDepth,sl::ResourceLifecycle::eValidUntilPresent,&extent},
  {&m,sl::kBufferTypeMotionVectors,sl::ResourceLifecycle::eValidUntilPresent,&extent},
  {&w,sl::kBufferTypeHUDLessColor,sl::ResourceLifecycle::eValidUntilPresent,&extent},
  {&u,sl::kBufferTypeUIColorAndAlpha,sl::ResourceLifecycle::eValidUntilPresent,&extent}
 };
 return int(tags(*static_cast<sl::FrameToken*>(token),sl::ViewportHandle(0),input,4,cmd));
}
