#include "fg_ui_protocol.hpp"
#include <cassert>
#include <fstream>
#include <iterator>
#include <iostream>
std::vector<uint8_t> load(const char*p){std::ifstream f(p,std::ios::binary);assert(f);return {std::istreambuf_iterator<char>(f),{}};}
int main(int argc,char**argv){assert(argc==3);auto b=load(argv[1]),runtime=load(argv[2]);mcd2::fgui::Intent out{};assert(mcd2::fgui::decode(b,out));assert(out.mode==1&&out.ready==1&&out.session==123);for(size_t n=0;n<b.size();++n){auto shortB=b;shortB.resize(n);assert(!mcd2::fgui::decode(shortB,out));}auto trailing=b;trailing.push_back(0);assert(!mcd2::fgui::decode(trailing,out));assert(!mcd2::fgui::decode(runtime,out));auto encoded=mcd2::fgui::encode(runtime,{{"SchemaVersion",1},{"Revision",3},{"SessionId",123},{"Available",1},{"Active",1},{"Phase",3},{"RestartRequired",1}});std::map<std::string,unsigned> v;size_t h=0;assert(mcd2::display::parse(encoded,"FGRuntimeSave",v,h));assert(v["Active"]==1&&v["Available"]==1&&v["RestartRequired"]==1);assert(mcd2::fgui::encode(runtime,{{"Unknown",1}}).empty());std::cout<<"FG UI protocol framing, truncation and runtime checks passed\n";}
