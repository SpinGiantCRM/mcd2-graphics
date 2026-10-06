#pragma once
#include "fg_camera_contract.h"
#include <cmath>
#include <cstring>
#include <limits>
namespace mcd2_fg {
inline bool inverse(const float* in,float* out){
 double work[4][8]{};
 for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c){if(!std::isfinite(in[r*4+c]))return false;work[r][c]=in[r*4+c];work[r][c+4]=r==c?1.:0.;}
 for(unsigned c=0;c<4;++c){unsigned pivot=c;for(unsigned r=c+1;r<4;++r)if(std::abs(work[r][c])>std::abs(work[pivot][c]))pivot=r;
  if(std::abs(work[pivot][c])<1e-12)return false;
  for(unsigned k=0;k<8;++k){double x=work[c][k];work[c][k]=work[pivot][k];work[pivot][k]=x;}
  const double d=work[c][c];for(double&v:work[c])v/=d;
  for(unsigned r=0;r<4;++r)if(r!=c){const double f=work[r][c];for(unsigned k=0;k<8;++k)work[r][k]-=f*work[c][k];}
 }
 for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c){out[r*4+c]=float(work[r][c+4]);if(!std::isfinite(out[r*4+c]))return false;}
 return true;
}
inline bool camera(const float* view,uint32_t stamp,uint32_t width,uint32_t height,bool reset,MCD2FGCamera& out){
 if(!view||!width||!height)return false;
 out={};out.size=sizeof(out);out.frame=stamp;out.width=width;out.height=height;out.reset=reset;
 // Build 25647713 ViewToClipNoAA, reversed-Z infinite perspective.
 std::memcpy(out.viewToClip,view+128,64);std::memcpy(out.clipToPrevious,view+544,64);
 if(!inverse(out.viewToClip,out.clipToView)||!inverse(out.clipToPrevious,out.previousToClip))return false;
 const auto*p=out.viewToClip;
 if(p[0]<=0||p[5]<=0||std::abs(p[10])>1e-6||std::abs(p[11]-1)>1e-6||p[14]<=0||std::abs(p[15])>1e-6||std::abs(p[8])>1e-6||std::abs(p[9])>1e-6)return false;
 for(unsigned i=0;i<3;++i){out.position[i]=view[288+i];out.forward[i]=view[292+i];out.up[i]=view[296+i];out.right[i]=view[300+i];
  if(!std::isfinite(out.position[i])||!std::isfinite(out.forward[i])||!std::isfinite(out.up[i])||!std::isfinite(out.right[i]))return false;}
 out.jitter[0]=view[576]*width*.5f;out.jitter[1]=-view[577]*height*.5f;
 if(!std::isfinite(out.jitter[0])||!std::isfinite(out.jitter[1]))return false;
 out.nearPlane=p[14];out.farPlane=std::numeric_limits<float>::infinity();out.verticalFOV=2*std::atan(1/p[5]);out.aspectRatio=p[5]/p[0];
 return true;
}
}
