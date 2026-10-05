// Compiled with the Microsoft C++ ABI; only a plain C interface crosses to the host.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <wintrust.h>
#include <softpub.h>
#include <sl.h>
#include <sl_reflex.h>
#include <sl_pcl.h>
namespace {
HMODULE dll=nullptr;bool initialized=false;
PFun_slInit*init=nullptr;PFun_slShutdown*shutdown=nullptr;
PFun_slGetFeatureFunction*feature=nullptr;PFun_slSetD3DDevice*setDevice=nullptr;PFun_slGetNewFrameToken*newToken=nullptr;
PFun_slReflexGetState*getState=nullptr;PFun_slReflexSetOptions*setOptions=nullptr;PFun_slReflexSleep*sleepFrame=nullptr;PFun_slPCLSetMarker*markFrame=nullptr;
PFun_slPCLGetState*getPclState=nullptr;
template<class T>bool bind(T*&out,const char*name){out=reinterpret_cast<T*>(GetProcAddress(dll,name));return out!=nullptr;}
template<class T>int bindFeature(T*&out,sl::Feature id,const char*name){void*p=nullptr;auto r=feature(id,name,p);out=reinterpret_cast<T*>(p);return int(r);}
}
extern "C" {
long mcd2_sl_verify(const wchar_t*path){
 WINTRUST_FILE_INFO file{};file.cbStruct=sizeof(file);file.pcwszFilePath=path;
 WINTRUST_DATA trust{};trust.cbStruct=sizeof(trust);trust.dwUIChoice=WTD_UI_NONE;trust.fdwRevocationChecks=WTD_REVOKE_NONE;trust.dwUnionChoice=WTD_CHOICE_FILE;trust.pFile=&file;trust.dwStateAction=WTD_STATEACTION_VERIFY;trust.dwProvFlags=WTD_CACHE_ONLY_URL_RETRIEVAL;
 GUID action=WINTRUST_ACTION_GENERIC_VERIFY_V2;auto r=WinVerifyTrust(nullptr,&action,&trust);trust.dwStateAction=WTD_STATEACTION_CLOSE;WinVerifyTrust(nullptr,&action,&trust);return r;
}
int mcd2_sl_init(const wchar_t*path,const wchar_t*folder,const wchar_t*logFolder){
 if(initialized||dll)return -1;if(mcd2_sl_verify(path)!=0)return -2;
 dll=LoadLibraryExW(path,nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);if(!dll)return -3;
 if(!bind(init,"slInit")||!bind(shutdown,"slShutdown")||!bind(feature,"slGetFeatureFunction")||!bind(setDevice,"slSetD3DDevice")||!bind(newToken,"slGetNewFrameToken"))return -4;
 sl::Feature features[]={sl::kFeatureReflex,sl::kFeaturePCL};const wchar_t*paths[]={folder};sl::Preferences p{};p.pathsToPlugins=paths;p.numPathsToPlugins=1;p.pathToLogsAndData=logFolder;p.featuresToLoad=features;p.numFeaturesToLoad=2;p.engine=sl::EngineType::eCustom;p.engineVersion="MCD2Graphics-0.2";p.flags=sl::PreferenceFlags::eUseManualHooking|sl::PreferenceFlags::eDisableDebugText|sl::PreferenceFlags::eUseFrameBasedResourceTagging;
 auto r=init(p,sl::kSDKVersion);if(r!=sl::Result::eOk)return int(r);initialized=true;
 return 0;
}
int mcd2_sl_set_device(void*device){
 if(!initialized||!device)return -1;auto r=setDevice(device);if(r!=sl::Result::eOk)return int(r);
 // Feature exports become available only after the graphics device initializes the plugins.
 if((r=sl::Result(bindFeature(getState,sl::kFeatureReflex,"slReflexGetState")))!=sl::Result::eOk)return int(r);
 if((r=sl::Result(bindFeature(setOptions,sl::kFeatureReflex,"slReflexSetOptions")))!=sl::Result::eOk)return int(r);
 if((r=sl::Result(bindFeature(sleepFrame,sl::kFeatureReflex,"slReflexSleep")))!=sl::Result::eOk)return int(r);
 if((r=sl::Result(bindFeature(markFrame,sl::kFeaturePCL,"slPCLSetMarker")))!=sl::Result::eOk)return int(r);
 return bindFeature(getPclState,sl::kFeaturePCL,"slPCLGetState");
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
int mcd2_sl_shutdown(){int r=initialized?int(shutdown()):0;initialized=false;getState=nullptr;getPclState=nullptr;setOptions=nullptr;sleepFrame=nullptr;markFrame=nullptr;init=nullptr;shutdown=nullptr;feature=nullptr;setDevice=nullptr;newToken=nullptr;if(dll){FreeLibrary(dll);dll=nullptr;}return r;}
}
