static std::pair<unsigned,unsigned> saved_input_size() {
 unsigned w=3840,h=2160,rw=0,rh=0;std::ifstream file(root/"saved-eval"/"input-dimensions.txt");
 if(file>>rw>>rh && ((rw==3840 && rh==2160)||(rw==2560 && rh==1440))){w=rw;h=rh;}
 return {w,h};
}
// Isolated, saved-input NGX test. Its output is read back locally and never displayed.
static std::string saved_exposure_mode() {
 std::ifstream file(root/"saved-eval"/"evaluation-mode.txt");std::string mode;file>>mode;
 return mode=="unity" || mode=="normalized" || mode=="auto"?mode:"current";
}
struct EvalOwned {
 std::vector<IUnknown*> resources;
 ~EvalOwned(){for(auto *r:resources)if(r)r->Release();}
 template<class T> T *keep(T *r){if(r)resources.push_back(r);return r;}
};
static void eval_transition(ID3D12GraphicsCommandList *cmd,ID3D12Resource *r,D3D12_RESOURCE_STATES before,D3D12_RESOURCE_STATES after) {
 D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition={r,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,before,after};cmd->ResourceBarrier(1,&b);
}
static ID3D12Resource *eval_buffer(ID3D12Device *device,EvalOwned &owned,uint64_t size,D3D12_HEAP_TYPE type,D3D12_RESOURCE_STATES state) {
 D3D12_HEAP_PROPERTIES heap{};heap.Type=type;D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;desc.Width=size;desc.Height=1;desc.DepthOrArraySize=1;desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
 ID3D12Resource *r=nullptr;auto hr=device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,state,nullptr,IID_PPV_ARGS(&r));return SUCCEEDED(hr)?owned.keep(r):nullptr;
}
static ID3D12Resource *eval_texture(ID3D12Device *device,EvalOwned &owned,uint32_t w,uint32_t h,DXGI_FORMAT fmt,bool output) {
 D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;desc.Width=w;desc.Height=h;desc.DepthOrArraySize=1;desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Format=fmt;desc.Flags=output?D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS:D3D12_RESOURCE_FLAG_NONE;
 ID3D12Resource *r=nullptr;auto hr=device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,output?D3D12_RESOURCE_STATE_UNORDERED_ACCESS:D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&r));return SUCCEEDED(hr)?owned.keep(r):nullptr;
}
static bool eval_upload(ID3D12Device *device,EvalOwned &owned,ID3D12GraphicsCommandList *cmd,ID3D12Resource *dest,const fs::path &path,const float *scalar=nullptr) {
 auto desc=dest->GetDesc();D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};UINT rows=0;UINT64 rowbytes=0,total=0;device->GetCopyableFootprints(&desc,0,1,0,&footprint,&rows,&rowbytes,&total);
 auto *upload=eval_buffer(device,owned,total,D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ);if(!upload)return false;
 void *data=nullptr;D3D12_RANGE no_read{0,0};if(FAILED(upload->Map(0,&no_read,&data)))return false;std::memset(data,0,total);bool okay=true;
 if(scalar)std::memcpy(data,scalar,sizeof(float));
 else {std::ifstream input(path,std::ios::binary|std::ios::ate);if(!input || input.tellg()!=static_cast<std::streamoff>(rowbytes*rows))okay=false;else {input.seekg(0);for(UINT y=0;y<rows;y++){input.read(static_cast<char*>(data)+footprint.Offset+y*footprint.Footprint.RowPitch,rowbytes);if(!input){okay=false;break;}}}}
 upload->Unmap(0,nullptr);if(!okay)return false;
 D3D12_TEXTURE_COPY_LOCATION src{},dst{};src.pResource=upload;src.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;src.PlacedFootprint=footprint;dst.pResource=dest;dst.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;cmd->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
 eval_transition(cmd,dest,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);return true;
}
#include "ngx_dense_motion.hpp"
static void evaluate_saved(HMODULE module,ID3D12Device *device,a::command_queue *queue,NVSDK_NGX_Parameter *params,NVSDK_NGX_Handle *feature,std::ofstream &log,bool gpu_motion=false) {
 using Eval=NVSDK_NGX_Result(*)(ID3D12GraphicsCommandList*,const NVSDK_NGX_Handle*,const NVSDK_NGX_Parameter*,void*);
 auto evaluate=reinterpret_cast<Eval>(GetProcAddress(module,"NVSDK_NGX_D3D12_EvaluateFeature"));
 // Direct-driver C++ callback entry, matching OptiScaler; callback is null.
 if(!evaluate){log<<"{\"stage\":\"evaluate_export\",\"status\":\"missing\"}\n";return;}
 auto table=*reinterpret_cast<void***>(params);
 auto ui=reinterpret_cast<void(*)(NVSDK_NGX_Parameter*,const char*,unsigned int)>(table[4]);
 auto integer=reinterpret_cast<void(*)(NVSDK_NGX_Parameter*,const char*,int)>(table[3]);
 auto f=reinterpret_cast<void(*)(NVSDK_NGX_Parameter*,const char*,float)>(table[6]);
 auto resource=reinterpret_cast<void(*)(NVSDK_NGX_Parameter*,const char*,ID3D12Resource*)>(table[1]);
 auto [input_width,input_height]=saved_input_size();
 auto *q=reinterpret_cast<ID3D12CommandQueue*>(queue->get_native());EvalOwned common;
 auto *command_device=device;ID3D12Device *resource_device=nullptr;
 if(SUCCEEDED(device->QueryInterface(observer_unwrapped,reinterpret_cast<void**>(&resource_device)))){common.keep(resource_device);device=resource_device;}
 auto *output=eval_texture(device,common,3840,2160,DXGI_FORMAT_R16G16B16A16_FLOAT,true);if(!output)return;
 auto outdesc=output->GetDesc();D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};UINT rows=0;UINT64 rowbytes=0,total=0;device->GetCopyableFootprints(&outdesc,0,1,0,&footprint,&rows,&rowbytes,&total);
 auto *readback=eval_buffer(device,common,total,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);if(!readback)return;
 unsigned frame_count=2;{std::ifstream n(root/"saved-eval"/"frame-count.txt");unsigned requested=0;if(n>>requested && requested>=2 && requested<=64)frame_count=requested;}
 for(unsigned index=0;index<frame_count;index++) {
  EvalOwned perframe;ID3D12CommandAllocator *allocator=nullptr;ID3D12GraphicsCommandList *cmd=nullptr;
  auto hr=command_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator));perframe.keep(allocator);
  if(SUCCEEDED(hr))hr=command_device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator,nullptr,IID_PPV_ARGS(&cmd));perframe.keep(cmd);if(FAILED(hr))return;
  auto base=root/"saved-eval";auto prefix="frame"+std::to_string(index);float constants[5]{};std::ifstream constants_file(base/(prefix+"-constants.bin"),std::ios::binary);constants_file.read(reinterpret_cast<char*>(constants),sizeof(constants));if(!constants_file)return;
  auto exposure_mode=saved_exposure_mode();bool normalized=exposure_mode=="normalized";
  auto *colour=eval_texture(device,perframe,input_width,input_height,normalized?DXGI_FORMAT_R16G16B16A16_FLOAT:DXGI_FORMAT_R11G11B10_FLOAT,false);
  auto *depth=eval_texture(device,perframe,input_width,input_height,DXGI_FORMAT_R32_FLOAT,false);
  auto *motion=eval_texture(device,perframe,input_width,input_height,DXGI_FORMAT_R16G16_FLOAT,gpu_motion);
  auto *exposure=eval_texture(device,perframe,1,1,DXGI_FORMAT_R32_FLOAT,gpu_motion);
  if(!colour || !depth || !motion || !exposure)return;
  if(!eval_upload(device,perframe,cmd,colour,base/(prefix+(normalized?"-normalized-colour.bin":"-colour.bin"))) || !eval_upload(device,perframe,cmd,depth,base/(prefix+"-depth.bin")))return;
  DenseReadback motion_read{},exposure_read{};
  if(gpu_motion) {
   if(!dense_gpu(device,command_device,perframe,cmd,depth,motion,exposure,base,prefix,log)){log<<"{\"stage\":\"gpu_motion_setup\",\"status\":\"failed\"}\n";return;}
   motion_read=dense_readback(device,perframe,cmd,motion,base/(prefix+"-gpu-motion.bin"));exposure_read=dense_readback(device,perframe,cmd,exposure,base/(prefix+"-gpu-exposure.bin"));
  } else if(!eval_upload(device,perframe,cmd,motion,base/(prefix+"-motion.bin")) || !eval_upload(device,perframe,cmd,exposure,{},&constants[4]))return;
  // Establish that native uploads/copies work and whether Evaluate overwrites output.
  eval_transition(cmd,output,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_DEST);
  if(!eval_upload(device,perframe,cmd,output,base/"sentinel.bin"))return;
  eval_transition(cmd,output,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  auto cdesc=colour->GetDesc();D3D12_PLACED_SUBRESOURCE_FOOTPRINT cf{};UINT crows=0;UINT64 crowbytes=0,ctotal=0;
  device->GetCopyableFootprints(&cdesc,0,1,0,&cf,&crows,&crowbytes,&ctotal);
  auto *colour_check=eval_buffer(device,perframe,ctotal,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);if(!colour_check)return;
  eval_transition(cmd,colour,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_SOURCE);
  D3D12_TEXTURE_COPY_LOCATION csrc{},cdst{};csrc.pResource=colour;csrc.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;cdst.pResource=colour_check;cdst.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;cdst.PlacedFootprint=cf;
  cmd->CopyTextureRegion(&cdst,0,0,0,&csrc,nullptr);eval_transition(cmd,colour,D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
  ID3D12Resource *supplied_exposure=exposure;
  if(exposure_mode=="unity") {
   supplied_exposure=eval_texture(device,perframe,1,1,DXGI_FORMAT_R32_FLOAT,false);float one=1.f;
   if(!supplied_exposure || !eval_upload(device,perframe,cmd,supplied_exposure,{},&one))return;
  } else if(exposure_mode=="auto")supplied_exposure=nullptr;
  resource(params,NVSDK_NGX_Parameter_Color,colour);resource(params,NVSDK_NGX_Parameter_Depth,depth);resource(params,NVSDK_NGX_Parameter_MotionVectors,motion);resource(params,NVSDK_NGX_Parameter_Output,output);resource(params,NVSDK_NGX_Parameter_ExposureTexture,supplied_exposure);
  f(params,NVSDK_NGX_Parameter_Jitter_Offset_X,constants[0]);f(params,NVSDK_NGX_Parameter_Jitter_Offset_Y,constants[1]);f(params,NVSDK_NGX_Parameter_MV_Scale_X,1.f);f(params,NVSDK_NGX_Parameter_MV_Scale_Y,1.f);
  f(params,NVSDK_NGX_Parameter_DLSS_Pre_Exposure,(normalized || exposure_mode=="unity")?1.f:constants[2]);f(params,NVSDK_NGX_Parameter_DLSS_Exposure_Scale,1.f);f(params,NVSDK_NGX_Parameter_FrameTimeDeltaInMsec,constants[3]);f(params,NVSDK_NGX_Parameter_Sharpness,0.f);
  integer(params,NVSDK_NGX_Parameter_Reset,index==0);ui(params,NVSDK_NGX_Parameter_DLSS_Render_Subrect_Dimensions_Width,input_width);ui(params,NVSDK_NGX_Parameter_DLSS_Render_Subrect_Dimensions_Height,input_height);
  auto getresource=reinterpret_cast<NVSDK_NGX_Result(*)(NVSDK_NGX_Parameter*,const char*,ID3D12Resource**)>(table[9]);
  auto getui=reinterpret_cast<NVSDK_NGX_Result(*)(NVSDK_NGX_Parameter*,const char*,unsigned int*)>(table[12]);
  ID3D12Resource *checked_colour=nullptr,*checked_output=nullptr;unsigned int checked_width=0,checked_height=0;
  auto cr=getresource(params,NVSDK_NGX_Parameter_Color,&checked_colour);auto orr=getresource(params,NVSDK_NGX_Parameter_Output,&checked_output);
  getui(params,NVSDK_NGX_Parameter_Width,&checked_width);getui(params,NVSDK_NGX_Parameter_Height,&checked_height);
  log<<"{\"stage\":\"evaluate_parameter_check\",\"width\":"<<checked_width<<",\"height\":"<<checked_height<<",\"colourMatch\":"<<(checked_colour==colour?"true":"false")<<",\"outputMatch\":"<<(checked_output==output?"true":"false")<<",\"colourGetterResult\":"<<static_cast<uint32_t>(cr)<<",\"outputGetterResult\":"<<static_cast<uint32_t>(orr)<<",\"entry\":\"direct_driver_cpp_callback\"}\n";
  log<<"{\"stage\":\"saved_evaluate_begin\",\"frame\":"<<index<<",\"reset\":"<<(index==0?"true":"false")<<"}\n";
  ID3D12GraphicsCommandList *evaluation_list=cmd,*unwrapped_evaluation=nullptr;
  std::string command_route;{std::ifstream route(base/"command-route.txt");route>>command_route;}
  if(command_route=="native-evaluate"){
   if(FAILED(cmd->QueryInterface(observer_unwrapped,reinterpret_cast<void**>(&unwrapped_evaluation)))){log<<"{\"stage\":\"evaluate_route\",\"status\":\"unwrap_failed\"}\n";return;}
   evaluation_list=unwrapped_evaluation;
  }
  log<<"{\"stage\":\"evaluate_command_route\",\"frame\":"<<index<<",\"nativeEvaluate\":"<<(unwrapped_evaluation?"true":"false")<<",\"deviceAndFeatureRemainWrapped\":true}\n";
  auto result=evaluate(evaluation_list,feature,params,nullptr);if(unwrapped_evaluation)unwrapped_evaluation->Release();log<<"{\"stage\":\"saved_evaluate\",\"frame\":"<<index<<",\"result\":"<<static_cast<uint32_t>(result)<<"}\n";
  if(NVSDK_NGX_SUCCEED(result)) {
   D3D12_RESOURCE_BARRIER uav{};uav.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;uav.UAV.pResource=output;cmd->ResourceBarrier(1,&uav);
   eval_transition(cmd,output,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);
   D3D12_TEXTURE_COPY_LOCATION src{},dst{};src.pResource=output;src.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;dst.pResource=readback;dst.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;dst.PlacedFootprint=footprint;cmd->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
   eval_transition(cmd,output,D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  }
  hr=cmd->Close();if(FAILED(hr))return;submit_probe(queue,cmd);
  void *check_data=nullptr;D3D12_RANGE check_range{0,static_cast<SIZE_T>(ctotal)};
  if(SUCCEEDED(colour_check->Map(0,&check_range,&check_data))) {
   std::ofstream copied(base/(prefix+"-gpu-colour.bin"),std::ios::binary);
   for(UINT y=0;y<crows;y++)copied.write(static_cast<const char*>(check_data)+cf.Offset+y*cf.Footprint.RowPitch,crowbytes);
   copied.close();D3D12_RANGE empty{0,0};colour_check->Unmap(0,&empty);
  }
  if(gpu_motion){dense_save(motion_read);dense_save(exposure_read);}
  if(NVSDK_NGX_FAILED(result))return;
  void *data=nullptr;D3D12_RANGE range{0,static_cast<SIZE_T>(total)};if(FAILED(readback->Map(0,&range,&data)))return;
  std::ofstream out(base/(prefix+"-dlaa-output.bin"),std::ios::binary);for(UINT y=0;y<rows;y++)out.write(static_cast<const char*>(data)+footprint.Offset+y*footprint.Footprint.RowPitch,rowbytes);out.close();D3D12_RANGE empty{0,0};readback->Unmap(0,&empty);
  log<<"{\"stage\":\"saved_output\",\"frame\":"<<index<<",\"bytes\":"<<rows*rowbytes<<",\"gpuCompleted\":true}\n";
 }
}
