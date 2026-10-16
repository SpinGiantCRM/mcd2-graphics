#include "../src/gdk/cache_time.hpp"
#include <cassert>
#include <limits>
int main(){using namespace mcd2::gdk;std::uint64_t out=7;const auto now=fileTimeEpoch+100*ticksPerSecond+5000000;
 assert(cacheFileTime(100,now,out)&&out==fileTimeEpoch+100*ticksPerSecond);
 assert(!cacheFileTime(101,now,out));assert(!cacheFileTime(0,now,out));assert(!cacheFileTime(-1,now,out));
 assert(!cacheFileTime(std::numeric_limits<std::int64_t>::max(),now,out));assert(!cacheFileTime(1,fileTimeEpoch-1,out));
 assert(cacheFileTime(1,std::numeric_limits<std::uint64_t>::max(),out));
 assert(freshBudgetTicks(0)==15000000);assert(freshBudgetTicks(1000)==25000000);assert(freshBudgetTicks(5000)==15000000);
 assert(now-(fileTimeEpoch+99*ticksPerSecond)<freshBudgetTicks(1000));
 assert(now-(fileTimeEpoch+98*ticksPerSecond)>=freshBudgetTicks(1000));
}
