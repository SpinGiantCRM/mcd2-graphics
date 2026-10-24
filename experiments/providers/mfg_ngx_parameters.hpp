#pragma once
#include "nvsdk_ngx_params.h"
#include "../../src/native/mfg_parameters.hpp"

// Compiled only with the Microsoft C++ ABI, against external SDK declarations.
// No third-party implementation, virtual-slot arithmetic or CRT object crosses
// our module boundary. The SDK's public parameter interface is the ABI.
class MfgNgxParameters final : public NVSDK_NGX_Parameter {
    mcd2::mfg::Parameters data_;
    bool healthy_=true;
    template<class T> void put(const char* key,T value) { if(!data_.set(key,value)) healthy_=false; }
    template<class T> NVSDK_NGX_Result read(const char* key,T* value) const {
        return value && data_.get(key,*value) ? NVSDK_NGX_Result_Success : NVSDK_NGX_Result_FAIL_InvalidParameter;
    }
public:
    bool healthy() const { return healthy_; }
    void Set(const char* k,unsigned long long v) override { put(k,v); }
    void Set(const char* k,float v) override { put(k,v); }
    void Set(const char* k,double v) override { put(k,v); }
    void Set(const char* k,unsigned v) override { put(k,v); }
    void Set(const char* k,int v) override { put(k,v); }
    void Set(const char* k,ID3D11Resource* v) override { put(k,static_cast<void*>(v)); }
    void Set(const char* k,ID3D12Resource* v) override { put(k,static_cast<void*>(v)); }
    void Set(const char* k,void* v) override { put(k,v); }
    NVSDK_NGX_Result Get(const char* k,unsigned long long* v) const override { return read(k,v); }
    NVSDK_NGX_Result Get(const char* k,float* v) const override { return read(k,v); }
    NVSDK_NGX_Result Get(const char* k,double* v) const override { return read(k,v); }
    NVSDK_NGX_Result Get(const char* k,unsigned* v) const override { return read(k,v); }
    NVSDK_NGX_Result Get(const char* k,int* v) const override { return read(k,v); }
    NVSDK_NGX_Result Get(const char* k,ID3D11Resource** v) const override {
        void* p=nullptr; auto r=read(k,&p); if(v && r==NVSDK_NGX_Result_Success) *v=static_cast<ID3D11Resource*>(p);
        return v ? r : NVSDK_NGX_Result_FAIL_InvalidParameter;
    }
    NVSDK_NGX_Result Get(const char* k,ID3D12Resource** v) const override {
        void* p=nullptr; auto r=read(k,&p); if(v && r==NVSDK_NGX_Result_Success) *v=static_cast<ID3D12Resource*>(p);
        return v ? r : NVSDK_NGX_Result_FAIL_InvalidParameter;
    }
    NVSDK_NGX_Result Get(const char* k,void** v) const override { return read(k,v); }
    void Reset() override { data_.reset(); healthy_=true; }
};
