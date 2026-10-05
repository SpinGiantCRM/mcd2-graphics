// Inspect only the two mod-owned slots. Never read account/character saves.
#include "../src/latency/display_protocol.hpp"
#include <fstream>
#include <iterator>
#include <iostream>
int main(int argc,char**argv){if(argc!=3)return 2;for(unsigned i=1;i<3;++i){std::ifstream f(argv[i],std::ios::binary);std::vector<uint8_t>b{std::istreambuf_iterator<char>(f),{}};std::map<std::string,unsigned>v;size_t h=0;if(!mcd2::display::parse(b,i==1?"DisplaySettingsSave":"DisplayRuntimeSave",v,h))return 3;std::cout<<(i==1?"Intent":"Runtime")<<"\n";for(auto&kv:v)std::cout<<kv.first<<"="<<kv.second<<"\n";}}
