#include "../src/gdk/save_slots.hpp"
#include <cassert>
#include <fstream>
#include <filesystem>
#include <iterator>
std::vector<uint8_t> load(const std::filesystem::path& path){std::ifstream f(path,std::ios::binary);assert(f);return {std::istreambuf_iterator<char>(f),{}};}
std::vector<uint8_t> field(std::vector<uint8_t> bytes,const std::string& name,unsigned value){
 // The synthetic save ends with the None FString and a zero terminator.
 assert(bytes.size()>13);bytes.resize(bytes.size()-13);
 auto u32=[&](unsigned v){for(unsigned n=0;n<4;++n)bytes.push_back(uint8_t(v>>(n*8)));};
 auto str=[&](const std::string& s){u32(unsigned(s.size()+1));bytes.insert(bytes.end(),s.begin(),s.end());bytes.push_back(0);};
 str(name);str("IntProperty");u32(0);u32(4);bytes.push_back(0);u32(value);str("None");u32(0);return bytes;
}
int main(int argc,char** argv){
 assert(argc==2);auto root=std::filesystem::path(argv[1]);auto settings=load(root/"fg-settings.sav"),runtime=load(root/"fg-runtime.sav");
 mcd2::fgui::Intent out{};assert(mcd2::fgui::decode(settings,out)&&out.frameMode==0);
 for(unsigned mode=0;mode<=5;++mode){auto saved=field(settings,"FrameMode",mode);
  assert(mcd2::fgui::decode(saved,out)&&out.frameMode==mode);assert(mcd2::gdk::valid(mcd2::gdk::slots[2],saved));
  for(const auto* name:{"frameMode","FRAMEMODE","framemode"}){
   auto variant=field(settings,name,mode);
   assert(mcd2::fgui::decode(variant,out)&&out.frameMode==mode);
   assert(mcd2::gdk::valid(mcd2::gdk::slots[2],variant));
   assert(!mcd2::fgui::decode(field(saved,name,mode),out));
  }
 }
 assert(!mcd2::fgui::decode(field(settings,"FrameMode",6),out));
 assert(!mcd2::fgui::decode(field(settings,"frameMode",6),out));
 assert(!mcd2::gdk::valid(mcd2::gdk::slots[2],field(settings,"FrameMode",6)));
 auto caps=mcd2::fgui::encode(runtime,{{"SchemaVersion",1},{"MaximumGeneratedFrames",5},{"DynamicSupported",1},{"ActiveFrameMode",5}});
 assert(mcd2::gdk::valid(mcd2::gdk::slots[5],caps));
 for(const auto& name:{"MaximumGeneratedFrames","DynamicSupported","ActiveFrameMode"}){
  auto bad=field(runtime,name,17);assert(!mcd2::gdk::valid(mcd2::gdk::slots[5],bad));
 }
}
