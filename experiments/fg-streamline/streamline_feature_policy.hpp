#pragma once
#include <array>
#include <cstdint>
namespace mcd2::fg {
// AMD presentation needs Reflex/PCL on NVIDIA, but not the DLSS-G plugin or
// its NGX instance. Legacy/unowned startups retain their existing feature set.
constexpr bool loadNvidiaFg(unsigned startupOwner) { return startupOwner != 2; }
struct AdapterIdentity {
 std::uint32_t low=0;std::int32_t high=0;
 bool operator==(const AdapterIdentity&) const = default;
};
class NvidiaFgCapabilities {
 struct Entry {AdapterIdentity adapter;int result;};
 std::array<Entry,16> entries{};unsigned count=0;
public:
 void clear(){count=0;}
 bool remember(AdapterIdentity adapter,int result){
  for(unsigned i=0;i<count;++i)if(entries[i].adapter==adapter){entries[i].result=result;return true;}
  if(count==entries.size())return false;
  entries[count++]={adapter,result};return true;
 }
 int find(AdapterIdentity adapter) const {
  for(unsigned i=0;i<count;++i)if(entries[i].adapter==adapter)return entries[i].result;
  return -1; // No inferred support for an untested adapter.
 }
};
}
