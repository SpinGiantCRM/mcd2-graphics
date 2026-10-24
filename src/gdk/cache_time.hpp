#pragma once
#include <cstdint>
namespace mcd2::gdk {
inline constexpr std::uint64_t fileTimeEpoch=116444736000000000ULL,ticksPerSecond=10000000;
inline bool cacheFileTime(std::int64_t seconds,std::uint64_t now,std::uint64_t& out){
 if(seconds<=0||now<fileTimeEpoch||std::uint64_t(seconds)>(now-fileTimeEpoch)/ticksPerSecond)return false;
 out=fileTimeEpoch+std::uint64_t(seconds)*ticksPerSecond;return true;
}
inline constexpr std::uint64_t freshBudgetTicks(std::uint32_t resolutionMilliseconds){
 // WGS container timestamps have one-second precision; native saves do not.
 return resolutionMilliseconds==1000?25000000:15000000;
}
}
