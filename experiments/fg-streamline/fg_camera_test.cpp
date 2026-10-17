#include "fg_camera_math.h"
#include <cassert>
#include <array>
int main(){
 std::array<char,mcd2_fg::viewFrameOffset+sizeof(uint32_t)+1> bytes{};uint32_t frame=99,expectedFrame=42;
 assert(!mcd2_fg::view_frame(nullptr,bytes.size(),frame)&&frame==99);
 assert(!mcd2_fg::view_frame(bytes.data(),mcd2_fg::viewFrameOffset+3,frame)&&frame==99);
 assert(!mcd2_fg::view_frame(bytes.data(),bytes.size(),frame)&&frame==99);
 std::memcpy(bytes.data()+1+mcd2_fg::viewFrameOffset,&expectedFrame,sizeof(expectedFrame));
 assert(mcd2_fg::view_frame(bytes.data()+1,bytes.size()-1,frame)&&frame==42);
 expectedFrame=UINT32_MAX;std::memcpy(bytes.data()+1+mcd2_fg::viewFrameOffset,&expectedFrame,sizeof(expectedFrame));
 assert(!mcd2_fg::view_frame(bytes.data()+1,bytes.size()-1,frame)&&frame==42);
 std::array<float,632> v{};v[128]=2.41421356f;v[133]=4.29193522f;v[139]=1;v[142]=10;
 for(unsigned i=0;i<4;++i)v[544+i*5]=1;
 v[292+2]=1;v[296+1]=1;v[300]=1;v[576]=.0002f;v[577]=-.0004f;
 MCD2FGCamera c{};assert(mcd2_fg::camera(v.data(),42,2560,1440,true,c));
 assert(c.frame==42&&c.reset==1&&std::isinf(c.farPlane)&&c.nearPlane==10);
 assert(std::abs(c.aspectRatio-16.f/9)<1e-5&&std::abs(c.jitter[0]-.256f)<1e-5&&std::abs(c.jitter[1]-.288f)<1e-5);
 for(unsigned r=0;r<4;++r)for(unsigned k=0;k<4;++k){float sum=0;for(unsigned j=0;j<4;++j)sum+=c.viewToClip[r*4+j]*c.clipToView[j*4+k];assert(std::abs(sum-(r==k?1.f:0.f))<1e-5);}
 v[128]=0;assert(!mcd2_fg::camera(v.data(),42,2560,1440,false,c));v[128]=2.41421356f;
 v[136]=.001f;assert(!mcd2_fg::camera(v.data(),42,2560,1440,false,c));v[136]=0;
 v[544]=std::numeric_limits<float>::quiet_NaN();assert(!mcd2_fg::camera(v.data(),42,2560,1440,false,c));
 assert(!mcd2_fg::camera(nullptr,42,2560,1440,false,c));assert(!mcd2_fg::camera(v.data(),42,0,1440,false,c));
}
