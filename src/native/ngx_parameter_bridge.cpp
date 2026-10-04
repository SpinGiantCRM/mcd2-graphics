// Standard SDK virtual calls compiled with the runtime's Microsoft C++ ABI.
// The external boundary uses C functions and scalar/pointer types only.
#include "nvsdk_ngx_params.h"
// MSVC-target Clang emits this CRT marker for the float setter. Define it here
// so the C ABI bridge also links with LLVM MinGW's static CRT on Windows.
extern "C" { int _fltused = 0; }
extern "C" {
void mcd2_ngx_set_ui(NVSDK_NGX_Parameter*p,const char*k,unsigned v){p->Set(k,v);}
void mcd2_ngx_set_i(NVSDK_NGX_Parameter*p,const char*k,int v){p->Set(k,v);}
void mcd2_ngx_set_f(NVSDK_NGX_Parameter*p,const char*k,float v){p->Set(k,v);}
void mcd2_ngx_set_resource(NVSDK_NGX_Parameter*p,const char*k,ID3D12Resource*v){p->Set(k,v);}
NVSDK_NGX_Result mcd2_ngx_get_ui(NVSDK_NGX_Parameter*p,const char*k,unsigned*v){return p->Get(k,v);}
NVSDK_NGX_Result mcd2_ngx_get_pointer(NVSDK_NGX_Parameter*p,const char*k,void**v){return p->Get(k,v);}
}
