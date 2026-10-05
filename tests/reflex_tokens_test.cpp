#include "../src/latency/token_coordinator.hpp"
#include <cassert>
#include <condition_variable>
#include <thread>
#include <vector>
#include <iostream>
#include <fstream>
using namespace mcd2::streamline_reflex;
struct Fake:Provider {
 struct Token {std::uint32_t index=0;};std::array<Token,6>tokens{};unsigned serial=0,releases=0,sleeps=0;
 bool wait=false,waiting=false,wake=false,failMarker=false;
 std::mutex mutex;std::condition_variable cv;std::vector<Mode>modes;std::vector<std::pair<unsigned,unsigned>>marks;
 bool mode(Mode m)override{modes.push_back(m);return true;}
 void*begin(std::uint32_t id)override{auto&t=tokens[serial++%6];t.index=id;return &t;}
 std::uint32_t index(void*t)override{return static_cast<Token*>(t)->index;}
 bool sleep(void*)override{++sleeps;std::unique_lock lock(mutex);if(wait){waiting=true;cv.notify_all();cv.wait(lock,[&]{return wake;});}return true;}
 bool marker(void*t,Marker m)override{marks.emplace_back(index(t),unsigned(m));return !failMarker;}
 void release()override{++releases;}
};
void simulation(TokenCoordinator&c,unsigned id){assert(c.pre_simulation(id));assert(c.mark(id,Marker::SimulationStart));assert(c.mark(id,Marker::SimulationEnd));}
void render(TokenCoordinator&c,unsigned id){for(unsigned i=2;i<6;++i)assert(c.mark(id,Marker(i)));}
int main(int argc,char**argv){
 {Fake p;TokenCoordinator c(p);assert(c.attach());for(unsigned n=1;n<=4;++n){c.request_mode(Mode(n==4?0:n-1));simulation(c,n);bool used=false;assert(c.with_token(n,[&](void*t,unsigned i){used=true;return t&&i==n&&p.index(t)==n;}));assert(used);render(c,n);}assert(p.sleeps==4&&c.completed()==4);assert(p.modes==std::vector<Mode>({Mode::Off,Mode::On,Mode::OnBoost,Mode::Off}));}
 {Fake p;TokenCoordinator c(p);assert(c.attach());simulation(c,10);simulation(c,11);render(c,10);render(c,11);assert(c.completed()==2);}
 {Fake p;TokenCoordinator c(p);assert(c.attach());simulation(c,1);assert(!c.mark(1,Marker::PresentStart));assert(!c.active()&&p.releases==1);}
 {Fake p;TokenCoordinator c(p);assert(c.attach());simulation(c,1);assert(!c.pre_simulation(1));}
 {Fake p;TokenCoordinator c(p);assert(c.attach());for(unsigned n=1;n<=6;++n)simulation(c,n);assert(!c.pre_simulation(7));assert(p.serial==6);}
 {Fake p;TokenCoordinator c(p);assert(c.attach());for(unsigned n=1;n<=18;++n){simulation(c,n);render(c,n);}assert(c.completed()==18);}
 {Fake p;TokenCoordinator c(p);assert(c.attach());assert(!c.pre_simulation(std::uint64_t(UINT32_MAX)+1));assert(p.serial==0);}
 {Fake p;TokenCoordinator c(p);assert(c.attach());simulation(c,1);p.tokens[0].index=99;assert(!c.mark(1,Marker::RenderStart));}
 {Fake p;TokenCoordinator c(p);assert(c.attach());simulation(c,1);p.wait=true;std::thread game([&]{assert(c.pre_simulation(2));});{std::unique_lock lock(p.mutex);assert(p.cv.wait_for(lock,std::chrono::seconds(1),[&]{return p.waiting;}));}render(c,1);{std::lock_guard lock(p.mutex);p.wake=true;p.cv.notify_all();}game.join();assert(c.mark(2,Marker::SimulationStart));assert(c.mark(2,Marker::SimulationEnd));render(c,2);}
 {Fake p;TokenCoordinator c(p);assert(c.attach());p.wait=true;std::thread game([&]{assert(!c.pre_simulation(1));});{std::unique_lock lock(p.mutex);assert(p.cv.wait_for(lock,std::chrono::seconds(1),[&]{return p.waiting;}));}c.shutdown();assert(p.releases==0);{std::lock_guard lock(p.mutex);p.wake=true;p.cv.notify_all();}game.join();assert(p.releases==1);}
 {Fake p;TokenCoordinator c(p);assert(c.attach());simulation(c,1);p.failMarker=true;assert(!c.mark(1,Marker::RenderStart));assert(!c.active());}
 // Captured startup: render begins on the render thread before simulation
 // finishes; frame 2 simulation can finish before frame 1 presentation.
 {Fake p;TokenCoordinator c(p);assert(c.attach());assert(c.pre_simulation(1));assert(c.mark(1,Marker::SimulationStart));std::thread renderer([&]{assert(c.mark(1,Marker::RenderStart));});renderer.join();assert(c.mark(1,Marker::SimulationEnd));simulation(c,2);for(unsigned i=3;i<6;++i)assert(c.mark(1,Marker(i)));render(c,2);assert(c.completed()==2);assert(p.marks[1]==std::make_pair(1u,2u)&&p.marks[2]==std::make_pair(1u,1u));}
 // No interval may end before its start; no rendering before simulation start.
 for(unsigned m=1;m<6;++m){Fake p;TokenCoordinator c(p);assert(c.attach());assert(c.pre_simulation(1));assert(!c.mark(1,Marker(m)));assert(p.marks.empty()&&!c.active());}
 // Every duplicate is rejected, including a repeated final marker.
 for(unsigned duplicate=0;duplicate<6;++duplicate){Fake p;TokenCoordinator c(p);assert(c.attach());assert(c.pre_simulation(1));for(unsigned m=0;m<=duplicate;++m)assert(c.mark(1,Marker(m)));assert(!c.mark(1,Marker(duplicate)));assert(!c.active());}
 // Captured menu transition: the render thread presents before SimulationEnd.
 // Preserve actual timestamps; completion/reuse waits for both intervals.
 {Fake p;TokenCoordinator c(p);assert(c.attach());assert(c.pre_simulation(1));assert(c.mark(1,Marker::SimulationStart));render(c,1);assert(c.completed()==0);assert(c.verify_present_end(1));assert(c.mark(1,Marker::SimulationEnd));assert(c.completed()==1);assert(p.marks.back()==std::make_pair(1u,1u));simulation(c,2);render(c,2);assert(c.completed()==2);}
 {Fake p;TokenCoordinator c(p);assert(c.attach());assert(c.pre_simulation(1));assert(c.mark(1,Marker::SimulationStart));render(c,1);assert(!c.pre_simulation(2));assert(p.serial==1);}
 {Fake p;TokenCoordinator c(p);assert(c.attach());assert(c.pre_simulation(1));assert(c.mark(1,Marker::SimulationStart));assert(c.mark(1,Marker::RenderStart));assert(!c.pre_simulation(2));assert(p.serial==1);}
 {Fake p;TokenCoordinator c(p);assert(c.attach());assert(c.pre_simulation(1));assert(!c.mark(1,Marker(32)));assert(p.marks.empty());}
 {Fake p;TokenCoordinator c(p);assert(c.attach());p.wait=true;std::thread game([&]{assert(c.pre_simulation(1));});{std::unique_lock lock(p.mutex);assert(p.cv.wait_for(lock,std::chrono::seconds(1),[&]{return p.waiting;}));}bool used=false;assert(!c.with_token(1,[&](void*,unsigned){used=true;return true;}));assert(!used);{std::lock_guard lock(p.mutex);p.wake=true;p.cv.notify_all();}game.join();assert(c.with_token(1,[](void*,unsigned){return true;}));}
 {Fake p;TokenCoordinator c(p);assert(c.attach());simulation(c,UINT32_MAX);render(c,UINT32_MAX);assert(c.completed()==1);assert(!c.pre_simulation(std::uint64_t(UINT32_MAX)+1));}
 {Fake p;TokenCoordinator c(p);assert(c.attach());simulation(c,1);assert(c.mark(1,Marker::RenderStart));assert(c.mark(1,Marker::RenderEnd));assert(c.mark(1,Marker::PresentStart));assert(!c.verify_present_end(1));assert(!c.active());}
 {Fake p;TokenCoordinator c(p);assert(c.attach());simulation(c,1);render(c,1);assert(c.verify_present_end(1));assert(c.active());}
 std::cout<<"31 token coordinator checks passed\n";
 if(argc==2){Fake p;TokenCoordinator c(p);assert(c.attach());std::ifstream input(argv[1]);assert(input);unsigned phase,id;unsigned frames=0;while(input>>phase>>id){bool ok=true;switch(phase){case 8:ok=c.pre_simulation(id);break;case 0:case 1:case 2:ok=c.mark(id,Marker(phase));break;case 9:ok=c.mark(id,Marker::RenderEnd)&&c.mark(id,Marker::PresentStart);break;case 10:ok=c.mark(id,Marker::PresentEnd);++frames;break;default:break;}if(!ok){std::cerr<<"Replay rejected phase "<<phase<<" id "<<id<<"\n";return 2;}}assert(c.completed()==frames&&p.sleeps==frames);std::cout<<frames<<" captured game frames replayed with matching tokens and ordered PCL markers\n";}
}
