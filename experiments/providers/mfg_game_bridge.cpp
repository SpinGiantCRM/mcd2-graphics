#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <memory>
#include <mutex>
#include <vector>
#include "mfg_game_bridge.h"
#include "../../third-party/AntiLag2/ffx_antilag2_dx12.h"
#include "mfg_game_swapchain.hpp"
#include "mfg_headless.hpp"
#include "mfg_game_parameters.hpp"
#include "mfg_game_guides.hpp"
#include "mfg_game_timeline.hpp"
#include "../../src/providers/fg_output_contract.hpp"
#include "../../src/providers/mfg_game_transaction.hpp"
#include "../../src/latency/process_exit.hpp"
namespace {
using namespace mcd2::mfg;
constexpr auto Read=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
constexpr char SoftwareHash[]="f131734e9bf1fd191a9f60fad54231cd88e45ada3c957bd02c8763f8e9ad4440";
// qualification/providers/linux-dev25/dependencies-2026-10-24.lock.json
constexpr char NativeHash[]="8d59d49b36410dd6b62fef2503a5871d347415bd85d943fa0848563a95dd50c7";
template<class T> void drop(T*& p){if(p){p->Release();p=nullptr;}}
IUnknown* canonical(IUnknown* object){if(!object)return nullptr;constexpr GUID unwrap={0x7f2c9a11,0x3b4e,0x4d6a,{0x81,0x2f,0x5e,0x9c,0xd3,0x7a,0x1b,0x42}};IUnknown* raw{},*result{};
 if(SUCCEEDED(object->QueryInterface(unwrap,reinterpret_cast<void**>(&raw)))&&raw)object=raw;object->QueryInterface(IID_PPV_ARGS(&result));drop(raw);return result;}
template<class T> T* nativeObject(T* object){auto* id=canonical(object);T* result{};if(id){id->QueryInterface(IID_PPV_ARGS(&result));id->Release();}return result;}
bool same(IUnknown* a,IUnknown* b){auto* x=canonical(a);auto* y=canonical(b);bool result=x&&y&&x==y;drop(x);drop(y);return result;}
HANDLE verify(const wchar_t* name,const char* expected){
 if(!name||!std::filesystem::path(name).is_absolute())return nullptr;
 auto file=CreateFileW(name,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
 if(file==INVALID_HANDLE_VALUE)return nullptr;
 BCRYPT_ALG_HANDLE alg{};BCRYPT_HASH_HANDLE hash{};DWORD size{},copied{};
 bool ok=BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
 ok=ok&&BCryptGetProperty(alg,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&size),sizeof(size),&copied,0)>=0;
 std::vector<unsigned char> object(size),buffer(65536);
 ok=ok&&BCryptCreateHash(alg,&hash,object.data(),size,nullptr,0,0)>=0;
 while(ok){DWORD bytes{};ok=ReadFile(file,buffer.data(),DWORD(buffer.size()),&bytes,nullptr)!=0;if(!ok||!bytes)break;ok=BCryptHashData(hash,buffer.data(),bytes,0)>=0;}
 unsigned char digest[32]{};char hex[65]{};ok=ok&&BCryptFinishHash(hash,digest,32,0)>=0;
 if(hash)BCryptDestroyHash(hash);if(alg)BCryptCloseAlgorithmProvider(alg,0);
 for(unsigned i=0;i<32;++i)snprintf(hex+2*i,3,"%02x",digest[i]);
 if(!ok||strcmp(hex,expected)){CloseHandle(file);return nullptr;}return file;
}
struct Native {
 HMODULE module{},featureModule{};NVSDK_NGX_Handle* feature{};NVSDK_NGX_Parameter* params{};
 using Init=NVSDK_NGX_Result(*)(const char*,NVSDK_NGX_EngineType,const char*,const wchar_t*,ID3D12Device*,NVSDK_NGX_Version,const NVSDK_NGX_FeatureCommonInfo*);
 using Allocate=NVSDK_NGX_Result(*)(NVSDK_NGX_Parameter**);
 using Destroy=NVSDK_NGX_Result(*)(NVSDK_NGX_Parameter*);
 Init init{};Allocate populate{},allocate{};Destroy destroy{};
 MfgHeadless::Create create{};MfgHeadless::Evaluate evaluate{};MfgHeadless::Release release{};
 std::wstring directory;
 bool load(const wchar_t* path){
  directory=std::filesystem::path(path).parent_path().wstring();
  // Pin the exact feature module before the driver resolves it. Never use a
  // proxy beside the application for the shared NGX driver entry points.
  featureModule=LoadLibraryExW(path,nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32);
  if(!featureModule)return false;
  std::vector<wchar_t> actual(32768);auto length=GetModuleFileNameW(featureModule,actual.data(),DWORD(actual.size()));
  if(!length||length>=actual.size())return false;auto held=verify(actual.data(),NativeHash);if(!held)return false;
  // Keep both verified identities locked until process exit.
  module=LoadLibraryExW(L"nvngx.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);if(!module)return false;
#define GET(member,type,name) member=reinterpret_cast<type>(GetProcAddress(module,"NVSDK_NGX_D3D12_" name));if(!member)return false
  GET(init,Init,"Init_ProjectID");GET(populate,Allocate,"GetCapabilityParameters");GET(allocate,Allocate,"AllocateParameters");GET(destroy,Destroy,"DestroyParameters");
  GET(create,MfgHeadless::Create,"CreateFeature");GET(evaluate,MfgHeadless::Evaluate,"EvaluateFeature");GET(release,MfgHeadless::Release,"ReleaseFeature");
#undef GET
  return true;
 }
};
// Shared NGX initialization and modules are process lifetime. Calling Shutdown1
// on this device would invalidate unrelated SR/Streamline owners.
struct Runtime {
 HANDLE softwareFile{},nativeFile{};MfgHeadless software;Native native;MfgGameParameters softParams;
 ID3D12Device* device{};bool tried{},softwareReady{},nativeReady{};unsigned softwareMax{},nativeMax{};
 bool loaded{};std::wstring softwarePath,nativePath;
};
Runtime& runtime(){static auto* r=new Runtime;return *r;}
std::recursive_mutex& gate(){static auto* g=new std::recursive_mutex;return *g;}
unsigned selectedStrategy{},selectedMultiplier=1,selectedDlss{},selectedFsr{},selectedOrder{};
uint64_t nowNs(){LARGE_INTEGER counter{},frequency{};QueryPerformanceCounter(&counter);QueryPerformanceFrequency(&frequency);return uint64_t(double(counter.QuadPart)*1e9/double(frequency.QuadPart));}
struct Session {
 ID3D12Device* device{};ID3D12CommandQueue* queue{};IDXGISwapChain4* swap{};HWND window{};
 ID3D12CommandAllocator* allocator{};ID3D12GraphicsCommandList* command{};ID3D12Fence* fence{};HANDLE event{};
 uint64_t sequence{},lastWork{},epoch=1,engine=UINT64_MAX,previous{},ordinal{},prepared{},images{},allReal{},allGenerated{};
 HANDLE timerEvent{},latencyEvent{},applicationEvent{},jobEvent{},jobDone{},workerThread{};
 bool working{},workerStop{},historyValid{},rawPresented{},latencyReady{},applicationWaitable{},applicationHandleIssued{};
 UINT applicationFlags{},requestedLatency{};unsigned timingFault{};HRESULT workerResult=S_OK;
 uint64_t refreshNs{},lastPresentation{},arrival{},lastOwnedNs{},jobEpoch{},jobAccepted{},jobCompleted{},latencyWaits{};
 RealCadence cadence;PhaseSchedule schedule;Identity jobIdentity{};RealPair jobPair{};Plan jobPlan{};GameTransaction jobTransaction;
 UINT jobFlags{};unsigned jobPresentCount{};std::array<uint64_t,6> jobTimes{};
 ID3D12Resource* history{},*composed{};
 ID3D12Resource* nonblockTarget{};bool nonblockCopy{},nativeRoute{};
 bool recording{},uncertain{},pending{},historyLost=true,outputValid{},closing{},hasFeatures{};
 unsigned logicalIndex{},fault{},warmup{},strategy{},multiplier=1,dlssGenerated{},fsrGenerated{},order{},operationLeases{};
 AMD::AntiLag2DX12::Context* antiLag{};bool antiLagEnabled{};
 DXGI_COLOR_SPACE_TYPE colour=DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
 std::vector<ID3D12Resource*> buffers;std::array<ID3D12Resource*,5> outputs{};ID3D12Resource* outputReal{},*scratch{};
 ID3D12Resource* depth{},*motion{},*scene{};MCD2AmdFgGuidesV1 guides{};MfgGameGuides converted;
 mcd2::providers::FgOutputContract output{};MCD2FgDiagnosticBindingV1 binding{};
 Identity identity{};RealPair pair{};Plan plan{};GameTransaction transaction;
 MCD2MfgStateV1 state{sizeof(MCD2MfgStateV1)};
 void invalidate(){cadence.reset();pending=false;historyLost=true;warmup=0;previous=0;++epoch;transaction.invalidate();auto restart=state.restartRequired;state={sizeof(state)};state.restartRequired=restart;state.identity=binding.identity;state.epoch=epoch;}
 void inputs(){drop(depth);drop(motion);drop(scene);engine=UINT64_MAX;}
 bool wait(uint64_t value){auto done=fence->GetCompletedValue();if(done==UINT64_MAX)return false;if(done>=value)return true;
  if(FAILED(fence->SetEventOnCompletion(value,event))||WaitForSingleObject(event,2000)!=WAIT_OBJECT_0)return false;
  done=fence->GetCompletedValue();return done!=UINT64_MAX&&done>=value;
 }
 bool drain(){if(!queue||!fence||!event)return false;auto value=++sequence;if(FAILED(queue->Signal(fence,value))){uncertain=true;return false;}if(!wait(value)){uncertain=true;return false;}uncertain=false;return true;}
 bool begin(){if(uncertain||!command||!allocator)return false;if(lastWork&&!wait(lastWork))return false;recording=SUCCEEDED(allocator->Reset())&&SUCCEEDED(command->Reset(allocator,nullptr));return recording;}
 bool submit(bool waitForCompletion=true){if(FAILED(command->Close()))return false;recording=false;ID3D12CommandList* commands[]={command};queue->ExecuteCommandLists(1,commands);lastWork=++sequence;
  if(FAILED(queue->Signal(fence,lastWork))||(waitForCompletion&&!wait(lastWork))){uncertain=true;return false;}return true;}
 bool commandCreate(){if(!allocator&&FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator))))return false;
  if(!command){if(FAILED(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator,nullptr,IID_PPV_ARGS(&command))))return false;if(FAILED(command->Close()))return false;}return true;}
};
// Acquired/released only under gate. Close cannot reclaim this session or any
// of its handles/resources while an API operation temporarily releases gate.
struct SessionLease {Session& session;explicit SessionLease(Session& s):session(s){++s.operationLeases;}~SessionLease(){--session.operationLeases;}SessionLease(const SessionLease&)=delete;};
bool waitWorker(Session& s,DWORD timeout=2500,bool teardown=false){if(s.closing&&!teardown)return false;if(!s.working)return true;auto epoch=s.epoch;gate().unlock();auto result=WaitForSingleObject(s.jobDone,timeout);gate().lock();if(result!=WAIT_OBJECT_0){s.uncertain=true;return false;}return !s.working&&(teardown||(!s.closing&&s.epoch==epoch));}
Session*& current(){static Session* s{};return s;}
void transition(ID3D12GraphicsCommandList* cmd,ID3D12Resource* resource,D3D12_RESOURCE_STATES from,D3D12_RESOURCE_STATES to){
 if(from==to)return;D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition={resource,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,from,to};cmd->ResourceBarrier(1,&b);
}
bool resourceDevice(ID3D12DeviceChild* child,ID3D12Device* device){if(!child)return false;ID3D12Device* actual{};child->GetDevice(IID_PPV_ARGS(&actual));bool ok=same(actual,device);drop(actual);return ok;}
bool texture(Session& s,ID3D12Resource* resource,unsigned w,unsigned h,DXGI_FORMAT format){
 if(!resource||!resourceDevice(resource,s.device))return false;auto d=resource->GetDesc();return d.Dimension==D3D12_RESOURCE_DIMENSION_TEXTURE2D&&d.Width==w&&d.Height==h&&d.DepthOrArraySize==1&&d.MipLevels==1&&d.SampleDesc.Count==1&&d.Format==format;
}
ID3D12Resource* image(Session& s,DXGI_FORMAT format,D3D12_RESOURCE_FLAGS flags,D3D12_RESOURCE_STATES state,unsigned width=0,unsigned height=0){
 D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;d.Width=width?width:s.output.width;d.Height=height?height:s.output.height;d.DepthOrArraySize=d.MipLevels=1;d.SampleDesc.Count=1;d.Format=format;d.Flags=flags;
 ID3D12Resource* result{};s.device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&d,state,nullptr,IID_PPV_ARGS(&result));return result;
}
bool virtualBuffers(Session& s){DXGI_SWAP_CHAIN_DESC1 d{};if(FAILED(s.swap->GetDesc1(&d)))return false;
 s.output.width=d.Width;s.output.height=d.Height;s.output.backbufferFormat=s.output.sceneFormat=unsigned(d.Format);
 s.buffers.resize(d.BufferCount);for(auto& buffer:s.buffers){buffer=image(s,d.Format,D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,D3D12_RESOURCE_STATE_PRESENT);if(!buffer)return false;}s.logicalIndex=0;return true;
}
bool retireFeatures(Session& s,bool teardown=false){
 if(s.closing&&!teardown)return false;s.invalidate();if(!waitWorker(s,2500,teardown)||!s.drain())return false;drop(s.nonblockTarget);s.nonblockCopy=false;
 // Destroy our closed recording before a strictly newer queue completion proof.
 drop(s.command);s.recording=false;if(!s.drain())return false;
 auto& r=runtime();if(r.software.feature){if(r.software.release(r.software.feature)!=NVSDK_NGX_Result_Success)return false;r.software.feature=nullptr;}
 if(r.native.feature){if(r.native.release(r.native.feature)!=NVSDK_NGX_Result_Success)return false;r.native.feature=nullptr;}
 if(r.native.params){if(r.native.destroy(r.native.params)!=NVSDK_NGX_Result_Success)return false;r.native.params=nullptr;}
 s.converted.clear();for(auto& p:s.outputs)drop(p);drop(s.outputReal);drop(s.scratch);s.inputs();s.hasFeatures=false;s.fault=0;s.timingFault=0;s.workerResult=S_OK;return s.commandCreate();
}
bool closePresenter(){auto* s=current();if(!s)return true;s->closing=true;if(!waitWorker(*s,2500,true))return false;
 auto until=GetTickCount64()+2500;while(s->operationLeases){if(GetTickCount64()>=until)return false;gate().unlock();Sleep(1);gate().lock();}
 s->antiLag=nullptr;s->antiLagEnabled=false;if(!retireFeatures(*s,true))return false;
 if(s->workerThread){s->workerStop=true;SetEvent(s->jobEvent);gate().unlock();auto ended=WaitForSingleObject(s->workerThread,2500);gate().lock();if(ended!=WAIT_OBJECT_0)return false;CloseHandle(s->workerThread);}
 for(auto h:{s->timerEvent,s->latencyEvent,s->applicationEvent,s->jobEvent,s->jobDone})if(h)CloseHandle(h);drop(s->history);drop(s->composed);
 for(auto& buffer:s->buffers)drop(buffer);drop(s->command);drop(s->allocator);drop(s->fence);drop(s->queue);drop(s->device);drop(s->swap);if(s->event)CloseHandle(s->event);current()=nullptr;delete s;return true;
}
int fail(Session& s,int code){
 // Discard an unsuccessful owned recording before real-frame fallback may reset
 // the allocator. Never submit an incomplete generated batch.
 if(s.recording&&!s.uncertain){if(FAILED(s.command->Close()))s.uncertain=true;else s.recording=false;}
 s.fault=unsigned(code<0?-code:code);s.invalidate();return code;}
bool live(IDXGISwapChain4* raw){return current()&&current()->swap==raw&&!current()->closing;}
HRESULT buffer(UINT index,REFIID iid,void** out){if(!out)return E_POINTER;*out=nullptr;auto& s=*current();if(s.nativeRoute)return s.swap->GetBuffer(index,iid,out);if(index>=s.buffers.size()||!s.buffers[index])return DXGI_ERROR_INVALID_CALL;return s.buffers[index]->QueryInterface(iid,out);}
UINT index(){auto& s=*current();return s.nativeRoute?s.swap->GetCurrentBackBufferIndex():s.logicalIndex;}
void changed(){if(auto* s=current()){s->refreshNs=0;s->invalidate();}}
bool synchronizePresentation(){if(!current())return true;auto& s=*current();SessionLease lease(s);return waitWorker(s);}
void colour(DXGI_COLOR_SPACE_TYPE value){if(auto* s=current();s&&s->colour!=value){s->colour=value;s->outputValid=false;s->historyValid=false;s->refreshNs=0;s->invalidate();}}
void stateIdentity(Session& s){auto id=s.binding.identity;
 Identity next{id.session,id.revision,id.checksum,id.world,s.epoch};
 if(s.identity.session!=next.session||s.identity.revision!=next.revision||s.identity.checksum!=next.checksum||s.identity.world!=next.world){
  s.invalidate();next.epoch=s.epoch;s.state={sizeof(s.state)};s.state.identity=id;
 }s.identity=next;s.state.epoch=s.epoch;
}
bool presentCopy(Session& s,ID3D12Resource* source,D3D12_RESOURCE_STATES state,bool waitForCompletion=true,bool append=false){
 ID3D12Resource* target{};if(FAILED(s.swap->GetBuffer(s.swap->GetCurrentBackBufferIndex(),IID_PPV_ARGS(&target))))return false;
 if(!append&&!s.begin()){drop(target);return false;}transition(s.command,source,state,D3D12_RESOURCE_STATE_COPY_SOURCE);transition(s.command,target,D3D12_RESOURCE_STATE_PRESENT,D3D12_RESOURCE_STATE_COPY_DEST);
 s.command->CopyResource(target,source);transition(s.command,target,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_PRESENT);transition(s.command,source,D3D12_RESOURCE_STATE_COPY_SOURCE,state);
 bool ok=s.submit(waitForCompletion);if(ok){if(waitForCompletion)drop(target);else s.nonblockTarget=target;} // uncertain GPU submission retains the lease
 return ok;
}
bool rectangle(const RECT& r,unsigned width,unsigned height){return r.left>=0&&r.top>=0&&r.right>r.left&&r.bottom>r.top&&uint64_t(r.right)<=width&&uint64_t(r.bottom)<=height;}
bool validPartial(Session& s,const DXGI_PRESENT_PARAMETERS* p){
 if(!p)return true;if((p->DirtyRectsCount&&!p->pDirtyRects)||bool(p->pScrollRect)!=bool(p->pScrollOffset)||p->DirtyRectsCount>4096)return false;
 for(unsigned i=0;i<p->DirtyRectsCount;++i)if(!rectangle(p->pDirtyRects[i],s.output.width,s.output.height))return false;
 if(p->pScrollRect){auto r=*p->pScrollRect;if(!rectangle(r,s.output.width,s.output.height))return false;
  int64_t left=int64_t(r.left)-p->pScrollOffset->x,top=int64_t(r.top)-p->pScrollOffset->y,right=int64_t(r.right)-p->pScrollOffset->x,bottom=int64_t(r.bottom)-p->pScrollOffset->y;
  if(left<0||top<0||right>s.output.width||bottom>s.output.height)return false;}
 return true;
}
bool composeReal(Session& s,const DXGI_PRESENT_PARAMETERS* p,bool submit=true){
 bool partial=p&&p->DirtyRectsCount;if(partial&&!s.historyValid)return false;
 if(!s.history)s.history=image(s,DXGI_FORMAT(s.output.backbufferFormat),D3D12_RESOURCE_FLAG_NONE,D3D12_RESOURCE_STATE_COPY_SOURCE);
 if(!s.composed)s.composed=image(s,DXGI_FORMAT(s.output.backbufferFormat),D3D12_RESOURCE_FLAG_NONE,D3D12_RESOURCE_STATE_COPY_SOURCE);
 if(!s.history||!s.composed||!s.begin())return false;auto* source=s.buffers[s.logicalIndex];
 transition(s.command,s.composed,D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_COPY_DEST);transition(s.command,source,D3D12_RESOURCE_STATE_PRESENT,D3D12_RESOURCE_STATE_COPY_SOURCE);
 if(!partial)s.command->CopyResource(s.composed,source);
 else{
  s.command->CopyResource(s.composed,s.history);
  D3D12_TEXTURE_COPY_LOCATION to{},from{};to.pResource=s.composed;to.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;from.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
  if(p->pScrollRect){auto& r=*p->pScrollRect;auto& o=*p->pScrollOffset;D3D12_BOX box{UINT(int64_t(r.left)-o.x),UINT(int64_t(r.top)-o.y),0,UINT(int64_t(r.right)-o.x),UINT(int64_t(r.bottom)-o.y),1};from.pResource=s.history;s.command->CopyTextureRegion(&to,r.left,r.top,0,&from,&box);}
  from.pResource=source;for(unsigned i=0;i<p->DirtyRectsCount;++i){auto& r=p->pDirtyRects[i];D3D12_BOX box{UINT(r.left),UINT(r.top),0,UINT(r.right),UINT(r.bottom),1};s.command->CopyTextureRegion(&to,r.left,r.top,0,&from,&box);}
 }
 transition(s.command,source,D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_PRESENT);transition(s.command,s.composed,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_COPY_SOURCE);
 return !submit||s.submit();
}
uint64_t outputRefresh(Session& s){IDXGIOutput* output{};DXGI_OUTPUT_DESC desc{};uint64_t result{};
 if(SUCCEEDED(s.swap->GetContainingOutput(&output))&&SUCCEEDED(output->GetDesc(&desc))){DEVMODEW mode{};mode.dmSize=sizeof(mode);if(EnumDisplaySettingsW(desc.DeviceName,ENUM_CURRENT_SETTINGS,&mode)&&mode.dmDisplayFrequency>=20&&mode.dmDisplayFrequency<=1000)result=1000000000ull/mode.dmDisplayFrequency;}drop(output);return result;}
void updateRefresh(Session& s,uint64_t refresh){s.refreshNs=refresh;s.timingFault=refresh?0u:314u;}
bool latencySlot(Session& s,bool nonblock=false,bool worker=false){
 auto epoch=s.epoch;
 if(s.latencyReady)return true;if(!s.latencyEvent)return false;
 auto timeout=nonblock?0u:DWORD(std::clamp<uint64_t>((s.refreshNs*4+999999)/1000000,50,250));
 gate().unlock();auto result=WaitForSingleObject(s.latencyEvent,timeout);gate().lock();s.latencyReady=result==WAIT_OBJECT_0;if(s.latencyReady)++s.latencyWaits;return s.latencyReady&&(worker||(!s.closing&&s.epoch==epoch));
}
HRESULT rawPresent(Session& s,UINT sync,UINT flags,bool generated){
 if(s.antiLag&&s.antiLagEnabled){auto marker=AMD::AntiLag2DX12::SetFrameGenFrameType(s.antiLag,generated);if(FAILED(marker))return marker;}
 DXGI_PRESENT_PARAMETERS full{};auto hr=s.swap->Present1(sync,flags,&full);if(hr==S_OK){s.latencyReady=false;s.rawPresented=true;s.lastPresentation=nowNs();}return hr;
}
void commitReal(Session& s,bool countIdentity){
 std::swap(s.history,s.composed);s.historyValid=true;++s.allReal;
 if(countIdentity){++s.state.realPresented;s.state.observedAtMs=GetTickCount64();}
}
bool deadline(Session& s,uint64_t target){
 while(nowNs()<target){if(s.epoch!=s.jobEpoch||s.closing||GetForegroundWindow()!=s.window)return false;
  auto now=nowNs();if(now>=target)break;auto remaining=std::min<uint64_t>(target-now,2000000);LARGE_INTEGER due{};due.QuadPart=-LONGLONG(std::max<uint64_t>(remaining/100,1));if(!SetWaitableTimer(s.timerEvent,&due,0,nullptr,nullptr,FALSE))return false;gate().unlock();auto waited=WaitForSingleObject(s.timerEvent,50);gate().lock();if(waited!=WAIT_OBJECT_0)return false;}
 return s.epoch==s.jobEpoch&&!s.closing&&GetForegroundWindow()==s.window;
}
DWORD WINAPI presenterWorker(void* context){auto& s=*static_cast<Session*>(context);
 for(;;){WaitForSingleObject(s.jobEvent,INFINITE);std::lock_guard lock(gate());if(s.workerStop)return 0;
  HRESULT result=S_OK;bool generated=true;unsigned completed=0;s.jobPresentCount=0;
  for(unsigned slot=1;slot<=s.jobPlan.count;++slot){
   auto target=std::max(s.schedule.deadline(slot-1),s.lastPresentation?s.lastPresentation+s.schedule.step:0ull);
   // Cancellation before/while waiting never produces a catch-up burst. The
   // accepted real image is still presented; no later real image replaces it.
   if(!deadline(s,target)){generated=false;break;}
   if(!latencySlot(s,false,true)){result=DXGI_ERROR_WAS_STILL_DRAWING;generated=false;break;}
   if(s.epoch!=s.jobEpoch||s.closing||GetForegroundWindow()!=s.window){generated=false;break;}
   auto output=s.jobTransaction.output(s.jobIdentity,s.jobPair,slot);
   if(!output||!presentCopy(s,reinterpret_cast<ID3D12Resource*>(output),D3D12_RESOURCE_STATE_UNORDERED_ACCESS)){result=E_FAIL;generated=false;break;}
   result=rawPresent(s,s.schedule.sync,s.jobFlags,true);
   if(!s.jobTransaction.presented(s.jobIdentity,s.jobPair,slot,result==S_OK)){generated=false;break;}
   s.jobTimes[s.jobPresentCount++]=s.lastPresentation;++s.allGenerated;++completed;
   if(s.epoch==s.jobEpoch&&s.identity==s.jobIdentity){if(s.jobPlan.phases[slot-1].provider==Provider::Software)++s.state.softwarePresented;else ++s.state.nativePresented;}
  }
  // Even after cancellation, serialize the accepted current real image behind
  // any phase already submitted. The per-phase minimum interval still applies.
  if(SUCCEEDED(result)){
   auto target=std::max(s.schedule.end,s.lastPresentation?s.lastPresentation+s.schedule.step:0ull);
   if(generated)deadline(s,target);
   else if(completed){gate().unlock();auto now=nowNs();if(target>now)Sleep(DWORD(std::min<uint64_t>((target-now+999999)/1000000,250)));gate().lock();}
   if(!latencySlot(s,false,true))result=DXGI_ERROR_WAS_STILL_DRAWING;
   else if(!presentCopy(s,s.composed,D3D12_RESOURCE_STATE_COPY_SOURCE))result=E_FAIL;
   else result=rawPresent(s,s.schedule.sync,s.jobFlags,false);
   if(result==S_OK){s.jobTimes[s.jobPresentCount++]=s.lastPresentation;bool identity=s.epoch==s.jobEpoch&&s.identity==s.jobIdentity;commitReal(s,identity);++s.jobCompleted;
    if(identity){s.state.active=generated&&s.jobTransaction.realPresented(s.jobIdentity,s.jobPair,true);s.state.observedAtMs=GetTickCount64();}}
  }
  if(!s.drain()){s.uncertain=true;result=E_FAIL;}
  else if(!s.jobTransaction.retire({reinterpret_cast<uint64_t>(s.queue),reinterpret_cast<uint64_t>(s.fence),s.sequence},s.fence->GetCompletedValue()))result=E_FAIL;
  if(FAILED(result)){s.fault=313;s.state.active=0;}
  s.workerResult=result;s.working=false;SetEvent(s.jobDone);
 }
}
HRESULT present(UINT sync,UINT flags,const DXGI_PRESENT_PARAMETERS* parameters){
 auto& s=*current();SessionLease lease(s);
 if(s.nativeRoute){if(!(flags&(DXGI_PRESENT_TEST|DXGI_PRESENT_DO_NOT_SEQUENCE))&&s.antiLag&&s.antiLagEnabled){auto marker=AMD::AntiLag2DX12::SetFrameGenFrameType(s.antiLag,false);if(FAILED(marker))return marker;}
  auto hr=parameters?s.swap->Present1(sync,flags,parameters):s.swap->Present(sync,flags);if(hr==S_OK&&!(flags&(DXGI_PRESENT_TEST|DXGI_PRESENT_DO_NOT_SEQUENCE)))++s.allReal;return hr;}
 if(flags&DXGI_PRESENT_TEST)return parameters?s.swap->Present1(sync,flags,parameters):s.swap->Present(sync,flags);
 if(s.working&&(flags&DXGI_PRESENT_DO_NOT_WAIT))return DXGI_ERROR_WAS_STILL_DRAWING;
 if(!waitWorker(s))return DXGI_ERROR_WAS_STILL_DRAWING;if(FAILED(s.workerResult))return s.workerResult;
 // This operation refers to the already-current raw buffer. It must not copy a
 // new virtual image, consume pending generated phases, or advance any history.
 if(flags&DXGI_PRESENT_DO_NOT_SEQUENCE){
  if(sync>4)return DXGI_ERROR_INVALID_CALL;
  if(!s.rawPresented)return S_OK; // DXGI's first-operation no-op.
  // Some translation layers incorrectly rotate even with this flag. The
  // already displayed full image is the current image: leave it in place.
  auto status=s.swap->Present(0,DXGI_PRESENT_TEST);if(status!=S_OK||!sync)return status;
  if(flags&DXGI_PRESENT_DO_NOT_WAIT)return DXGI_ERROR_WAS_STILL_DRAWING;
  IDXGIOutput* output{};if(FAILED(s.swap->GetContainingOutput(&output)))return E_FAIL;HRESULT result=S_OK;
  gate().unlock();for(unsigned i=0;i<sync&&SUCCEEDED(result);++i)result=output->WaitForVBlank();gate().lock();output->Release();return result;
 }
 if(sync>4||!validPartial(s,parameters))return E_INVALIDARG;
 const bool partial=parameters&&(parameters->DirtyRectsCount||parameters->pScrollRect);
 if(partial||!s.strategy||GetForegroundWindow()!=s.window||(flags&DXGI_PRESENT_RESTART))s.invalidate();
 if(s.pending&&GetTickCount64()-s.state.observedAtMs>1500)s.invalidate();
 bool generated=s.pending&&s.identity==Identity{s.binding.identity.session,s.binding.identity.revision,s.binding.identity.checksum,s.binding.identity.world,s.epoch};
 auto actualRefresh=outputRefresh(s);if(actualRefresh!=s.refreshNs){s.cadence.reset();s.lastPresentation=0;}updateRefresh(s,actualRefresh);
 s.schedule=s.cadence.schedule(nowNs(),s.lastPresentation,s.multiplier,s.refreshNs,sync,(flags&DXGI_PRESENT_ALLOW_TEARING)!=0);
 s.timingFault=!s.refreshNs?314u:s.cadence.period()&&!s.schedule?315u:0u;
 generated=generated&&bool(s.schedule)&&!(flags&DXGI_PRESENT_DO_NOT_WAIT);
 if(generated){
  if(!composeReal(s,parameters))return E_FAIL;
  s.jobIdentity=s.identity;s.jobPair=s.pair;s.jobPlan=s.plan;s.jobEpoch=s.epoch;s.jobFlags=flags;s.jobTransaction.invalidate();
  Completion ticket{reinterpret_cast<uint64_t>(s.queue),reinterpret_cast<uint64_t>(s.fence),s.lastWork};
  if(!s.jobTransaction.begin(s.identity,s.pair,s.plan))return E_FAIL;
  for(unsigned slot=1;slot<=s.plan.count;++slot){auto output=s.transaction.output(s.identity,s.pair,slot);if(!output||!s.jobTransaction.record(s.pair,slot,s.plan.phases[slot-1].provider,output,true))return E_FAIL;}
  if(!s.jobTransaction.submitted(ticket)||!s.jobTransaction.completed(ticket,s.fence->GetCompletedValue()))return E_FAIL;
  s.pending=false;s.transaction.invalidate();s.working=true;s.workerResult=S_OK;++s.jobAccepted;ResetEvent(s.jobDone);s.logicalIndex=(s.logicalIndex+1)%unsigned(s.buffers.size());SetEvent(s.jobEvent);SetEvent(s.applicationEvent);s.lastOwnedNs=nowNs()-s.arrival;return S_OK;
 }
 // A missing cadence, output refresh or feasible phase interval is a full real
 // frame fallback. There is no multiplier lie or burst of overdue phases.
 s.pending=false;s.transaction.invalidate();s.state.active=0;
 bool copied=false;
 if(flags&DXGI_PRESENT_DO_NOT_WAIT){
  if(!s.nonblockCopy){s.invalidate();auto done=s.fence->GetCompletedValue();if(done==UINT64_MAX)return DXGI_ERROR_DEVICE_REMOVED;if(done<s.lastWork||!latencySlot(s,true)||s.closing)return DXGI_ERROR_WAS_STILL_DRAWING;
   if(!composeReal(s,parameters,false)||!presentCopy(s,s.composed,D3D12_RESOURCE_STATE_COPY_SOURCE,false,true))return E_FAIL;s.nonblockCopy=true;return DXGI_ERROR_WAS_STILL_DRAWING;}
  auto done=s.fence->GetCompletedValue();if(done==UINT64_MAX)return DXGI_ERROR_DEVICE_REMOVED;if(done<s.lastWork)return DXGI_ERROR_WAS_STILL_DRAWING;drop(s.nonblockTarget);copied=true;
 }else{
  if(s.nonblockCopy){if(!s.wait(s.lastWork))return E_FAIL;drop(s.nonblockTarget);s.nonblockCopy=false;}
  if(!latencySlot(s)||s.closing)return DXGI_ERROR_WAS_STILL_DRAWING;
  if(!composeReal(s,parameters)||!presentCopy(s,s.composed,D3D12_RESOURCE_STATE_COPY_SOURCE))return E_FAIL;
 }
 auto hr=rawPresent(s,sync,flags,false);if(hr==DXGI_ERROR_WAS_STILL_DRAWING&&copied)return hr;s.nonblockCopy=false;
 if(hr==S_OK){commitReal(s,s.binding.engineFrame==s.state.currentReal&&bool(s.identity));s.logicalIndex=(s.logicalIndex+1)%unsigned(s.buffers.size());SetEvent(s.applicationEvent);}else s.invalidate();s.lastOwnedNs=s.arrival?nowNs()-s.arrival:0;return hr;
}
HRESULT resize(UINT count,UINT width,UINT height,DXGI_FORMAT format,UINT flags,const UINT* nodes,IUnknown*const* queues){
 auto& s=*current();SessionLease lease(s);if((flags^s.applicationFlags)&DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT)return DXGI_ERROR_INVALID_CALL;
 if((count&&(count<2||count>16))||width>D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION||height>D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION)return DXGI_ERROR_INVALID_CALL;
 DXGI_SWAP_CHAIN_DESC1 old{};if(FAILED(s.swap->GetDesc1(&old)))return E_FAIL;
 const auto buffers=count?count:old.BufferCount;
 if(queues)for(unsigned i=0;i<buffers;++i)if(!same(queues[i],s.queue)||(nodes&&nodes[i]>1))return DXGI_ERROR_INVALID_CALL;
 // Match native DXGI reference refusal before changing any visible route.
 for(auto* p:s.buffers){p->AddRef();if(p->Release()!=1)return DXGI_ERROR_INVALID_CALL;}
 if(!retireFeatures(s))return DXGI_ERROR_WAS_STILL_DRAWING;
 const bool native=s.applicationHandleIssued?s.nativeRoute:s.strategy==0;std::vector<ID3D12Resource*> staged;
 auto newFormat=format==DXGI_FORMAT_UNKNOWN?old.Format:format;
 if(!native){
  RECT client{};if((!width||!height)&&!GetClientRect(s.window,&client))return E_FAIL;
  if(!width)width=UINT(client.right-client.left);if(!height)height=UINT(client.bottom-client.top);
  if(!width||!height||width>D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION||height>D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION)return DXGI_ERROR_INVALID_CALL;
  staged.resize(buffers);
  for(auto& p:staged){p=image(s,newFormat,D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,D3D12_RESOURCE_STATE_PRESENT,width,height);if(!p){for(auto& q:staged)drop(q);return E_OUTOFMEMORY;}}
 }
 UINT previousLatency{};if(FAILED(s.swap->GetMaximumFrameLatency(&previousLatency))){for(auto& p:staged)drop(p);return E_FAIL;}
 UINT targetLatency=native?s.requestedLatency:1;if(native&&previousLatency!=1)targetLatency=previousLatency;
 auto latencyResult=s.swap->SetMaximumFrameLatency(targetLatency);if(FAILED(latencyResult)){for(auto& p:staged)drop(p);return latencyResult;}
 auto rawFlags=flags|DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;auto hr=queues?s.swap->ResizeBuffers1(count,width,height,format,rawFlags,nodes,queues):s.swap->ResizeBuffers(count,width,height,format,rawFlags);
 if(FAILED(hr)){for(auto& p:staged)drop(p);if(FAILED(s.swap->SetMaximumFrameLatency(previousLatency)))s.fault=316;return hr;}
 if(native)s.requestedLatency=targetLatency;else if(s.nativeRoute)s.requestedLatency=previousLatency;
 for(auto& p:s.buffers)drop(p);s.buffers=std::move(staged);drop(s.history);drop(s.composed);s.historyValid=false;s.rawPresented=false;s.latencyReady=false;s.lastPresentation=0;s.cadence.reset();s.applicationFlags=flags;s.applicationWaitable=(flags&DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT)!=0;SetEvent(s.applicationEvent);s.nativeRoute=native;s.logicalIndex=0;s.outputValid=false;s.state.restartRequired=native!=(s.strategy==0);
 DXGI_SWAP_CHAIN_DESC1 actual{};if(SUCCEEDED(s.swap->GetDesc1(&actual))){s.output.width=actual.Width;s.output.height=actual.Height;s.output.backbufferFormat=s.output.sceneFormat=unsigned(actual.Format);}
 return hr;
}
HANDLE applicationLatency(){auto& s=*current();if(!s.applicationWaitable)return nullptr;HANDLE duplicate{};auto source=s.nativeRoute?s.latencyEvent:s.applicationEvent;DuplicateHandle(GetCurrentProcess(),source,GetCurrentProcess(),&duplicate,0,FALSE,DUPLICATE_SAME_ACCESS);if(duplicate)s.applicationHandleIssued=true;return duplicate;}
HRESULT maximumLatency(UINT value){if(!value||value>16)return DXGI_ERROR_INVALID_CALL;auto& s=*current();if(!s.applicationWaitable)return DXGI_ERROR_INVALID_CALL;auto hr=s.swap->SetMaximumFrameLatency(s.nativeRoute?value:1);if(SUCCEEDED(hr))s.requestedLatency=value;return hr;}
HRESULT maximumLatencyGet(UINT* value){if(!value)return E_POINTER;auto& s=*current();if(s.nativeRoute)return s.swap->GetMaximumFrameLatency(value);*value=1;return S_OK;}
UINT applicationFlags(){return current()->applicationFlags;}
int capabilities(ID3D12Device* device,MCD2MfgCapabilitiesV1* out){
 auto& r=runtime();if(!device||!r.loaded)return -1;
 if(r.device&&!same(r.device,device))return -2;
 if(!r.tried){r.tried=true;r.device=device;device->AddRef();
  wchar_t cache[MAX_PATH]{};if(!GetTempPathW(MAX_PATH,cache))return -3;
  if(r.software.init(0,cache,device,NVSDK_NGX_Version_API,nullptr)==NVSDK_NGX_Result_Success&&r.software.populate(&r.softParams)==NVSDK_NGX_Result_Success){
   unsigned count{};if(r.softParams.Get("DLSSG.MultiFrameCountMax",&count)==NVSDK_NGX_Result_Success&&count>=1&&count<=5){r.softwareReady=true;r.softwareMax=count;}}
  if(r.native.module&&r.native.init&&r.native.populate&&r.native.allocate&&r.native.destroy&&r.native.create&&r.native.evaluate&&r.native.release){const wchar_t* paths[]={r.native.directory.c_str()};NVSDK_NGX_FeatureCommonInfo info{};info.PathListInfo.Path=paths;info.PathListInfo.Length=1;
   if(r.native.init("3f1c38b7-1bcd-4d3c-9847-7230a1893d82",NVSDK_NGX_ENGINE_TYPE_CUSTOM,"mcd2-mfg",cache,device,NVSDK_NGX_Version_API,&info)==NVSDK_NGX_Result_Success){
    NVSDK_NGX_Parameter* p{};if(r.native.populate(&p)==NVSDK_NGX_Result_Success&&p){unsigned count{};if(p->Get("DLSSG.MultiFrameCountMax",&count)==NVSDK_NGX_Result_Success&&count>=1&&count<=5){r.nativeReady=true;r.nativeMax=count;}r.native.destroy(p);}}}
 }
 *out={sizeof(*out),r.softwareReady?1u:0u,r.nativeReady?1u:0u,r.softwareMax,r.nativeMax,0,0,0};
 for(unsigned n=2;n<=6;++n)if(r.softwareReady&&r.softwareMax>=n-1)out->softwareMultiplierMask|=1u<<n;
 if(r.softwareReady&&r.nativeReady){if(r.softwareMax>=3)out->mixedMultiplierMask|=1u<<4;if(r.softwareMax>=5)out->mixedMultiplierMask|=1u<<6;}
 return r.softwareReady?0:-4;
}
void parameters(Session& s,NVSDK_NGX_Parameter& p,bool native,unsigned slot){
 const auto& c=s.guides.camera;
 p.Set("Width",s.output.width);p.Set("Height",s.output.height);p.Set("OutWidth",s.output.width);p.Set("OutHeight",s.output.height);
 p.Set("CreationNodeMask",1u);p.Set("VisibilityNodeMask",1u);
 p.Set("DLSSG.BackbufferWidth",s.output.width);p.Set("DLSSG.BackbufferHeight",s.output.height);p.Set("DLSSG.BackbufferFormat",s.output.backbufferFormat);
 p.Set("DLSSG.CmdQueue",static_cast<void*>(s.queue));p.Set("DLSSG.CmdAlloc",static_cast<void*>(s.allocator));
 p.Set("DLSSG.ColorBuffersHDR",mcd2::providers::fgOutputHdr(s.output)?1u:0u);p.Set("DLSSG.DepthInverted",1u);
 p.Set("DLSSG.MvecDilated",0u);p.Set("DLSSG.MvecJittered",0u);p.Set("DLSSG.MvecScaleX",1.f);p.Set("DLSSG.MvecScaleY",1.f);
 p.Set("DLSSG.CameraNear",c.nearPlane);p.Set("DLSSG.CameraFar",c.farPlane);p.Set("DLSSG.CameraFOV",c.verticalFOV);p.Set("DLSSG.CameraAspectRatio",c.aspectRatio);
 p.Set("DLSSG.JitterOffsetX",c.jitter[0]*s.output.width/c.width);p.Set("DLSSG.JitterOffsetY",c.jitter[1]*s.output.height/c.height);p.Set("FrameTimeDeltaInMsec",s.guides.frameTimeMs);p.Set("DLSSG.EnableInterp",1u);
 p.Set("DLSSG.CameraViewToClip",const_cast<float*>(c.viewToClip));p.Set("DLSSG.ClipToCameraView",const_cast<float*>(c.clipToView));
 p.Set("DLSSG.ClipToPrevClip",const_cast<float*>(c.clipToPrevious));p.Set("DLSSG.PrevClipToClip",const_cast<float*>(c.previousToClip));
 p.Set("DLSSG.IsRecording",1u);p.Set("DLSSG.FrameIndex",unsigned(s.ordinal));p.Set("DLSSG.FrameCount",unsigned(s.ordinal));p.Set("DLSSG.Reset",s.historyLost||c.reset?1u:0u);
 p.Set("DLSSG.MultiFrameCount",native?native_generated_count(s.plan):s.plan.count);p.Set("DLSSG.MultiFrameIndex",native?native_phase_index(s.plan,slot):slot);
 for(const char* name:{"Backbuffer","Depth","MVecs","HUDLess"}){std::string prefix=std::string("DLSSG.")+name;
  p.Set((prefix+"SubrectBaseX").c_str(),0u);p.Set((prefix+"SubrectBaseY").c_str(),0u);p.Set((prefix+"SubrectWidth").c_str(),s.output.width);p.Set((prefix+"SubrectHeight").c_str(),s.output.height);}
 p.Set("DLSSG.Backbuffer",s.buffers[s.logicalIndex]);p.Set("DLSSG.HUDLess",s.scene);p.Set("DLSSG.Depth",s.converted.depth?s.converted.depth:s.depth);p.Set("DLSSG.MVecs",s.converted.motion?s.converted.motion:s.motion);
 p.Set("DLSSG.OutputReal",s.outputReal);p.Set("DLSSG.OutputInterpolated",s.outputs[slot-1]);
}
bool features(Session& s){
 if(s.hasFeatures)return true;auto& r=runtime();auto format=DXGI_FORMAT(s.output.backbufferFormat);
 D3D12_FEATURE_DATA_FORMAT_SUPPORT support{format};if(FAILED(s.device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT,&support,sizeof(support)))||!(support.Support2&D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE))return false;
 s.outputReal=image(s,format,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);if(!s.outputReal)return false;
 for(auto& output:s.outputs){output=image(s,format,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);if(!output)return false;}
 {
  if(!s.converted.init(s.device,s.output.width,s.output.height)||!s.begin())return false;
  s.converted.record(s.device,s.command,s.depth,s.motion,s.guides.camera.width,s.guides.camera.height);if(!s.submit())return false;}
 if(s.plan.fsrGenerated){
 r.softParams.Reset();unsigned softwareSlot=1;while(s.plan.phases[softwareSlot-1].provider!=Provider::Software)++softwareSlot;parameters(s,r.softParams,false,softwareSlot);size_t bytes{};
 if(r.software.scratch(NVSDK_NGX_Feature_FrameGeneration,&r.softParams,&bytes)!=NVSDK_NGX_Result_Success||bytes>512ull*1024*1024)return false;
 if(bytes){D3D12_HEAP_PROPERTIES h{};h.Type=D3D12_HEAP_TYPE_DEFAULT;D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;d.Width=bytes;d.Height=d.DepthOrArraySize=d.MipLevels=1;d.SampleDesc.Count=1;d.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  if(FAILED(s.device->CreateCommittedResource(&h,D3D12_HEAP_FLAG_NONE,&d,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(&s.scratch))))return false;r.softParams.Set("Scratch",s.scratch);}
 if(!r.softParams.healthy()||!s.begin())return false;
 if(r.software.create(s.command,NVSDK_NGX_Feature_FrameGeneration,&r.softParams,&r.software.feature)!=NVSDK_NGX_Result_Success||!r.software.feature)return false;
 if(!s.submit())return false;
 }
 if(s.plan.dlssGenerated){if(r.native.allocate(&r.native.params)!=NVSDK_NGX_Result_Success||!r.native.params)return false;unsigned nativeSlot=1;while(s.plan.phases[nativeSlot-1].provider!=Provider::NvidiaNative)++nativeSlot;parameters(s,*r.native.params,true,nativeSlot);
  if(!s.begin()||r.native.create(s.command,NVSDK_NGX_Feature_FrameGeneration,r.native.params,&r.native.feature)!=NVSDK_NGX_Result_Success||!r.native.feature)return false;if(!s.submit())return false;}
 s.hasFeatures=true;return true;
}
}
extern "C" int mcd2_mfg_load_v1(const wchar_t* software,const wchar_t* native){
 std::lock_guard lock(gate());mcd2::process_exit::initialize();auto& r=runtime();if(r.software.module)return r.loaded&&software&&r.softwarePath==software&&r.nativePath==(native?native:L"")?0:-2;
 auto softwareFile=verify(software,SoftwareHash);if(!softwareFile)return -3;
 HANDLE nativeFile{};if(native)nativeFile=verify(native,NativeHash);
 r.softwareFile=softwareFile;r.nativeFile=nativeFile;r.softwarePath=software;r.nativePath=native?native:L"";
 if(!r.software.load(software))return -5;
 std::vector<wchar_t> actual(32768);auto length=GetModuleFileNameW(r.software.module,actual.data(),DWORD(actual.size()));
 if(!length||length>=actual.size()||!verify(actual.data(),SoftwareHash))return -7;
 if(nativeFile&&!r.native.load(native)){r.native.init=nullptr;}r.loaded=true;return 0;
}
extern "C" int mcd2_mfg_capabilities_v1(void* device,MCD2MfgCapabilitiesV1* out){if(!out||out->size!=sizeof(*out))return -1;std::lock_guard lock(gate());auto* raw=nativeObject(static_cast<ID3D12Device*>(device));auto result=capabilities(raw,out);drop(raw);return result;}
extern "C" int mcd2_afg_support_v1(void* device){MCD2MfgCapabilitiesV1 caps{sizeof(caps)};return mcd2_mfg_capabilities_v1(device,&caps);}
extern "C" int mcd2_mfg_configure_v3(unsigned strategy,unsigned multiplier,unsigned dlss,unsigned fsr,unsigned order){std::lock_guard lock(gate());auto& r=runtime();
 if(strategy==0){if(multiplier!=1||dlss||fsr||order)return -1;}
 else{
  if(strategy>3 || (strategy==1 && (!dlss||fsr)) ||
     (strategy==2 && (!dlss||!fsr)) || (strategy==3 && (dlss||!fsr)))return -2;
  auto plan=make_allocation_plan(dlss,fsr,{r.softwareReady?r.softwareMax+1:0,r.nativeReady?r.nativeMax+1:0,r.nativeReady},order);
  if(!plan||plan->multiplier!=multiplier)return -3;
 }
 if(auto* s=current()){if(s->closing)return MCD2_AFG_BUSY_V2;SessionLease lease(*s);if(s->strategy!=strategy||s->multiplier!=multiplier||s->dlssGenerated!=dlss||s->fsrGenerated!=fsr||s->order!=order){
  // Off ends generation immediately, even if a bounded drain must be retried.
  if(!strategy){s->strategy=0;s->multiplier=1;s->dlssGenerated=s->fsrGenerated=s->order=0;s->state.active=0;s->state.restartRequired=s->nativeRoute?0u:1u;}
  if(!retireFeatures(*s))return MCD2_AFG_BUSY_V2;s->strategy=strategy;s->multiplier=multiplier;s->dlssGenerated=dlss;s->fsrGenerated=fsr;s->order=order;s->state.active=0;s->state.restartRequired=(s->nativeRoute!=(strategy==0))?1u:0u;}}
 selectedStrategy=strategy;selectedMultiplier=multiplier;selectedDlss=dlss;selectedFsr=fsr;selectedOrder=order;return 0;
}
extern "C" int mcd2_mfg_configure_v2(unsigned strategy,unsigned multiplier,unsigned dlss,unsigned fsr){
 return mcd2_mfg_configure_v3(strategy,multiplier,dlss,fsr,0);
}
extern "C" int mcd2_mfg_configure_v1(unsigned strategy,unsigned multiplier){
 if(strategy==0)return mcd2_mfg_configure_v2(0,multiplier,0,0);
 if(strategy!=2&&strategy!=3)return -2;
 std::optional<Plan> plan;
 {std::lock_guard lock(gate());auto& r=runtime();
  plan=make_plan(strategy==2?Mode::Hybrid:Mode::Software,multiplier,{r.softwareReady?r.softwareMax+1:0,r.nativeReady?r.nativeMax+1:0,r.nativeReady});}
 if(!plan)return -3;
 // v2 owns the sole gate level while retirement waits for the worker.
 // Its capability recheck also covers a change after this legacy translation.
 return mcd2_mfg_configure_v2(strategy,multiplier,plan->dlssGenerated,plan->fsrGenerated);
}
extern "C" int mcd2_afg_swap_v1(void* factory,void* queue,void* hwnd,const MCD2AmdFgSwapV1* p,void** out){
 if(!out)return -1;*out=nullptr;std::lock_guard lock(gate());if(current()||!factory||!queue||!hwnd||!p||p->size!=sizeof(*p)||p->samples!=1||p->buffers<2||p->buffers>16)return -2;
 auto* q=nativeObject(static_cast<ID3D12CommandQueue*>(queue));if(!q)return -3;if(q->GetDesc().Type!=D3D12_COMMAND_LIST_TYPE_DIRECT){drop(q);return -3;}
 auto* s=new(std::nothrow) Session;if(!s){drop(q);return -4;}
 if(FAILED(q->GetDevice(IID_PPV_ARGS(&s->device)))){drop(q);delete s;return -5;}s->queue=q;s->window=static_cast<HWND>(hwnd);s->strategy=selectedStrategy;s->multiplier=selectedMultiplier;s->dlssGenerated=selectedDlss;s->fsrGenerated=selectedFsr;s->order=selectedOrder;s->nativeRoute=selectedStrategy==0;s->applicationFlags=p->flags;s->applicationWaitable=(p->flags&DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT)!=0;
 current()=s;DXGI_SWAP_CHAIN_DESC1 d{p->width,p->height,DXGI_FORMAT(p->format),FALSE,{1,p->sampleQuality},p->usage,p->buffers,DXGI_SCALING(p->scaling),DXGI_SWAP_EFFECT(p->effect),DXGI_ALPHA_MODE(p->alpha),p->flags|DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT};
 DXGI_SWAP_CHAIN_FULLSCREEN_DESC full{{p->refreshNumerator,p->refreshDenominator},DXGI_MODE_SCANLINE_ORDER(p->scanline),DXGI_MODE_SCALING(p->modeScaling),BOOL(p->windowed)};
 IDXGISwapChain1* raw{};auto hr=static_cast<IDXGIFactory2*>(factory)->CreateSwapChainForHwnd(q,s->window,&d,p->fullscreenProvided?&full:nullptr,nullptr,&raw);
 if(SUCCEEDED(hr)){hr=raw->QueryInterface(IID_PPV_ARGS(&s->swap));raw->Release();}
 if(FAILED(hr)){drop(s->queue);drop(s->device);delete s;current()=nullptr;return int(hr);}
 if(FAILED(s->device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&s->fence)))||!(s->event=CreateEventW(nullptr,FALSE,FALSE,nullptr))||!s->commandCreate()||(!s->nativeRoute&&!virtualBuffers(*s))){
  // No compute work has been submitted; releasing failed startup is safe.
  for(auto& b:s->buffers)drop(b);drop(s->command);drop(s->allocator);drop(s->fence);drop(s->swap);drop(s->queue);drop(s->device);if(s->event)CloseHandle(s->event);delete s;current()=nullptr;return -6;}
 s->timerEvent=CreateWaitableTimerExW(nullptr,nullptr,2,TIMER_ALL_ACCESS);if(!s->timerEvent)s->timerEvent=CreateWaitableTimerExW(nullptr,nullptr,0,TIMER_ALL_ACCESS);s->latencyEvent=s->swap->GetFrameLatencyWaitableObject();s->applicationEvent=CreateEventW(nullptr,FALSE,TRUE,nullptr);s->jobEvent=CreateEventW(nullptr,FALSE,FALSE,nullptr);s->jobDone=CreateEventW(nullptr,TRUE,TRUE,nullptr);
 if(!s->timerEvent||!s->latencyEvent||!s->applicationEvent||!s->jobEvent||!s->jobDone||FAILED(s->swap->GetMaximumFrameLatency(&s->requestedLatency))||(!s->nativeRoute&&FAILED(s->swap->SetMaximumFrameLatency(1))))return closePresenter()?-8:MCD2_AFG_SWAP_RETAINED;
 s->refreshNs=outputRefresh(*s);s->workerThread=CreateThread(nullptr,0,presenterWorker,s,0,nullptr);if(!s->workerThread)return closePresenter()?-9:MCD2_AFG_SWAP_RETAINED;
 auto* wrapper=new(std::nothrow) MCD2MfgSwapchain(s->swap,gate(),live,mcd2::process_exit::terminating,present,buffer,index,resize,closePresenter,changed,colour,applicationLatency,maximumLatency,maximumLatencyGet,applicationFlags,synchronizePresentation);
 if(!wrapper){return closePresenter()?-7:MCD2_AFG_SWAP_RETAINED;}*out=static_cast<IDXGISwapChain1*>(wrapper);return 0;
}
extern "C" unsigned mcd2_afg_recording_contract_v2(){return 2;}
extern "C" int mcd2_afg_output_v1(const MCD2AmdFgOutputV1* p){std::lock_guard lock(gate());auto* s=current();if(!s||s->closing||!p||p->size!=sizeof(*p)||p->reserved)return -1;SessionLease lease(*s);
 if(s->nativeRoute)return MCD2_AFG_BUSY_V2;
 using namespace mcd2::providers;FgOutputContract next{p->width,p->height,p->backbufferFormat,p->sceneFormat,FgOutputColour(p->colourSpace),FgSceneStage(p->sceneStage)};DXGI_SWAP_CHAIN_DESC1 d{};
 // No colour transform or reinterpretation. SDK reads/writes this actual
 // encoding and identical source/output formats. BGRA->RGBA is not inferred.
 if((next.colour==FgOutputColour::Srgb&&s->colour!=DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709)||(next.colour==FgOutputColour::Pq2020&&s->colour!=DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020)||!validFgOutput(next)||next.sceneFormat!=next.backbufferFormat||FAILED(s->swap->GetDesc1(&d))||d.Width!=next.width||d.Height!=next.height||unsigned(d.Format)!=next.backbufferFormat){s->outputValid=false;s->invalidate();return MCD2_AFG_OUTPUT_UNSUPPORTED_V3;}
 if(s->outputValid&&s->output==next)return 0;if(!retireFeatures(*s))return MCD2_AFG_BUSY_V2;s->output=next;s->outputValid=true;return 0;
}
extern "C" int mcd2_afg_guides_v2(void* command,void* depth,void* motion,const MCD2AmdFgGuidesV1* p){std::lock_guard lock(gate());auto* s=current();if(!s||s->closing||!s->outputValid||!p||p->size!=sizeof(*p)||p->reserved)return -1;
 const auto& c=p->camera;if(c.size!=sizeof(c)||c.reset>1||!c.width||!c.height||c.width>s->output.width||c.height>s->output.height||!command||!resourceDevice(static_cast<ID3D12GraphicsCommandList*>(command),s->device))return -2;
 if(!texture(*s,static_cast<ID3D12Resource*>(depth),c.width,c.height,DXGI_FORMAT_R32_FLOAT)||!texture(*s,static_cast<ID3D12Resource*>(motion),c.width,c.height,DXGI_FORMAT_R16G16_FLOAT))return -3;
 const float values[]={p->frameTimeMs,p->worldToMeters,c.nearPlane,c.verticalFOV,c.aspectRatio,c.jitter[0],c.jitter[1]};for(float f:values)if(!std::isfinite(f))return -4;
 for(auto* matrix:{c.viewToClip,c.clipToView,c.clipToPrevious,c.previousToClip})for(unsigned i=0;i<16;++i)if(!std::isfinite(matrix[i]))return -4;
 if(std::isnan(c.farPlane)||c.farPlane<=c.nearPlane||p->frameTimeMs<=0||p->frameTimeMs>1000||p->worldToMeters<=0||c.nearPlane<=0||c.verticalFOV<=0||c.verticalFOV>=3.141593f||c.aspectRatio<=0)return -4;
 if(s->guides.camera.width&&(s->guides.camera.width!=c.width||s->guides.camera.height!=c.height))s->invalidate();
 if((s->engine!=UINT64_MAX&&c.frame<=s->engine)||(s->previous&&c.frame<=s->previous)){s->invalidate();return -5;}s->inputs();s->depth=static_cast<ID3D12Resource*>(depth);s->motion=static_cast<ID3D12Resource*>(motion);s->depth->AddRef();s->motion->AddRef();s->guides=*p;s->engine=c.frame;return 0;
}
extern "C" int mcd2_afg_scene_v1(void* device,void* world,uint64_t frame){std::lock_guard lock(gate());auto* s=current();if(!s||!s->depth||frame!=s->engine||!same(static_cast<IUnknown*>(device),s->device)||!texture(*s,static_cast<ID3D12Resource*>(world),s->output.width,s->output.height,DXGI_FORMAT(s->output.sceneFormat)))return -1;
 drop(s->scene);s->scene=static_cast<ID3D12Resource*>(world);s->scene->AddRef();return 0;
}
extern "C" int mcd2_afg_world_v2(void* command,void* world,uint64_t frame){std::lock_guard lock(gate());auto* s=current();if(!s||!command||!resourceDevice(static_cast<ID3D12GraphicsCommandList*>(command),s->device))return -1;return mcd2_afg_scene_v1(s->device,world,frame);}
extern "C" int mcd2_afg_present_v2(void* queue,uint64_t frame,uint32_t wanted){std::lock_guard lock(gate());auto* s=current();if(!s||s->closing||wanted>1)return -1;SessionLease lease(*s);
 if(!wanted){s->invalidate();if(!s->uncertain)s->inputs();return 0;}
 auto arrival=nowNs();if(!waitWorker(*s)||FAILED(s->workerResult))return MCD2_AFG_BUSY_V2;
 if(s->nativeRoute){s->invalidate();if(!s->uncertain)s->inputs();return MCD2_AFG_BUSY_V2;}
 if(!same(static_cast<IUnknown*>(queue),s->queue)||s->pending||s->nonblockCopy||s->fault||!s->outputValid||!s->depth||!s->motion||!s->scene||frame!=s->engine||s->binding.engineFrame!=frame||!s->strategy){s->invalidate();if(!s->uncertain)s->inputs();return -41;}
 if(GetForegroundWindow()!=s->window){s->invalidate();s->inputs();return MCD2_AFG_BUSY_V2;}
 s->arrival=arrival;
 stateIdentity(*s);if(!s->identity)return fail(*s,-42);
 auto& r=runtime();auto plan=make_allocation_plan(s->dlssGenerated,s->fsrGenerated,{r.softwareReady?r.softwareMax+1:0,r.nativeReady?r.nativeMax+1:0,r.nativeReady},s->order);if(!plan||plan->multiplier!=s->multiplier)return fail(*s,-43);s->plan=*plan;
 if(s->previous&&frame!=s->previous+1)s->invalidate();s->identity.epoch=s->epoch;
 if(s->guides.camera.reset){s->invalidate();s->identity.epoch=s->epoch;}
 s->cadence.observe(arrival,s->lastOwnedNs);s->pair={s->epoch,s->previous,frame};s->state.previousReal=s->previous;s->state.currentReal=frame;s->state.epoch=s->epoch;s->state.identity=s->binding.identity;s->state.observedAtMs=GetTickCount64();s->state.active=0;++s->ordinal;
 if(!features(*s)||!s->begin())return fail(*s,-44);
 if(s->converted.depth)s->converted.record(s->device,s->command,s->depth,s->motion,s->guides.camera.width,s->guides.camera.height);
 auto* bb=s->buffers[s->logicalIndex];transition(s->command,bb,D3D12_RESOURCE_STATE_PRESENT,Read);
 bool publish=s->warmup>=2&&s->previous&&s->transaction.begin(s->identity,s->pair,s->plan);
 for(unsigned slot=1;slot<=s->plan.count;++slot){bool native=s->plan.phases[slot-1].provider==Provider::NvidiaNative;auto& p=native?*r.native.params:static_cast<NVSDK_NGX_Parameter&>(r.softParams);parameters(*s,p,native,slot);
  auto result=native?r.native.evaluate(s->command,r.native.feature,&p,nullptr):r.software.evaluate_recording(s->command,&p);
  unsigned flush{};bool ok=result==NVSDK_NGX_Result_Success&&(native||(p.Get("DLSSG.FlushRequired",&flush)==NVSDK_NGX_Result_Success&&!flush));
  if(!ok)return fail(*s,-45);
  if(publish&&!s->transaction.record(s->pair,slot,s->plan.phases[slot-1].provider,reinterpret_cast<uint64_t>(s->outputs[slot-1]),true))return fail(*s,-46);
  // Each phase may share provider-internal history and OutputReal storage.
  D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;s->command->ResourceBarrier(1,&barrier);
 }
 transition(s->command,bb,Read,D3D12_RESOURCE_STATE_PRESENT);if(!s->submit())return fail(*s,-47);
 if(publish){Completion ticket{reinterpret_cast<uint64_t>(s->queue),reinterpret_cast<uint64_t>(s->fence),s->lastWork};if(!s->transaction.submitted(ticket)||!s->transaction.completed(ticket,s->fence->GetCompletedValue()))return fail(*s,-48);
  for(unsigned i=0;i<s->plan.count;++i){if(s->plan.phases[i].provider==Provider::Software)++s->state.softwareGenerated;else ++s->state.nativeGenerated;}s->pending=true;}
 ++s->warmup;++s->prepared;s->images+=s->plan.count;s->previous=frame;s->historyLost=false;s->inputs();return 0;
}
extern "C" int mcd2_afg_diagnostic_bind_v1(const MCD2FgDiagnosticBindingV1* binding){if(!binding||binding->size!=sizeof(*binding)||binding->reserved)return -1;std::lock_guard lock(gate());if(!current())return -2;current()->binding=*binding;stateIdentity(*current());return 0;}
extern "C" int mcd2_mfg_state_v1(MCD2MfgStateV1* out){if(!out||out->size!=sizeof(*out))return -1;std::lock_guard lock(gate());auto* s=current();if(!s)return -2;*out=s->state;out->ready=s->hasFeatures?1u:0u;out->fault=s->fault?s->fault:s->timingFault;out->strategy=s->strategy;out->multiplier=s->multiplier;if(GetTickCount64()-out->observedAtMs>1500)out->active=0;return 0;}
extern "C" int mcd2_mfg_timing_v1(MCD2MfgTimingV1* out){if(!out||out->size!=sizeof(*out))return -1;std::lock_guard lock(gate());auto* s=current();if(!s)return -2;
 *out={sizeof(*out),s->nativeRoute?0u:FAILED(s->workerResult)?4u:!s->refreshNs?3u:s->timingFault==315?5u:!s->schedule?0u:s->schedule.limited?2u:1u,s->refreshNs?uint32_t(1000000000000ull/s->refreshNs):0u,s->schedule.sync,s->cadence.period(),s->schedule.step*s->schedule.count,s->lastPresentation,s->jobAccepted,s->jobCompleted};return 0;}
extern "C" int mcd2_afg_diagnostics_v2(MCD2FgDiagnosticsV2* out){if(!out||out->size!=sizeof(*out))return -1;MCD2MfgStateV1 state{sizeof(state)};auto result=mcd2_mfg_state_v1(&state);if(result)return result;*out={sizeof(*out),state.strategy==2?1u:2u,state.active,state.fault,0,state.identity.session,state.identity.revision,state.identity.checksum,state.identity.world,0,state.observedAtMs,state.realPresented,state.softwarePresented+state.nativePresented};return 0;}
extern "C" int mcd2_afg_state_v1(MCD2AmdFgStateV1* out){if(!out||out->size!=sizeof(*out))return -1;std::lock_guard lock(gate());auto* s=current();if(!s)return -2;*out={sizeof(*out),s->hasFeatures?1u:0u,s->state.active,s->fault,0,0,s->state.currentReal,s->ordinal,s->prepared,s->images,s->allReal,s->allGenerated};if(GetTickCount64()-s->state.observedAtMs>1500)out->active=0;return 0;}
extern "C" int mcd2_afg_retire_v1(void* fence,uint64_t value){std::lock_guard lock(gate());auto* s=current();if(!s)return 0;if(s->closing)return MCD2_AFG_BUSY_V2;SessionLease lease(*s);
 if(fence){auto done=static_cast<ID3D12Fence*>(fence)->GetCompletedValue();if(done==UINT64_MAX||!value||done<value)return MCD2_AFG_BUSY_V2;}return retireFeatures(*s)?0:MCD2_AFG_BUSY_V2;
}
extern "C" int mcd2_afg_presenter_switch_state_v1(uint32_t* route,uint32_t* eligible){if(!route||!eligible)return -1;std::lock_guard lock(gate());*route=current()?(current()->nativeRoute?1u:4u):0u;*eligible=current()&&!current()->closing&&!current()->applicationHandleIssued?1u:0u;return 0;}
// The Anti-Lag bridge owns and initializes the persistent SDK context. This
// gate serializes every marker and the clear/drain acknowledgement before that
// owner may destroy it. Readiness here does not claim driver/SDK approval.
extern "C" int mcd2_afg_antilag_ready_v1(void* device){std::lock_guard lock(gate());auto* s=current();return s&&s->swap&&!s->closing&&s->strategy==3&&same(static_cast<IUnknown*>(device),s->device)?0:E_NOINTERFACE;}
extern "C" int mcd2_afg_antilag_v1(void* device,void* context,uint32_t enabled,uint32_t drain){
 if(enabled>1||drain>1||(enabled&&!context)||(drain&&(context||enabled)))return E_INVALIDARG;
 std::lock_guard lock(gate());auto* s=current();if(!s)return !context&&!enabled?0:E_NOINTERFACE;if(s->closing)return MCD2_AFG_BUSY_V2;SessionLease lease(*s);
 if(!same(static_cast<IUnknown*>(device),s->device)||(context&&(s->closing||s->strategy!=3)))return E_INVALIDARG;
 auto* typed=static_cast<AMD::AntiLag2DX12::Context*>(context);if(typed&&!typed->m_pAntiLagAPI)return E_NOINTERFACE;
 if(!waitWorker(*s))return MCD2_AFG_BUSY_V2;
 s->antiLag=typed;s->antiLagEnabled=enabled!=0;
 if(drain&&!s->drain())return MCD2_AFG_BUSY_V2;return 0;
}
