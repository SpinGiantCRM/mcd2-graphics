// Opt-in analytical AMD FG game bridge. No vendor code/runtime is bundled.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <atomic>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <map>
#include <mutex>
#include <new>
#include <vector>
#include "fsr_fg_game_bridge.h"
#include "../../src/native/reset_epoch_contract.hpp"
#include "../../src/latency/amd_fg_private_data.hpp"
#include "api/include/dx12/ffx_api_dx12.h"
#include "framegeneration/include/ffx_framegeneration.h"
#include "framegeneration/include/dx12/ffx_api_framegeneration_dx12.h"
namespace {
template<class T> void release(T*& p){if(p){p->Release();p=nullptr;}}
std::mutex guard;
// Returned swapchain COM interfaces execute vendor code even after the SDK
// context is destroyed. Keep one runtime module reference for process lifetime;
// context/resource retirement must not unload code beneath an external owner.
HMODULE retainedRuntime=nullptr;
std::atomic<unsigned> errors{0},warnings{0};
void debug(uint32_t type,const wchar_t*){if(type==FFX_API_MESSAGE_TYPE_ERROR)++errors;else ++warnings;}
struct Session {
 HMODULE module=nullptr;PfnFfxCreateContext create=nullptr;PfnFfxDestroyContext destroy=nullptr;
 PfnFfxQuery query=nullptr;PfnFfxDispatch dispatch=nullptr;PfnFfxConfigure configure=nullptr;
 ffxContext fg=nullptr,presenter=nullptr;IDXGISwapChain4* swap=nullptr;
 ID3D12Device* device=nullptr;ID3D12CommandQueue* queue=nullptr;ID3D12Resource* world=nullptr;
 bool worldInitialized=false,historyLost=true,preparedCurrent=false,imageCurrent=false;std::atomic<bool> closing{false};
 uint32_t width=0,height=0;uint64_t engine=UINT64_MAX,ordinal=0,prepared=0,images=0;
 std::atomic<uint64_t> real{0},generated{0};std::atomic<unsigned> fault{0},active{0};
 struct Recording {uint64_t epoch;std::vector<ID3D12Resource*> borrowed;};
 std::map<ID3D12GraphicsCommandList*,Recording> recordings;
 ID3D12Fence* retirementFence=nullptr;HANDLE retirementEvent=nullptr;bool retirementSignalled=false;
} *session=nullptr;
bool epoch(ID3D12GraphicsCommandList* cmd,uint64_t& value,bool arm=false){
 UINT size=sizeof(value);value=0;const auto hr=cmd->GetPrivateData(mcd2_reset_epoch_guid,&size,&value);
 if(hr==DXGI_ERROR_NOT_FOUND)return arm&&SUCCEEDED(cmd->SetPrivateData(mcd2_reset_epoch_guid,sizeof(value),&value));
 return SUCCEEDED(hr)&&size==sizeof(value);
}
void forget(Session::Recording& recording){for(auto* resource:recording.borrowed)resource->Release();}
void confirmRecordings(){
 auto& records=session->recordings;
 for(auto it=records.begin();it!=records.end();){uint64_t current=0;
  if(epoch(it->first,current)&&current!=it->second.epoch){forget(it->second);it->first->Release();it=records.erase(it);}else ++it;
 }
}
bool record(ID3D12GraphicsCommandList* cmd,std::initializer_list<ID3D12Resource*> borrowed){
 confirmRecordings();auto& records=session->recordings;uint64_t current=0;if(!epoch(cmd,current,true))return false;
 auto found=records.find(cmd);
 if(found==records.end()){if(records.size()>=256)return false;cmd->AddRef();found=records.emplace(cmd,Session::Recording{current,{}}).first;}
 for(auto* resource:borrowed)if(std::find(found->second.borrowed.begin(),found->second.borrowed.end(),resource)==found->second.borrowed.end()){resource->AddRef();found->second.borrowed.push_back(resource);}
 return true;
}
bool absolute(const wchar_t* p){return p&&((p[0]&&p[1]==L':'&&(p[2]==L'\\'||p[2]==L'/'))||(p[0]==L'\\'&&p[1]==L'\\'));}
bool hashRuntime(const wchar_t* path){
 constexpr unsigned char expected[]={0x02,0x29,0x7b,0xee,0xdd,0x28,0x5e,0x82,0x2d,0x3a,0x64,0xf3,0x14,0xcf,0x00,0xfa,0xf3,0x78,0xdc,0xec,0x0e,0xdc,0x47,0xff,0x0c,0x4d,0xd7,0x1b,0x3a,0x8c,0x2f,0x18};
 HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
 if(file==INVALID_HANDLE_VALUE)return false;
 BCRYPT_ALG_HANDLE algorithm=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;bool ok=false;
 if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0&&BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)>=0){
  auto bytes=std::make_unique<unsigned char[]>(65536);DWORD count=0;bool read=true;
  while((read=ReadFile(file,bytes.get(),65536,&count,nullptr))&&count){if(BCryptHashData(hash,bytes.get(),count,0)<0){read=false;break;}}
  unsigned char result[32]{};ok=read&&BCryptFinishHash(hash,result,32,0)>=0&&!memcmp(result,expected,32);
 }
 if(hash)BCryptDestroyHash(hash);if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);CloseHandle(file);return ok;
}
void transition(ID3D12GraphicsCommandList* cmd,ID3D12Resource* image,D3D12_RESOURCE_STATES from,D3D12_RESOURCE_STATES to){
 D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition={image,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,from,to};cmd->ResourceBarrier(1,&b);
}
bool sameDevice(ID3D12Device* owner);
bool texture(void* opaque,uint32_t w,uint32_t h,DXGI_FORMAT format,bool exact=false){
 if(!opaque)return false;auto* image=static_cast<ID3D12Resource*>(opaque);auto d=image->GetDesc();ID3D12Device* owner=nullptr;
 bool same=SUCCEEDED(image->GetDevice(IID_PPV_ARGS(&owner)))&&sameDevice(owner);release(owner);
 return same&&d.Dimension==D3D12_RESOURCE_DIMENSION_TEXTURE2D&&(exact?(d.Width==w&&d.Height==h&&d.MipLevels==1):(d.Width>=w&&d.Height>=h))&&d.DepthOrArraySize==1&&d.SampleDesc.Count==1&&d.Format==format;
}
IUnknown* identity(IUnknown* object){
 constexpr GUID unwrap={0x7f2c9a11,0x3b4e,0x4d6a,{0x81,0x2f,0x5e,0x9c,0xd3,0x7a,0x1b,0x42}};
 IUnknown* underlying=nullptr;IUnknown* canonical=nullptr;
 if(SUCCEEDED(object->QueryInterface(unwrap,reinterpret_cast<void**>(&underlying))))object=underlying;
 object->QueryInterface(IID_IUnknown,reinterpret_cast<void**>(&canonical));release(underlying);return canonical;
}
bool sameDevice(ID3D12Device* owner){if(!owner)return false;auto* a=identity(owner);auto* b=identity(session->device);const bool same=a&&a==b;release(a);release(b);return same;}
bool command(void* opaque){if(!opaque)return false;ID3D12Device* owner=nullptr;bool same=SUCCEEDED(static_cast<ID3D12GraphicsCommandList*>(opaque)->GetDevice(IID_PPV_ARGS(&owner)))&&sameDevice(owner);release(owner);return same;}
ffxReturnCode_t generate(ffxDispatchDescFrameGeneration* p,void* user){
 auto& s=*static_cast<Session*>(user);if(!p)return FFX_API_RETURN_ERROR_PARAMETER;
 const auto r=s.dispatch(&s.fg,&p->header);if(r)s.fault=unsigned(r);return r;
}
D3D12_RESOURCE_STATES state(uint32_t value){
 D3D12_RESOURCE_STATES r=D3D12_RESOURCE_STATE_COMMON;
 if(value&FFX_API_RESOURCE_STATE_UNORDERED_ACCESS)r|=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
 if(value&FFX_API_RESOURCE_STATE_COMPUTE_READ)r|=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
 if(value&FFX_API_RESOURCE_STATE_PIXEL_READ)r|=D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
 if(value&FFX_API_RESOURCE_STATE_COPY_SRC)r|=D3D12_RESOURCE_STATE_COPY_SOURCE;
 if(value&FFX_API_RESOURCE_STATE_COPY_DEST)r|=D3D12_RESOURCE_STATE_COPY_DEST;
 if(value&FFX_API_RESOURCE_STATE_RENDER_TARGET)r|=D3D12_RESOURCE_STATE_RENDER_TARGET;
 return r;
}
ffxReturnCode_t compose(ffxCallbackDescFrameGenerationPresent* p,void* user){
 auto& s=*static_cast<Session*>(user);if(!p||!p->commandList||!p->currentBackBuffer.resource||!p->outputSwapChainBuffer.resource){s.fault=1;return FFX_API_RETURN_ERROR_PARAMETER;}
 auto* cmd=static_cast<ID3D12GraphicsCommandList*>(p->commandList);auto* src=static_cast<ID3D12Resource*>(p->currentBackBuffer.resource);auto* dst=static_cast<ID3D12Resource*>(p->outputSwapChainBuffer.resource);
 const auto a=state(p->currentBackBuffer.state),b=state(p->outputSwapChainBuffer.state);
 if(src!=dst){transition(cmd,src,a,D3D12_RESOURCE_STATE_COPY_SOURCE);transition(cmd,dst,b,D3D12_RESOURCE_STATE_COPY_DEST);cmd->CopyResource(dst,src);transition(cmd,dst,D3D12_RESOURCE_STATE_COPY_DEST,b);transition(cmd,src,D3D12_RESOURCE_STATE_COPY_SOURCE,a);}
 if(p->isGeneratedFrame)++s.generated;else ++s.real;return FFX_API_RETURN_OK;
}
int configure(bool enabled){
 auto& s=*session;ffxConfigureDescFrameGeneration c{};c.header.type=FFX_API_CONFIGURE_DESC_TYPE_FRAMEGENERATION;
 c.swapChain=s.swap;c.frameGenerationEnabled=enabled;c.frameID=s.ordinal;c.generationRect={0,0,int32_t(s.width),int32_t(s.height)};
 c.presentCallback=compose;c.presentCallbackUserContext=&s;c.frameGenerationCallback=generate;c.frameGenerationCallbackUserContext=&s;
 if(s.worldInitialized)c.HUDLessColor=ffxApiGetResourceDX12(s.world,FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
 const auto r=s.configure(&s.fg,&c.header);s.active=!r&&enabled;if(r)s.fault=unsigned(r);return int(r);
}
}
extern "C" int mcd2_afg_load_v1(const wchar_t* path){
 std::lock_guard lock(guard);if(session)return -1;if(!absolute(path)||!hashRuntime(path))return -2;
 auto* s=new(std::nothrow) Session;if(!s)return -3;s->module=retainedRuntime?retainedRuntime:LoadLibraryExW(path,nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32);
 if(!s->module){delete s;return -4;}
 s->create=reinterpret_cast<PfnFfxCreateContext>(GetProcAddress(s->module,"ffxCreateContext"));s->destroy=reinterpret_cast<PfnFfxDestroyContext>(GetProcAddress(s->module,"ffxDestroyContext"));
 s->query=reinterpret_cast<PfnFfxQuery>(GetProcAddress(s->module,"ffxQuery"));s->dispatch=reinterpret_cast<PfnFfxDispatch>(GetProcAddress(s->module,"ffxDispatch"));s->configure=reinterpret_cast<PfnFfxConfigure>(GetProcAddress(s->module,"ffxConfigure"));
 if(!s->create||!s->destroy||!s->query||!s->dispatch||!s->configure){FreeLibrary(s->module);delete s;return -5;}
 retainedRuntime=s->module;errors=warnings=0;session=s;return 0;
}
extern "C" int mcd2_afg_support_v1(void* opaque){
 std::lock_guard lock(guard);if(!session||!opaque)return -1;
 auto* device=static_cast<ID3D12Device*>(opaque);
 D3D12_FEATURE_DATA_SHADER_MODEL shader{D3D_SHADER_MODEL_6_2};
 if(FAILED(device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL,&shader,sizeof(shader)))||shader.HighestShaderModel<D3D_SHADER_MODEL_6_2)return -2;
 ffxQueryDescGetVersions versions{};versions.header.type=FFX_API_QUERY_DESC_TYPE_GET_VERSIONS;
 versions.createDescType=FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATION;versions.device=device;
 uint64_t count=0;versions.outputCount=&count;
 if(session->query(nullptr,&versions.header)||!count||count>32)return -3;
 std::vector<uint64_t> ids(count);std::vector<const char*> names(count);
 versions.versionIds=ids.data();versions.versionNames=names.data();
 if(session->query(nullptr,&versions.header)||count>ids.size())return -4;
 for(size_t i=0;i<count;++i)if(ids[i]==17726168133342859270ull&&names[i]&&!strcmp(names[i],"3.1.6"))return 0;
 return -5;
}
extern "C" int mcd2_afg_swap_v1(void* factory,void* queue,void* hwnd,const MCD2AmdFgSwapV1* p,void** result){
 std::lock_guard lock(guard);
 if(!session)return -101;if(session->swap||session->fg||session->presenter)return -102;
 if(!factory||!queue||!hwnd||!result||*result||!p)return -103;
 if(p->size!=sizeof(*p))return -104;
 if(!p->width||!p->height||p->width>7680||p->height>4320||p->samples!=1||p->buffers<2||p->buffers>8)return -105;
 auto& s=*session;s.width=p->width;s.height=p->height;s.queue=static_cast<ID3D12CommandQueue*>(queue);s.queue->AddRef();
 auto hr=s.queue->GetDevice(IID_PPV_ARGS(&s.device));if(FAILED(hr))return int(hr);
 const auto format=DXGI_FORMAT(p->format);if(format!=DXGI_FORMAT_R16G16B16A16_FLOAT&&format!=DXGI_FORMAT_R10G10B10A2_UNORM)return -11;
 ffxQueryDescGetVersions versions{};versions.header.type=FFX_API_QUERY_DESC_TYPE_GET_VERSIONS;versions.createDescType=FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATION;versions.device=s.device;
 uint64_t count=0;versions.outputCount=&count;auto code=s.query(nullptr,&versions.header);if(code||!count||count>32)return -12;
 std::vector<uint64_t> ids(count);std::vector<const char*> names(count);versions.versionIds=ids.data();versions.versionNames=names.data();code=s.query(nullptr,&versions.header);if(code||count>ids.size())return -13;
 bool found=false;for(size_t i=0;i<count;++i)found|=ids[i]==17726168133342859270ull&&names[i]&&!strcmp(names[i],"3.1.6");if(!found)return -14;
 ffxOverrideVersion selected{};selected.header.type=FFX_API_DESC_TYPE_OVERRIDE_VERSION;selected.versionId=17726168133342859270ull;
 ffxCreateBackendDX12Desc backend{};backend.header.type=FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12;backend.device=s.device;backend.header.pNext=&selected.header;
 ffxCreateContextDescFrameGenerationVersion version{};version.header.type=FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATION_VERSION;version.version=FFX_FRAMEGENERATION_VERSION;version.header.pNext=&backend.header;
 ffxCreateContextDescFrameGenerationHudless hud{};hud.header.type=FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATION_HUDLESS;hud.hudlessBackBufferFormat=FFX_API_SURFACE_FORMAT_R10G10B10A2_UNORM;hud.header.pNext=&version.header;
 ffxCreateContextDescFrameGeneration create{};create.header.type=FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATION;create.header.pNext=&hud.header;
 create.displaySize={s.width,s.height};create.maxRenderSize=create.displaySize;create.backBufferFormat=ffxApiGetSurfaceFormatDX12(format);
 create.flags=FFX_FRAMEGENERATION_ENABLE_HIGH_DYNAMIC_RANGE|FFX_FRAMEGENERATION_ENABLE_DEPTH_INVERTED|FFX_FRAMEGENERATION_ENABLE_DEPTH_INFINITE|FFX_FRAMEGENERATION_ENABLE_DEBUG_CHECKING;
 code=s.create(&s.fg,&create.header,nullptr);if(code||!s.fg)return code?int(code):-15;
 ffxQueryGetProviderVersion active{};active.header.type=FFX_API_QUERY_DESC_TYPE_GET_PROVIDER_VERSION;code=s.query(&s.fg,&active.header);if(code||active.versionId!=selected.versionId)return -16;
 ffxConfigureDescGlobalDebug1 diagnostic{};diagnostic.header.type=FFX_API_CONFIGURE_DESC_TYPE_GLOBALDEBUG1;diagnostic.fpMessage=debug;code=s.configure(&s.fg,&diagnostic.header);if(code)return int(code);
 DXGI_SWAP_CHAIN_DESC1 desc{};desc.Width=p->width;desc.Height=p->height;desc.Format=format;desc.SampleDesc={p->samples,p->sampleQuality};desc.BufferUsage=p->usage;desc.BufferCount=p->buffers;desc.Scaling=DXGI_SCALING(p->scaling);desc.SwapEffect=DXGI_SWAP_EFFECT(p->effect);desc.AlphaMode=DXGI_ALPHA_MODE(p->alpha);desc.Flags=p->flags;
 DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreen{};fullscreen.Windowed=p->windowed;fullscreen.RefreshRate={p->refreshNumerator,p->refreshDenominator};fullscreen.ScanlineOrdering=DXGI_MODE_SCANLINE_ORDER(p->scanline);fullscreen.Scaling=DXGI_MODE_SCALING(p->modeScaling);
 ffxCreateContextDescFrameGenerationSwapChainVersionDX12 swapVersion{};swapVersion.header.type=FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_VERSION_DX12;swapVersion.version=FFX_FRAMEGENERATION_SWAPCHAIN_DX12_VERSION;
 ffxCreateContextDescFrameGenerationSwapChainForHwndDX12 swap{};swap.header.type=FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_FOR_HWND_DX12;swap.header.pNext=&swapVersion.header;swap.swapchain=&s.swap;swap.hwnd=static_cast<HWND>(hwnd);swap.desc=&desc;swap.fullscreenDesc=p->fullscreenProvided?&fullscreen:nullptr;swap.dxgiFactory=static_cast<IDXGIFactory*>(factory);swap.gameQueue=s.queue;
 code=s.create(&s.presenter,&swap.header,nullptr);if(code||!s.swap||!s.presenter)return code?int(code):-17;
 ffxQueryGetProviderVersion current{};current.header.type=FFX_API_QUERY_DESC_TYPE_GET_PROVIDER_VERSION;code=s.query(&s.presenter,&current.header);if(code||current.versionId!=17752306900579389447ull)return -18;
 D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;D3D12_RESOURCE_DESC texture{};texture.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;texture.Width=s.width;texture.Height=s.height;texture.DepthOrArraySize=texture.MipLevels=1;texture.SampleDesc.Count=1;texture.Format=DXGI_FORMAT_R10G10B10A2_UNORM;
 hr=s.device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&texture,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&s.world));if(FAILED(hr))return int(hr);
 code=configure(false);if(code)return int(code);
 return int(s.swap->QueryInterface(IID_IDXGISwapChain1,result));
}
extern "C" int mcd2_afg_guides_v1(void* cmd,void* depth,void* motion,const MCD2AmdFgGuidesV1* p){
 std::lock_guard lock(guard);if(!session||!session->fg||session->closing||session->fault||!p||p->size!=sizeof(*p)||p->reserved)return -20;
 auto& s=*session;const auto& c=p->camera;
 if(c.size!=sizeof(c))return -201;
 if(!c.width||!c.height||c.width>s.width||c.height>s.height)return -202;
 if(c.reset>1)return -203;if(!command(cmd))return -204;
 if(!texture(depth,c.width,c.height,DXGI_FORMAT_R32_FLOAT))return -205;
 if(!texture(motion,c.width,c.height,DXGI_FORMAT_R16G16_FLOAT))return -206;
 if(s.engine!=UINT64_MAX&&c.frame<=s.engine)return -22;
 const float values[]={p->frameTimeMs,p->worldToMeters,c.nearPlane,c.verticalFOV,c.jitter[0],c.jitter[1]};for(float v:values)if(!std::isfinite(v))return -23;
 if(p->frameTimeMs<=0||p->frameTimeMs>1000||p->worldToMeters<=0||c.nearPlane<=0||c.verticalFOV<=0||c.verticalFOV>=3.141593f)return -23;
 for(auto* vector:{c.position,c.up,c.right,c.forward})for(unsigned i=0;i<3;++i)if(!std::isfinite(vector[i]))return -23;
 const bool gap=s.engine==UINT64_MAX||uint64_t(c.frame)!=s.engine+1;s.engine=c.frame;++s.ordinal;s.preparedCurrent=s.imageCurrent=false;
 // Retain borrowed inputs even when a dispatch fails after recording commands.
 if(!record(static_cast<ID3D12GraphicsCommandList*>(cmd),{static_cast<ID3D12Resource*>(depth),static_cast<ID3D12Resource*>(motion)})){s.fault=61;return -61;}
 auto code=configure(true);if(code)return code;
 ffxDispatchDescFrameGenerationPrepareV2 d{};d.header.type=FFX_API_DISPATCH_DESC_TYPE_FRAMEGENERATION_PREPARE_V2;d.commandList=cmd;d.frameID=s.ordinal;d.renderSize={c.width,c.height};d.jitterOffset={c.jitter[0],c.jitter[1]};d.motionVectorScale={1,1};
 d.frameTimeDelta=p->frameTimeMs;d.reset=gap||s.historyLost||c.reset;d.cameraNear=c.nearPlane;d.cameraFar=c.farPlane;d.cameraFovAngleVertical=c.verticalFOV;d.viewSpaceToMetersFactor=p->worldToMeters;
 memcpy(d.cameraPosition,c.position,sizeof(c.position));memcpy(d.cameraUp,c.up,sizeof(c.up));memcpy(d.cameraRight,c.right,sizeof(c.right));memcpy(d.cameraForward,c.forward,sizeof(c.forward));
 d.depth=ffxApiGetResourceDX12(static_cast<ID3D12Resource*>(depth),FFX_API_RESOURCE_STATE_COMPUTE_READ);d.motionVectors=ffxApiGetResourceDX12(static_cast<ID3D12Resource*>(motion),FFX_API_RESOURCE_STATE_COMPUTE_READ);
 code=s.dispatch(&s.fg,&d.header);if(code){s.fault=unsigned(code);s.historyLost=true;return int(code);}s.preparedCurrent=true;++s.prepared;return 0;
}
extern "C" int mcd2_afg_world_v1(void* cmd,void* world,uint64_t id){
 std::lock_guard lock(guard);if(!session||session->closing||!session->preparedCurrent||id!=session->engine||!command(cmd)||!texture(world,session->width,session->height,DXGI_FORMAT_R10G10B10A2_UNORM,true))return -30;
 auto& s=*session;auto* list=static_cast<ID3D12GraphicsCommandList*>(cmd);auto* src=static_cast<ID3D12Resource*>(world);
 if(!record(list,{src})){s.fault=61;return -61;}
 constexpr auto read=D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
 if(s.worldInitialized)transition(list,s.world,read,D3D12_RESOURCE_STATE_COPY_DEST);
 transition(list,src,read,D3D12_RESOURCE_STATE_COPY_SOURCE);list->CopyResource(s.world,src);transition(list,src,D3D12_RESOURCE_STATE_COPY_SOURCE,read);transition(list,s.world,D3D12_RESOURCE_STATE_COPY_DEST,read);
 s.worldInitialized=s.imageCurrent=true;++s.images;return 0;
}
extern "C" int mcd2_afg_present_v1(uint64_t id,uint32_t wanted){
 std::lock_guard lock(guard);if(!session||!session->fg||session->closing||wanted>1)return -40;
 auto& s=*session;const bool enable=wanted&&!s.fault&&id==s.engine&&s.preparedCurrent&&s.imageCurrent;
 s.historyLost=!enable;return configure(enable);
}
extern "C" int mcd2_afg_state_v1(MCD2AmdFgStateV1* out){
 std::lock_guard lock(guard);if(!session||!out||out->size!=sizeof(*out))return -50;auto& s=*session;
 *out={sizeof(*out),s.swap&&s.fg&&s.presenter?1u:0u,s.active.load(),s.fault.load(),errors.load(),warnings.load(),s.engine,s.ordinal,s.prepared,s.images,s.real.load(),s.generated.load()};return 0;
}
extern "C" int mcd2_afg_antilag_ready_v1(void* device){
 std::lock_guard lock(guard);
 return session&&session->swap&&session->presenter&&!session->closing&&
        sameDevice(static_cast<ID3D12Device*>(device))?0:-1;
}
extern "C" int mcd2_afg_antilag_v1(void* device,void* context,uint32_t enabled,uint32_t drain){
 std::lock_guard lock(guard);if(enabled>1||drain>1||(enabled&&!context)||(drain&&(context||enabled)))return E_INVALIDARG;
 // A destroyed presenter cannot make further callbacks. Clearing is idempotent.
 if(!session||!session->swap||!session->presenter)return !context&&!enabled?0:E_NOINTERFACE;
 if(!sameDevice(static_cast<ID3D12Device*>(device)))return E_INVALIDARG;
 if(session->closing&&context)return E_ABORT;
 auto result=mcd2::amd_fg::publish(session->swap,context,enabled!=0);if(FAILED(result))return int(result);
 if(drain){
  ffxDispatchDescFrameGenerationSwapChainWaitForPresentsDX12 wait{};
  wait.header.type=FFX_API_DISPATCH_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_WAIT_FOR_PRESENTS_DX12;
  return int(session->dispatch(&session->presenter,&wait.header));
 }
 return 0;
}
extern "C" int mcd2_afg_retire_v1(void* fence,uint64_t value){
 std::lock_guard lock(guard);if(!session)return 0;auto& s=*session;
 // ReShade can retire temporary startup devices before the game creates its
 // world swapchain. No presenter/recording exists to retire in that case.
 if(!s.fg&&!s.presenter&&!s.queue)return 0;
 if(s.fg&&!s.closing){const auto r=configure(false);if(r)return r;s.closing=true;}
 confirmRecordings();if(!s.recordings.empty())return -61;
 // An externally supplied completed fence alone cannot prove its signal occurred
 // after invalidation. Always signal our own queue after the epoch check above.
 if(fence){const auto completed=static_cast<ID3D12Fence*>(fence)->GetCompletedValue();if(!value||completed==UINT64_MAX||completed<value)return -60;}
 if(s.queue&&!s.retirementSignalled){
  if(!s.retirementFence&&FAILED(s.device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&s.retirementFence))))return -62;
  if(FAILED(s.queue->Signal(s.retirementFence,1)))return -62;s.retirementSignalled=true;
 }
 if(s.retirementFence){auto completed=s.retirementFence->GetCompletedValue();if(completed==UINT64_MAX)return -62;
  if(completed<1){if(!s.retirementEvent)s.retirementEvent=CreateEventW(nullptr,FALSE,FALSE,nullptr);
   if(!s.retirementEvent||FAILED(s.retirementFence->SetEventOnCompletion(1,s.retirementEvent))||WaitForSingleObject(s.retirementEvent,5000)!=WAIT_OBJECT_0)return -62;
  }
 }
 if(s.presenter){ffxDispatchDescFrameGenerationSwapChainWaitForPresentsDX12 wait{};wait.header.type=FFX_API_DISPATCH_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_WAIT_FOR_PRESENTS_DX12;auto r=s.dispatch(&s.presenter,&wait.header);if(r)return int(r);}
 if(s.fg){auto r=s.destroy(&s.fg,nullptr);if(r)return int(r);s.fg=nullptr;}
 if(s.presenter){auto r=s.destroy(&s.presenter,nullptr);if(r)return int(r);s.presenter=nullptr;}
 release(s.world);release(s.swap);release(s.retirementFence);if(s.retirementEvent)CloseHandle(s.retirementEvent);release(s.queue);release(s.device);delete session;session=nullptr;return 0;
}
