// Synthetic public-SDK fixture only. Never install this DLL in a game.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include "../third-party/AntiLag2/ffx_antilag2_dx12.h"
#include <array>
namespace {
std::array<unsigned,8> counts{};bool failDelay=false;
struct Fake : AMD::AntiLag2DX12::IAmdExtAntiLagApi {
    ULONG references=0;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID,void**) override { return E_NOINTERFACE; }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++references; }
    ULONG STDMETHODCALLTYPE Release() override {++counts[6];return --references;}
    HRESULT UpdateAntiLagState(void* data) override {
        if(!data){++counts[3];return failDelay?E_FAIL:S_FALSE;}
        auto v=static_cast<AMD::AntiLag2DX12::APIData_v1*>(data);
        if(v->uiVersion==1){
            if(v->uiSize!=sizeof(*v)||v->maxFPS||v->sControlStr||v->uiControlStrLength)return E_INVALIDARG;
            if(v->eMode==1)++counts[1];else if(v->eMode==2)++counts[2];else return E_INVALIDARG;
            return S_OK;
        }
        auto f=static_cast<AMD::AntiLag2DX12::APIData_v2*>(data);
        if(f->uiVersion!=2||f->uiSize!=sizeof(*f)||f->iiFrameIdx)return E_INVALIDARG;
        if(f->flags.signalEndOfFrameIdx)++counts[4];
        else if(f->flags.signalFgFrameType&&!f->flags.isInterpolatedFrame)++counts[5];
        else return E_INVALIDARG;
        return S_OK;
    }
} fake;
}
extern "C" __declspec(dllexport) HRESULT __cdecl AmdExtD3DCreateInterface(IUnknown* device,REFIID iid,void** out){
    if(!device||!out||iid!=__uuidof(AMD::AntiLag2DX12::IAmdExtAntiLagApi))return E_NOINTERFACE;
    ++counts[0];fake.AddRef();*out=&fake;return S_OK;
}
extern "C" __declspec(dllexport) unsigned test_count(unsigned index){return index<counts.size()?counts[index]:0;}
extern "C" __declspec(dllexport) void test_fail_delay(unsigned fail){failDelay=fail!=0;}
