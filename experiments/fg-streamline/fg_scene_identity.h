#pragma once
#include "fg_camera_contract.h"
#include <cmath>
#include <cstring>
#include <initializer_list>
namespace mcd2_fg {
// The scene parser decodes jitter at 1x1; validate it against the independently
// produced guides at their actual render size. CPU recording order is irrelevant.
inline bool same_scene_view(const MCD2FGCamera&scene,const MCD2FGCamera&expected){
 if(scene.size!=sizeof(scene)||expected.size!=sizeof(expected)||scene.width!=1||scene.height!=1||!expected.width||!expected.height||scene.frame>=UINT32_MAX||scene.frame!=expected.frame)return false;
 auto decoded=scene;decoded.jitter[0]*=expected.width;decoded.jitter[1]*=expected.height;
 const MCD2FGCamera*cameras[]{&decoded,&expected};
 for(const auto*camera:cameras){
  for(const auto*vector:{camera->position,camera->forward,camera->up,camera->right})for(unsigned i=0;i<3;++i)if(!std::isfinite(vector[i]))return false;
  for(const auto*matrix:{camera->viewToClip,camera->clipToPrevious})for(unsigned i=0;i<16;++i)if(!std::isfinite(matrix[i]))return false;
  for(float value:camera->jitter)if(!std::isfinite(value))return false;
 }
 return !memcmp(decoded.position,expected.position,sizeof(decoded.position))&&!memcmp(decoded.forward,expected.forward,sizeof(decoded.forward))&&!memcmp(decoded.up,expected.up,sizeof(decoded.up))&&!memcmp(decoded.right,expected.right,sizeof(decoded.right))&&!memcmp(decoded.viewToClip,expected.viewToClip,sizeof(decoded.viewToClip))&&!memcmp(decoded.clipToPrevious,expected.clipToPrevious,sizeof(decoded.clipToPrevious))&&!memcmp(decoded.jitter,expected.jitter,sizeof(decoded.jitter));
}
}
