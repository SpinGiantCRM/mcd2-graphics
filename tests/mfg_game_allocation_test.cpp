// CPU-only actual-source check. Creates no device, graphics runtime or window.
#include "../experiments/providers/mfg_game_bridge.cpp"
#include <cstdio>
#define REQUIRE(expr) do { if(!(expr)){std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#expr);return 1;} } while(false)
int main(){
 auto& r=runtime();r.softwareReady=true;r.softwareMax=5;r.nativeReady=true;r.nativeMax=2;
 REQUIRE(mcd2_mfg_configure_v3(2,6,2,3,35)!=0); // D D requires sixths
 REQUIRE(mcd2_mfg_configure_v3(2,6,2,3,42)==0 && selectedOrder==42);
 REQUIRE(mcd2_mfg_configure_v3(2,6,2,3,106)!=0 && selectedOrder==42);
 REQUIRE(mcd2_mfg_configure_v2(2,6,2,3)==0 && selectedOrder==0);
 REQUIRE(selectedStrategy==2 && selectedMultiplier==6 && selectedDlss==2 && selectedFsr==3);
 REQUIRE(mcd2_mfg_configure_v2(2,6,3,2)!=0); // sixths require native SDK count5
 REQUIRE(selectedDlss==2 && selectedFsr==3); // failed request preserves prior selection
 REQUIRE(mcd2_mfg_configure_v2(2,5,2,3)!=0);
 REQUIRE(mcd2_mfg_configure_v2(3,6,2,3)!=0);
 REQUIRE(mcd2_mfg_configure_v2(2,6,0,5)!=0);
 REQUIRE(mcd2_mfg_configure_v2(0,1,1,0)!=0);
 REQUIRE(mcd2_mfg_configure_v2(2,6,UINT_MAX,3)!=0);
 REQUIRE(mcd2_mfg_configure_v2(1,3,2,0)==0 && selectedDlss==2 && selectedFsr==0);
 REQUIRE(mcd2_mfg_configure_v2(1,4,3,0)!=0);
 REQUIRE(mcd2_mfg_configure_v2(1,3,1,1)!=0);
 REQUIRE(mcd2_mfg_configure_v2(99,3,2,0)!=0);
 REQUIRE(mcd2_mfg_configure_v1(2,6)==0 && selectedDlss==1 && selectedFsr==4);
 REQUIRE(mcd2_mfg_configure_v1(2,5)!=0);
 REQUIRE(mcd2_mfg_configure_v1(3,6)==0 && selectedDlss==0 && selectedFsr==5);
 r.softwareReady=false;REQUIRE(mcd2_mfg_configure_v2(2,6,2,3)!=0);r.softwareReady=true;
 r.nativeReady=false;REQUIRE(mcd2_mfg_configure_v2(2,6,2,3)!=0);r.nativeReady=true;
 Session s;s.plan=*make_allocation_plan(2,3,{6,3,true});
 s.buffers.push_back(nullptr);s.output.width=1920;s.output.height=1080;
 s.guides.camera.width=1920;s.guides.camera.height=1080;s.guides.camera.farPlane=1000;
 MfgGameParameters params;
 for(unsigned slot=1;slot<=5;++slot){
  const bool native=slot==2||slot==4;parameters(s,params,native,slot);
  unsigned count{},index{};
  REQUIRE(params.Get("DLSSG.MultiFrameCount",&count)==NVSDK_NGX_Result_Success);
  REQUIRE(params.Get("DLSSG.MultiFrameIndex",&index)==NVSDK_NGX_Result_Success);
  REQUIRE(count==(native?2u:5u) && index==(native?slot/2:slot));
 }
 s.plan=*make_plan(Mode::Hybrid,6,{6,2,true});parameters(s,params,true,3);
 unsigned count{},index{};params.Get("DLSSG.MultiFrameCount",&count);params.Get("DLSSG.MultiFrameIndex",&index);
 REQUIRE(count==1 && index==1); // preserved legacy midpoint SDK contract
 REQUIRE(mcd2_mfg_configure_v2(0,1,0,0)==0 && selectedDlss==0 && selectedFsr==0);
 r.nativeMax=5;
 REQUIRE(mcd2_mfg_configure_v3(2,6,2,3,35)==0 && selectedOrder==35);
 REQUIRE(mcd2_mfg_configure_v3(0,1,0,0,35)!=0);
 REQUIRE(mcd2_mfg_configure_v3(1,3,2,0,35)==0 && selectedOrder==35);
 REQUIRE(mcd2_mfg_configure_v3(1,3,2,0,33)!=0);
 s.plan=*make_allocation_plan(2,3,{6,6,true},35);
 for(unsigned slot=1;slot<=5;++slot){
  const bool native=slot<=2;parameters(s,params,native,slot);
  params.Get("DLSSG.MultiFrameCount",&count);params.Get("DLSSG.MultiFrameIndex",&index);
  REQUIRE(count==5 && index==slot); // unchanged sixths, selected providers reevaluated
 }
 REQUIRE(mcd2_mfg_configure_v1(3,6)==0 && selectedOrder==0);
 std::puts("PASS CPU allocation configuration and SDK parameter grid");return 0;
}
