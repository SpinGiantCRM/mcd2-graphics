#include "../src/latency/display_protocol.hpp"
#include <fstream>
#include <iterator>
#include <cassert>
#include <iostream>
std::vector<uint8_t> read(const char*p){std::ifstream f(p,std::ios::binary);assert(f);return {std::istreambuf_iterator<char>(f),{}};}
std::vector<uint8_t> save(const std::vector<uint8_t>&seed,const char*cls,const std::map<std::string,unsigned>&v){std::map<std::string,unsigned> values;size_t header=0;assert(mcd2::display::parse(seed,cls,values,header));std::vector<uint8_t> b(seed.begin(),seed.begin()+header);auto u32=[&](unsigned n){for(unsigned k=0;k<4;k++)b.push_back(uint8_t(n>>(k*8)));};auto str=[&](std::string s){u32(s.size()+1);b.insert(b.end(),s.begin(),s.end());b.push_back(0);};for(auto&p:v){str(p.first);str("IntProperty");u32(0);u32(4);b.push_back(0);u32(p.second);}str("None");u32(0);return b;}
int main(int argc,char**argv){assert(argc==3);auto b=read(argv[1]),runtime=read(argv[2]);mcd2::display::Intent intent;assert(mcd2::display::decode(b,intent));assert(intent.revision&&intent.paper==203&&intent.ui==203);std::map<std::string,unsigned> v;size_t h=0;assert(mcd2::display::parse(b,"DisplaySettingsSave",v,h));
 unsigned count=1;
 for(auto p:std::map<std::string,unsigned>{{"SchemaVersion",2},{"Revision",0},{"HDROutput",2},{"PeakNits",99},{"PaperWhiteNits",501},{"UINits",47},{"ReflexMode",3},{"Unknown",1}}){auto altered=v;altered[p.first]=p.second;assert(!mcd2::display::decode(save(b,"DisplaySettingsSave",altered),intent));++count;}
 auto altered=v;altered["PeakNits"]=111;assert(!mcd2::display::decode(save(b,"DisplaySettingsSave",altered),intent));++count;
 for(size_t n=0;n<b.size();++n){auto shortB=b;shortB.resize(n);assert(!mcd2::display::decode(shortB,intent));}++count;
 auto trailing=b;trailing.push_back(0);assert(!mcd2::display::decode(trailing,intent));++count;
 auto encoded=mcd2::display::encode_runtime(runtime,{{"SchemaVersion",1},{"SessionId",123},{"Revision",3},{"ReflexMode",2},{"HDRRestartRequired",1}});v.clear();assert(mcd2::display::parse(encoded,"DisplayRuntimeSave",v,h));assert(v["ReflexMode"]==2&&v["HDRRestartRequired"]==1);++count;
 encoded=mcd2::display::encode_runtime(runtime,{{"SchemaVersion",1},{"SessionId",123},{"Revision",9},{"AmdAntiLagAvailable",1},{"AmdAntiLagMode",1},{"AmdAntiLagFault",0},{"AmdAntiLagRevision",9}});v.clear();assert(mcd2::display::parse(encoded,"DisplayRuntimeSave",v,h));assert(v["AmdAntiLagAvailable"]==1&&v["AmdAntiLagMode"]==1&&v["AmdAntiLagFault"]==0&&v["AmdAntiLagRevision"]==9);++count;
 altered.clear();assert(!mcd2::display::parse(runtime,"DisplaySettingsSave",altered,h));++count;
 std::cout<<count<<" display protocol checks passed\n";
}
