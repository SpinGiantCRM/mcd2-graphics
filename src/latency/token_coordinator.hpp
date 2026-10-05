#pragma once
#include <array>
#include <cstdint>
#include <mutex>
#include <condition_variable>
#include <limits>
namespace mcd2::streamline_reflex {
enum class Mode : unsigned { Off, On, OnBoost };
enum class Marker : unsigned { SimulationStart, SimulationEnd, RenderStart, RenderEnd, PresentStart, PresentEnd };
struct Provider {
 virtual ~Provider()=default;
 virtual bool mode(Mode)=0;
 virtual void* begin(std::uint32_t)=0;
 virtual std::uint32_t index(void*)=0;
 virtual bool sleep(void*)=0;
 virtual bool marker(void*,Marker)=0;
 // Release must occur after any outstanding Sleep call returns.
 virtual void release()=0;
};
// Tokens belong to Streamline. SDK 2.14.1 reuses a six-token ring; an older
// unfinished frame must not be overwritten by the seventh allocation.
struct MarkerFailure {std::uint64_t frame=0,last=0;unsigned marker=0,sent=0,required=0;bool found=false,ready=false,complete=false,identity=false;};
class TokenCoordinator {
 struct Frame {std::uint64_t engine=0;void*token=nullptr;unsigned sent=0;bool complete=false,ready=false;};
 static constexpr unsigned bit(Marker marker){return 1u<<unsigned(marker);}
 // Simulation and rendering overlap on separate engine threads. Preserve
 // their actual marker times, while enforcing each interval and Present order.
 static constexpr unsigned prerequisites(Marker marker){
  switch(marker){
   case Marker::SimulationStart:return 0;
   case Marker::SimulationEnd:return bit(Marker::SimulationStart);
   case Marker::RenderStart:return bit(Marker::SimulationStart);
   case Marker::RenderEnd:return bit(Marker::RenderStart);
   case Marker::PresentStart:return bit(Marker::RenderEnd);
   case Marker::PresentEnd:return bit(Marker::PresentStart);
  }
  return ~0u;
 }
 std::array<Frame,6> frames{};Provider&provider;std::mutex mutex;std::condition_variable idle;
 bool active_=false,sleeping=false,releasePending=false;std::uint64_t last=0,serial=0,epoch=0,completed_=0;
 unsigned error_=0;MarkerFailure markerFailure_{};
 Mode requested=Mode::Off,applied=Mode::Off;
 Frame* find(std::uint64_t id){for(auto&f:frames)if(f.token&&f.engine==id)return &f;return nullptr;}
 void stop(){if(active_){if(sleeping)releasePending=true;else provider.release();}active_=false;++epoch;}
 bool fail(unsigned error){error_=error;stop();return false;}
 bool valid(Frame&f){return f.token&&provider.index(f.token)==std::uint32_t(f.engine);}
 public:
 explicit TokenCoordinator(Provider&p):provider(p){}
 ~TokenCoordinator(){shutdown();} // Owner must join the game thread before destruction.
 bool attach(){std::lock_guard lock(mutex);if(active_||sleeping||releasePending)return false;frames={};last=serial=completed_=0;requested=applied=Mode::Off;if(!provider.mode(Mode::Off))return false;active_=true;return true;}
 void request_mode(Mode m){std::lock_guard lock(mutex);if(unsigned(m)<=2)requested=m;}
 // This is the pre-input pacing boundary, separate from SimulationStart.
 bool pre_simulation(std::uint64_t id){
  std::unique_lock lock(mutex);if(!active_)return false;
  if(sleeping||!id||id<=last||id>std::numeric_limits<std::uint32_t>::max())return fail(1);
  if(last){auto*p=find(last);if(!p||!(p->sent&bit(Marker::SimulationEnd)))return fail(2);}
  auto&f=frames[serial%frames.size()];if(f.token&&!f.complete)return fail(3);
  if(requested!=applied){if(!provider.mode(requested))return fail(4);applied=requested;}
  f={id,provider.begin(std::uint32_t(id)),0,false,false};if(!valid(f))return fail(5);++serial;last=id;
  const auto generation=epoch;auto*token=f.token;sleeping=true;
  // Rendering older frames must remain possible while driver pacing waits.
  lock.unlock();const bool slept=provider.sleep(token);lock.lock();sleeping=false;idle.notify_all();
  if(releasePending){provider.release();releasePending=false;}
  if(!active_||generation!=epoch)return false;
  if(!slept||!valid(f))return fail(6);
  f.ready=true;return true;
 }
 bool mark(std::uint64_t id,Marker marker){
  std::lock_guard lock(mutex);if(!active_)return false;
  if(unsigned(marker)>unsigned(Marker::PresentEnd))return fail(7);
  auto*f=find(id);auto required=prerequisites(marker);
  if(!f||!f->ready||!valid(*f)||f->complete||(f->sent&bit(marker))||(f->sent&required)!=required){markerFailure_={id,last,unsigned(marker),f?f->sent:0,required,f!=nullptr,f&&f->ready,f&&f->complete,f&&valid(*f)};return fail(8);}
  if(!provider.marker(f->token,marker))return fail(9);
  // A paused/menu frame can finish rendering while the game thread is still
  // completing simulation. Do not retime its SimulationEnd or recycle its
  // token at PresentEnd: all six actual markers must arrive first.
  f->sent|=bit(marker);if(f->sent==63u){f->complete=true;++completed_;}return true;
 }
 // Future FG constants/tagging must use this identity, never a Present counter.
 // The callback cannot retain the token or recursively call this coordinator.
 template<class F>bool with_token(std::uint64_t id,F&&use){std::lock_guard lock(mutex);if(!active_)return false;auto*f=find(id);if(!f||!f->ready||f->complete||!valid(*f))return false;return use(f->token,std::uint32_t(f->engine));}
 // Engine PresentEnd runs even when native presentation fails. Do not fabricate a successful marker.
 bool verify_present_end(std::uint64_t id){std::lock_guard lock(mutex);if(!active_)return false;auto*f=find(id);if(!f)return true;if((f->sent&bit(Marker::PresentStart)) && !(f->sent&bit(Marker::PresentEnd)))return fail(10);return true;}
 MarkerFailure marker_failure(){std::lock_guard lock(mutex);return markerFailure_;}
 unsigned error(){std::lock_guard lock(mutex);return error_;}
 Mode mode(){std::lock_guard lock(mutex);return applied;}
 void shutdown(){std::lock_guard lock(mutex);stop();}
 // Device/SDK teardown must wait for an in-flight driver Sleep to return.
 void drain(){std::unique_lock lock(mutex);idle.wait(lock,[this]{return !sleeping&&!releasePending;});}
 bool active(){std::lock_guard lock(mutex);return active_;}
 std::uint64_t completed(){std::lock_guard lock(mutex);return completed_;}
};
}
