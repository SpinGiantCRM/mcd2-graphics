#include "../experiments/fg-streamline/fg_ui_protocol.hpp"
#include <cassert>
#include <iostream>

// Synthetic mod-only GVAS envelope. No game/account/save data is included.
std::vector<uint8_t> fixture(std::vector<std::pair<std::string,unsigned>> fields){
 std::vector<uint8_t> b;
 auto num=[&](unsigned v,unsigned n){for(unsigned i=0;i<n;++i)b.push_back(uint8_t(v>>(i*8)));};
 auto str=[&](const std::string&s){num(unsigned(s.size()+1),4);b.insert(b.end(),s.begin(),s.end());b.push_back(0);};
 num(0x53415647,4);num(3,4);num(522,4);num(1017,4);num(5,2);num(6,2);num(1,2);num(0,4);str("UE5");num(3,4);num(93,4);b.resize(b.size()+93*20);
 str("/Game/Mods/MCD2Graphics/FGSettingsSave.FGSettingsSave_C");num(0,1);
 fields.insert(fields.begin(),{{"SchemaVersion",1},{"Revision",19},{"Mode",1},{"ContextReady",1},{"SessionId",123},{"SettingsRevision",42}});
 for(auto& [name,value]:fields){str(name);str("IntProperty");num(0,4);num(4,4);num(0,1);num(value,4);}str("None");num(0,4);return b;
}
int main(){
 mcd2::fgui::Intent i;
 for(const char* field:{"FGProvider","fgProvider","Provider","provider"}){
  auto b=fixture({{field,1}});assert(mcd2::fgui::decode(b,i));
  assert(i.provider==1&&i.mode==1&&i.ready==1&&i.session==123&&i.settingsRevision==42);
  for(size_t n=0;n<b.size();++n){auto truncated=b;truncated.resize(n);assert(!mcd2::fgui::decode(truncated,i));}
  b.push_back(0);assert(!mcd2::fgui::decode(b,i));
 }
 for(const std::string base:{"fgprovider","provider"}){
  for(unsigned mask=0;mask<(1u<<base.size());++mask){auto name=base;
   for(unsigned n=0;n<name.size();++n)if(mask&(1u<<n))name[n]=char(name[n]-'a'+'A');
   assert(mcd2::fgui::decode(fixture({{name,1}}),i)&&i.provider==1);
   assert(!mcd2::fgui::decode(fixture({{name,1},{"FGProvider",1}}),i));
  }
 }
 assert(mcd2::fgui::decode(fixture({}),i)&&i.provider==0); // Existing NVIDIA-only saves.
 for(auto fields:std::vector<std::vector<std::pair<std::string,unsigned>>>{
  {{"FGProvider",2}},{{"UnknownProvider",1}},{{"Provider",1},{"provider",1}},
  {{"FGProvider",1},{"Provider",1}},{{"FGProvider",1},{"provider",1}},{{"FGProvider",1},{"FGProvider",1}},
  {{"FGProvider",1},{"fgProvider",1}},{{"fgProvider",2}},{{"fgProvider",1},{"PROVIDER",0}}})
  assert(!mcd2::fgui::decode(fixture(fields),i));
 std::cout<<"FG provider framing, legacy FName alias, current metadata and duplicate/truncation checks pass\n";
}
