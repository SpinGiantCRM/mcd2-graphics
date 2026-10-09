// Synthetic FSR presentation transport. Never package or install this module.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include "../third-party/AntiLag2/ffx_antilag2_dx12.h"
#include "../src/latency/amd_fg_private_data.hpp"
#include <cassert>
#include <cstring>
namespace {
AMD::AntiLag2DX12::Context *current=nullptr,*inFlight=nullptr;
bool enabled=false,failDrain=false,available=true;
unsigned publishes=0,drains=0;
struct Swap : IDXGIObject {
 HRESULT STDMETHODCALLTYPE QueryInterface(REFIID,void**)override{return E_NOINTERFACE;}
 ULONG STDMETHODCALLTYPE AddRef()override{return 1;}
 ULONG STDMETHODCALLTYPE Release()override{return 1;}
 HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID guid,UINT size,const void* bytes)override{
  assert(guid==mcd2::amd_fg::antiLagDataGuid&&size==sizeof(mcd2::amd_fg::AntiLagData));
  mcd2::amd_fg::AntiLagData data{};std::memcpy(&data,bytes,size);
  current=static_cast<AMD::AntiLag2DX12::Context*>(data.context);enabled=data.enabled;++publishes;return S_OK;
 }
 HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID,const IUnknown*)override{return E_NOTIMPL;}
 HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID,UINT*,void*)override{return E_NOTIMPL;}
 HRESULT STDMETHODCALLTYPE GetParent(REFIID,void**)override{return E_NOTIMPL;}
} swap;
struct OfficialShape {AMD::AntiLag2DX12::Context* context;bool enabled;};
static_assert(sizeof(OfficialShape)==sizeof(mcd2::amd_fg::AntiLagData));
}
extern "C" __declspec(dllexport) int mcd2_afg_antilag_ready_v1(void* device){return available&&device==reinterpret_cast<void*>(123)?0:-1;}
extern "C" __declspec(dllexport) int mcd2_afg_antilag_v1(void* device,void* context,unsigned mode,unsigned drain){
 if(device!=reinterpret_cast<void*>(123)||mode>1||drain>1||(drain&&(context||mode)))return E_INVALIDARG;
 if(context){assert(available);inFlight=static_cast<AMD::AntiLag2DX12::Context*>(context);}
 const auto result=mcd2::amd_fg::publish(&swap,context,mode!=0);if(FAILED(result))return result;
 if(drain){++drains;if(failDrain)return -1;inFlight=nullptr;}
 return 0;
}
extern "C" __declspec(dllexport) int test_fg_present(unsigned generated){
 auto context=current?current:inFlight;
 return context?AMD::AntiLag2DX12::SetFrameGenFrameType(context,generated!=0):E_NOINTERFACE;
}
extern "C" __declspec(dllexport) void test_fg_drain_failure(unsigned fail){failDrain=fail!=0;}
extern "C" __declspec(dllexport) unsigned test_fg_count(unsigned field){return field==0?publishes:field==1?drains:enabled?1u:0u;}
