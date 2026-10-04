#pragma once
// Opaque SDK objects cross this boundary without C++ layout or slot arithmetic.
extern "C" {
void mcd2_ngx_set_ui(NVSDK_NGX_Parameter*,const char*,unsigned);
void mcd2_ngx_set_i(NVSDK_NGX_Parameter*,const char*,int);
void mcd2_ngx_set_f(NVSDK_NGX_Parameter*,const char*,float);
void mcd2_ngx_set_resource(NVSDK_NGX_Parameter*,const char*,ID3D12Resource*);
NVSDK_NGX_Result mcd2_ngx_get_ui(NVSDK_NGX_Parameter*,const char*,unsigned*);
NVSDK_NGX_Result mcd2_ngx_get_pointer(NVSDK_NGX_Parameter*,const char*,void**);
}
