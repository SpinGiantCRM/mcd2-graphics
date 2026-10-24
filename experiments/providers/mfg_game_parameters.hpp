#pragma once
#include <cmath>
#include <cstring>
#include <limits>
#include "mfg_ngx_parameters.hpp"
// The game's reversed-Z projection has an infinite far plane. Preserve this
// one semantic value through the public NGX interface; the general bounded
// parameter store continues to reject every other nonfinite numeric value.
class MfgGameParameters final:public NVSDK_NGX_Parameter {
 MfgNgxParameters values_;
 bool infiniteFar_{};
 static bool farKey(const char* key){return key&&std::strcmp(key,"DLSSG.CameraFar")==0;}
 template<class T> void set(const char* key,T value){
  if(farKey(key)){
   if constexpr(std::is_floating_point_v<T>){if(std::isinf(value)&&value>0){infiniteFar_=true;return;}}
   infiniteFar_=false;
  }
  values_.Set(key,value);
 }
 template<class T> NVSDK_NGX_Result get(const char* key,T* value)const{
  if(infiniteFar_&&farKey(key)){
   if(!value)return NVSDK_NGX_Result_FAIL_InvalidParameter;
   if constexpr(std::is_floating_point_v<T>){*value=std::numeric_limits<T>::infinity();return NVSDK_NGX_Result_Success;}
   return NVSDK_NGX_Result_FAIL_InvalidParameter;
  }
  return values_.Get(key,value);
 }
public:
 bool healthy()const{return values_.healthy();}
#define MFG_GAME_PARAMETER_TYPE(T) void Set(const char* k,T v)override{set(k,v);} NVSDK_NGX_Result Get(const char* k,T* v)const override{return get(k,v);}
 MFG_GAME_PARAMETER_TYPE(unsigned long long)
 MFG_GAME_PARAMETER_TYPE(float)
 MFG_GAME_PARAMETER_TYPE(double)
 MFG_GAME_PARAMETER_TYPE(unsigned)
 MFG_GAME_PARAMETER_TYPE(int)
 MFG_GAME_PARAMETER_TYPE(ID3D11Resource*)
 MFG_GAME_PARAMETER_TYPE(ID3D12Resource*)
 MFG_GAME_PARAMETER_TYPE(void*)
#undef MFG_GAME_PARAMETER_TYPE
 void Reset()override{infiniteFar_=false;values_.Reset();}
};
