#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cassert>
#include "../experiments/providers/mfg_ngx_parameters.hpp"
int main() {
    MfgNgxParameters own; NVSDK_NGX_Parameter* p=&own;
    unsigned u=0; int i=0; float f=0; double d=0; unsigned long long ull=0;
    p->Set("u",3u); p->Set("i",-2); p->Set("f",1.25f);
    p->Set("d",2.5); p->Set("ull",~0ull);
    assert(p->Get("u",&u)==NVSDK_NGX_Result_Success && u==3);
    assert(p->Get("i",&i)==NVSDK_NGX_Result_Success && i==-2);
    assert(p->Get("f",&f)==NVSDK_NGX_Result_Success && f==1.25f);
    assert(p->Get("d",&d)==NVSDK_NGX_Result_Success && d==2.5);
    assert(p->Get("ull",&ull)==NVSDK_NGX_Result_Success && ull==~0ull);
    auto resource=reinterpret_cast<ID3D12Resource*>(uintptr_t(0x1234));
    p->Set("resource",resource); void* out=nullptr; ID3D12Resource* r=nullptr;
    assert(p->Get("resource",&out)==NVSDK_NGX_Result_Success && out==resource);
    assert(p->Get("resource",&r)==NVSDK_NGX_Result_Success && r==resource);
    auto d11=reinterpret_cast<ID3D11Resource*>(uintptr_t(0x4321)); ID3D11Resource* r11=nullptr;
    p->Set("d11",d11); assert(p->Get("d11",&r11)==NVSDK_NGX_Result_Success && r11==d11);
    p->Set("pointer",out); assert(p->Get("pointer",&r)==NVSDK_NGX_Result_Success && r==resource);
    assert(p->Get("resource",&u)==NVSDK_NGX_Result_FAIL_InvalidParameter);
    assert(p->Get("missing",&u)==NVSDK_NGX_Result_FAIL_InvalidParameter);
    assert(p->Get("resource",static_cast<ID3D12Resource**>(nullptr))==NVSDK_NGX_Result_FAIL_InvalidParameter);
    assert(p->Get("u",static_cast<unsigned*>(nullptr))==NVSDK_NGX_Result_FAIL_InvalidParameter);
    p->Set(nullptr,1u); assert(!own.healthy());
    p->Reset(); assert(own.healthy());
    assert(p->Get("resource",&r)==NVSDK_NGX_Result_FAIL_InvalidParameter);
}
