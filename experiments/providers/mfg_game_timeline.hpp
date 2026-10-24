#pragma once
#include <algorithm>
#include <cstdint>
// Nanosecond clock values. This policy never sleeps or calls DXGI, so cadence,
// gap admission, finite deadlines and backpressure can be checked deterministically.
namespace mcd2::mfg {
struct PhaseSchedule {
 std::uint64_t start{},step{},end{};unsigned count{},sync{};bool limited{};
 explicit operator bool()const{return count>=2&&step&&end>=start;}
 std::uint64_t deadline(unsigned phase)const{return phase<count?start+step*phase:0;}
};
class RealCadence {
 std::uint64_t previous_{},period_{};unsigned samples_{};
public:
 void reset(){previous_=period_=0;samples_=0;}
 bool observe(std::uint64_t arrival,std::uint64_t priorOwnedTime=0){
  if(!arrival||!previous_||arrival<=previous_){previous_=arrival;period_=0;samples_=0;return false;}
  auto delta=arrival-previous_;previous_=arrival;if(priorOwnedTime>=delta){period_=0;samples_=0;return false;}delta-=priorOwnedTime;
  if(delta<2000000||delta>250000000){period_=0;samples_=0;return false;}
  if(period_&&(delta>period_*2||delta*2<period_)){period_=delta;samples_=1;return false;}
  period_=period_?(period_*3+delta)/4:delta;++samples_;return samples_>=2;
 }
 std::uint64_t period()const{return samples_>=2?period_:0;}
 PhaseSchedule schedule(std::uint64_t now,std::uint64_t previousPresentation,unsigned multiplier,std::uint64_t refresh,unsigned sync,bool tearing)const{
  auto period=this->period();if(!now||!period||!refresh||multiplier<2||multiplier>6||sync>4||(tearing&&sync))return {};
  period=std::max(period,refresh*sync);
  // Explicit real-frame limiter: a requested multiplier cannot exceed the
  // actual output refresh. One-batch backpressure applies this cadence.
  bool limited=period/multiplier<refresh;period=std::max(period,refresh*multiplier);
  if(period>250000000)return {};
  auto step=period/multiplier;
  auto start=std::max(now,previousPresentation?previousPresentation+step:now);
  // A missed complete interval is a real-frame fallback, never a catch-up burst.
  if(start-now>period||now>previous_+period)return {};
  return {start,step,start+step*(multiplier-1),multiplier,sync?1u:0u,limited};
 }
};
}
