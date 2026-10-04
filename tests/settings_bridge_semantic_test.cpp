#include "settings_bridge.hpp"
#include <fstream>
#include <iterator>
#include <iostream>
#include <utility>
#include <stdexcept>
using Bytes=std::vector<uint8_t>;
Bytes read(const char*p){std::ifstream f(p,std::ios::binary);return Bytes((std::istreambuf_iterator<char>(f)),{});}
void u32(Bytes&b,uint32_t v){for(int i=0;i<4;i++)b.push_back(uint8_t(v>>(i*8)));}
void str(Bytes&b,const std::string&s){u32(b,s.size()+1);b.insert(b.end(),s.begin(),s.end());b.push_back(0);}
size_t header(const Bytes&b){mcd2ui::Reader r{b};r.at=4*4+2*3+4;r.str();r.u32();auto n=r.u32();r.at+=n*20;r.str();r.u8();return r.at;}
Bytes make(const Bytes&original,std::vector<std::pair<std::string,uint32_t>> props,bool fallback=false){Bytes b(original.begin(),original.begin()+header(original));for(auto &[n,v]:props){str(b,n);str(b,"IntProperty");u32(b,0);u32(b,4);b.push_back(0);u32(b,v);}if(fallback){str(b,"NativeFallback");str(b,"BoolProperty");u32(b,0);u32(b,0);b.push_back(0x10);}str(b,"None");u32(b,0);return b;}
int main(int argc,char**argv){if(argc!=3)return 1;auto settings=read(argv[1]),runtime=read(argv[2]);unsigned checks=0;auto check=[&](bool v){++checks;if(!v)throw std::runtime_error("semantic check "+std::to_string(checks));};mcd2ui::Settings s;mcd2ui::RuntimeState r;
check(mcd2ui::decode(settings,s));check(mcd2ui::decode_runtime(runtime,r));
auto ps=std::vector<std::pair<std::string,uint32_t>>{{"SchemaVersion",1},{"Revision",22},{"ReconstructionMode",2},{"RenderScaleBasisPoints",6667},{"AppliedSourceRevision",21},{"AppliedSourceSessionId",345}};
check(mcd2ui::decode(make(settings,ps,true),s));
for(auto pair:std::vector<std::pair<int,uint32_t>>{{0,5},{1,0},{1,0xffffffff},{2,3},{3,10000},{4,23},{5,0xffffffff}}){auto p=ps;p[pair.first].second=pair.second;check(!mcd2ui::decode(make(settings,p,true),s));}
auto p=ps;p.push_back(ps[0]);check(!mcd2ui::decode(make(settings,p,true),s));p=ps;p.push_back({"FutureInt",99});check(mcd2ui::decode(make(settings,p,true),s));
auto v2=ps;v2[0].second=2;v2.push_back({"SRPreset",0});v2.push_back({"CustomScaleBasisPoints",6700});
for(auto preset:std::vector<std::pair<uint32_t,uint32_t>>{{0,6667},{1,5800},{2,5000},{3,3333},{4,6700}}){auto p=v2;p[6].second=preset.first;p[3].second=preset.second;check(mcd2ui::decode(make(settings,p,true),s));p[3].second++;check(!mcd2ui::decode(make(settings,p,true),s));}
for(auto value:{4900u,10001u,6751u}){auto p=v2;p[7].second=value;check(!mcd2ui::decode(make(settings,p,true),s));}
auto v3=v2;v3[0].second=3;v3[6].second=4;
for(auto value:{100u,1700u,6700u,9900u}){auto p=v3;p[7].second=value;p[3].second=value;check(mcd2ui::decode(make(settings,p,true),s));}
for(auto value:{0u,99u,10000u,9901u}){auto p=v3;p[7].second=value;p[3].second=value;check(!mcd2ui::decode(make(settings,p,true),s));}
auto v4=v3;v4[0].second=4;
for(auto preset:std::vector<std::pair<uint32_t,uint32_t>>{{5,10000},{0,6700},{1,5800},{2,5000},{3,3300},{4,9900}}){auto p=v4;p[6].second=preset.first;p[7].second=preset.second;p[2].second=preset.first==5?1:2;p[3].second=preset.first==0?6667:(preset.first==3?3333:preset.second);check(mcd2ui::decode(make(settings,p,true),s));p[2].second=0;p[3].second=10000;check(mcd2ui::decode(make(settings,p,true),s));p[7].second=preset.second==10000?9900:10000;check(!mcd2ui::decode(make(settings,p,true),s));}
for(auto value:{100u,1700u,7700u,9900u}){auto p=v4;p[7].second=value;p[3].second=value;check(mcd2ui::decode(make(settings,p,true),s));}
for(auto value:{0u,99u,3300u,5000u,5800u,6700u,10000u,10001u}){auto p=v4;p[7].second=value;p[3].second=value;check(!mcd2ui::decode(make(settings,p,true),s));}
auto bad=v4;bad[6].second=5;bad[7].second=10000;bad[3].second=10000;check(!mcd2ui::decode(make(settings,bad,true),s));bad[6].second=0;bad[7].second=6700;bad[2].second=1;check(!mcd2ui::decode(make(settings,bad,true),s));
// Context authorization is optional for old saves and fails closed at the renderer.
v4[7].second=7700;v4[3].second=7700;
check(mcd2ui::decode(make(settings,v4,true),s));check(s.contextReady==0 && s.contextSession==0);
for(auto ready:{0u,1u}){auto p=v4;p.push_back({"RenderContextReady",ready});p.push_back({"RenderContextSessionId",345});check(mcd2ui::decode(make(settings,p,true),s));check(s.contextReady==ready && s.contextSession==345);}
for(auto value:{2u,0xffffffffu}){auto p=v4;p.push_back({"RenderContextReady",value});check(!mcd2ui::decode(make(settings,p,true),s));}
{auto p=v4;p.push_back({"RenderContextSessionId",0xffffffffu});check(!mcd2ui::decode(make(settings,p,true),s));}
auto pr=std::vector<std::pair<std::string,uint32_t>>{{"SchemaVersion",1},{"RequestedRevision",22},{"SessionId",345},{"Phase",2},{"ErrorCode",0}};
check(mcd2ui::decode_runtime(make(runtime,pr),r));auto limit=pr;limit[3].second=3;limit[4].second=2;check(mcd2ui::decode_runtime(make(runtime,limit),r));
for(auto pair:std::vector<std::pair<int,uint32_t>>{{0,2},{1,0},{1,0xffffffff},{2,0},{3,6},{4,3}}){auto p=pr;p[pair.first].second=pair.second;check(!mcd2ui::decode_runtime(make(runtime,p),r));}
p=pr;p.push_back(pr[0]);check(!mcd2ui::decode_runtime(make(runtime,p),r));p=pr;p.push_back({"FutureInt",99});check(mcd2ui::decode_runtime(make(runtime,p),r));check(!mcd2ui::decode_runtime(make(runtime,pr,true),r));
for(size_t i=0;i<runtime.size();++i){Bytes b(runtime.begin(),runtime.begin()+i);check(!mcd2ui::decode_runtime(b,r));}
auto b=runtime;b.push_back(0);check(!mcd2ui::decode_runtime(b,r));b=runtime;b[0]^=1;check(!mcd2ui::decode_runtime(b,r));
s.mode=1;check(mcd2ui::source_ratio(s,3840,2160,3840,2160));check(!mcd2ui::source_ratio(s,2560,1440,3840,2160));s.mode=2;s.preset=0;s.scale=6667;check(mcd2ui::source_ratio(s,2560,1440,3840,2160));check(!mcd2ui::source_ratio(s,3840,2160,3840,2160));s.preset=4;s.scale=8400;check(mcd2ui::source_ratio(s,3228,1816,3840,2160));check(!mcd2ui::source_ratio(s,3840,2160,3840,2160));
std::cout<<"Settings/runtime semantic and truncation checks PASS: "<<checks<<"\n";
}
