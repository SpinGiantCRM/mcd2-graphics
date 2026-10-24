#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>
using UINT=unsigned;
[[maybe_unused]] constexpr unsigned D3D12_RESOURCE_DIMENSION_TEXTURE2D=3;
constexpr unsigned D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE=1;
constexpr unsigned D3D12_RESOURCE_STATE_COPY_SOURCE=2;
constexpr unsigned D3D12_RESOURCE_STATE_UNORDERED_ACCESS=3;
constexpr unsigned D3D12_RESOURCE_STATE_COPY_DEST=4;
[[maybe_unused]] constexpr unsigned D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX=0;
[[maybe_unused]] constexpr unsigned DXGI_FORMAT_R16G16B16A16_FLOAT=10;
struct D3D12_RESOURCE_DESC {
 unsigned Dimension=3; std::uint64_t Width=1680; unsigned Height=1050;
 unsigned Format=10,DepthOrArraySize=1,MipLevels=1;
 struct {unsigned Count=1,Quality=0;} SampleDesc;
};
struct ID3D12Resource {D3D12_RESOURCE_DESC desc;auto GetDesc(){return desc;}};
struct D3D12_BOX {unsigned left,top,front,right,bottom,back;};
struct D3D12_TEXTURE_COPY_LOCATION {ID3D12Resource* pResource=nullptr;unsigned Type=0,SubresourceIndex=0;};
struct ID3D12GraphicsCommandList {
 unsigned whole=0,region=0;D3D12_BOX box{};
 ID3D12Resource *copiedFrom=nullptr,*copiedTo=nullptr;
 void CopyResource(ID3D12Resource* to,ID3D12Resource* from){++whole;copiedTo=to;copiedFrom=from;}
 void CopyTextureRegion(const D3D12_TEXTURE_COPY_LOCATION* to,unsigned x,unsigned y,unsigned z,
  const D3D12_TEXTURE_COPY_LOCATION* from,const D3D12_BOX* bounds){
  if(x||y||z||to->Type||from->Type||to->SubresourceIndex||from->SubresourceIndex||!bounds)throw std::runtime_error("wrong copy subresource/origin");
  ++region;box=*bounds;copiedTo=to->pResource;copiedFrom=from->pResource;
 }
};
struct Generation {ID3D12Resource* output=nullptr;unsigned outputWidth=1680,outputHeight=1050;bool history_dirty=false;};
struct Transition {ID3D12Resource* resource;unsigned from,to;};
static std::vector<Transition> transitions;
static void eval_transition(ID3D12GraphicsCommandList*,ID3D12Resource* r,unsigned from,unsigned to){transitions.push_back({r,from,to});}
// ACTUAL_COPY_OUTPUT
static void require(bool condition,const char* text){if(!condition)throw std::runtime_error(text);}
int main(){try{
 ID3D12Resource source{},target{};Generation c{&source};ID3D12GraphicsCommandList cmd;
 auto reset=[&]{source={};target={};c={&source};cmd={};transitions.clear();};
 auto accepted=[&](bool padded){
  require(copy_output(c,&cmd,&target),"copy rejected");
  require(cmd.whole==unsigned(!padded)&&cmd.region==unsigned(padded),"wrong copy mode");
  require(cmd.copiedFrom==&source&&cmd.copiedTo==&target&&c.history_dirty,"wrong resource/history");
  require(transitions.size()==4&&transitions[0].resource==&source&&transitions[0].from==1&&transitions[0].to==2&&
   transitions[1].resource==&target&&transitions[1].from==3&&transitions[1].to==4&&
   transitions[2].resource==&target&&transitions[2].from==4&&transitions[2].to==3&&
   transitions[3].resource==&source&&transitions[3].from==2&&transitions[3].to==1,"transition contract changed");
  if(padded)require(cmd.box.left==0&&cmd.box.top==0&&cmd.box.front==0&&cmd.box.right==1680&&cmd.box.bottom==1050&&cmd.box.back==1,"unbounded logical copy");
 };
 auto rejected=[&]{require(!copy_output(c,&cmd,&target),"unsafe copy accepted");require(!cmd.whole&&!cmd.region&&transitions.empty()&&!c.history_dirty,"rejected copy recorded work");};
 reset();accepted(false);
 reset();target.desc.Height=1052;accepted(true);
 reset();target.desc.Width=1696;accepted(true);
 reset();target.desc.Width=1696;target.desc.Height=1052;accepted(true);
 reset();source.desc.DepthOrArraySize=target.desc.DepthOrArraySize=2;accepted(false); // Existing exact-copy contract.
 reset();target.desc.Height=1049;rejected();
 reset();target.desc.Width=1679;rejected();
 reset();target.desc.Format=11;rejected();
 reset();target.desc.DepthOrArraySize=2;rejected();
 reset();target.desc.MipLevels=2;rejected();
 reset();target.desc.SampleDesc.Count=2;rejected();
 reset();target.desc.SampleDesc.Quality=1;rejected();
 reset();target.desc.Dimension=2;rejected();
 reset();target.desc.Height=1052;source.desc.DepthOrArraySize=target.desc.DepthOrArraySize=2;rejected();
 reset();target.desc.Height=1052;source.desc.MipLevels=target.desc.MipLevels=2;rejected();
 reset();target.desc.Height=1052;source.desc.SampleDesc.Count=target.desc.SampleDesc.Count=2;rejected();
 reset();target.desc.Height=1052;source.desc.SampleDesc.Quality=target.desc.SampleDesc.Quality=1;rejected();
 reset();target.desc.Height=1052;source.desc.Dimension=target.desc.Dimension=2;rejected();
 reset();target.desc.Height=1052;source.desc.Format=target.desc.Format=11;rejected();
 reset();target.desc.Height=1052;c.outputWidth=1679;rejected();
 reset();target.desc.Height=1052;c.outputHeight=1049;rejected();
 reset();target.desc.Height=1052;c.outputWidth=0;rejected();
 reset();require(!copy_output(c,&cmd,&source)&&transitions.empty(),"alias accepted");
 std::cout<<"Actual output copy: exact fast path, padded width/height, bounded box, transitions and descriptor rejections pass\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
