#include "ngx_parameter_bridge.hpp"
// Bounded integration gate: saved owned inputs, the real game command-list
// proxy, owned NGX output, and paired original-AA outputs. No displayed NGX.
struct LiveFixture {
 HMODULE module=nullptr;bool initialized=false,ready=false;
 EvalOwned owned;ID3D12Device *proxy_device=nullptr,*resource_device=nullptr;
 ID3D12Resource *colour=nullptr,*depth=nullptr,*motion=nullptr,*exposure=nullptr,*output=nullptr,*sentinel=nullptr;
 ID3D12Resource *current_packed=nullptr,*current_depth=nullptr;
 ID3D12RootSignature *current_signature=nullptr;
 ID3D12PipelineState *current_pipeline=nullptr;
 bool current_resources_initialized=false;bool reset_pending=false;
 ID3D12Resource *validation=nullptr,*validation_zero=nullptr;
 ID3D12RootSignature *validation_signature=nullptr;ID3D12PipelineState *validation_pipeline=nullptr;
 ID3D12QueryHeap *timestamps=nullptr;uint64_t timestamp_frequency=0;unsigned evaluated=0;bool validation_initialized=false;
 a::command_queue *integration_queue=nullptr;
 NVSDK_NGX_Parameter *params=nullptr;NVSDK_NGX_Handle *feature=nullptr;
 float constants[5]{};unsigned width=0,height=0,outwidth=3840,outheight=2160;
 std::ofstream log;
};
static std::unique_ptr<LiveFixture> live_fixture;
static bool live_cleanup_busy=false;
template<class F> static F live_export(const char *name){return reinterpret_cast<F>(GetProcAddress(live_fixture->module,name));}
// Creation uses a dedicated allocator/list retained by the generation. Nothing
// reuses that allocator, so CPU-side GPU idle is unnecessary: creation is
// submitted before evaluations on the same integration queue. Teardown still
// requires a fresh completed queue fence before releasing any owned object.
static bool live_submit_creation(a::command_queue *queue,ID3D12GraphicsCommandList *cmd,LiveFixture &c) {
 if(!queue || queue!=c.integration_queue)return false;
 ID3D12CommandList *unwrapped=nullptr;
 if(FAILED(cmd->QueryInterface(observer_unwrapped,reinterpret_cast<void**>(&unwrapped))) || !unwrapped)return false;
 auto *native_queue=reinterpret_cast<ID3D12CommandQueue*>(queue->get_native());
 native_queue->ExecuteCommandLists(1,&unwrapped);unwrapped->Release();
 c.log<<"{\"stage\":\"creation_submission\",\"sameIntegrationQueue\":true,\"allocatorReused\":false,\"CPUWait\":false,\"retainedUntilGenerationCompletion\":true}\n";c.log.flush();return true;
}
// A fresh queue-side signal covers all previous submissions. A timeout/device
// failure is not completion evidence and must preserve the generation.
static bool live_wait_generation(a::command_queue *q,LiveFixture &c) {
 if(!q || q!=c.integration_queue || FAILED(c.resource_device->GetDeviceRemovedReason()))return false;
 q->flush_immediate_command_list();
 ID3D12Fence *fence=nullptr;if(FAILED(c.resource_device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence))))return false;
 auto *queue=reinterpret_cast<ID3D12CommandQueue*>(q->get_native());auto hr=queue->Signal(fence,1);
 bool done=false;
 // Polling avoids leaving SetEventOnCompletion holding an event after timeout.
 const ULONGLONG start=GetTickCount64();
 while(SUCCEEDED(hr)){
  const uint64_t completed=fence->GetCompletedValue();
  if(completed==UINT64_MAX)break;
  if(completed>=1){done=true;break;}
  if(GetTickCount64()-start>=2000)break;
  Sleep(1);
 }
 c.log<<"{\"stage\":\"cleanup_gpu_completion\",\"signalHRESULT\":"<<unsigned(hr)<<",\"completed\":"<<(done?"true":"false")<<",\"outsideObserverLock\":true}\n";c.log.flush();fence->Release();return done;
}
// Called only after GPU completion, with the observer lock released. Keep both
// device references alive through feature release and any unshared Shutdown.
static bool cleanup_detached_live(LiveFixture &c) {
 auto result=[&](const char *stage,NVSDK_NGX_Result value){c.log<<"{\"stage\":\""<<stage<<"\",\"result\":"<<static_cast<uint32_t>(value)<<"}\n";c.log.flush();return NVSDK_NGX_SUCCEED(value);};
 if(c.feature){using F=NVSDK_NGX_Result(*)(NVSDK_NGX_Handle*);auto fn=reinterpret_cast<F>(GetProcAddress(c.module,"NVSDK_NGX_D3D12_ReleaseFeature"));if(!fn || !result("release_feature",fn(c.feature)))return false;c.feature=nullptr;}
 if(c.params){using F=NVSDK_NGX_Result(*)(NVSDK_NGX_Parameter*);auto fn=reinterpret_cast<F>(GetProcAddress(c.module,"NVSDK_NGX_D3D12_DestroyParameters"));if(!fn || !result("destroy_parameters",fn(c.params)))return false;c.params=nullptr;}
 if(c.initialized){
  auto bridge=GetModuleHandleW(L"fg-sdk-bridge.dll");
  using Initialized=int(*)();using Owner=int(*)(void*);
  auto active=bridge?reinterpret_cast<Initialized>(GetProcAddress(bridge,"mcd2_fg_initialized")):nullptr;
  auto owner=bridge?reinterpret_cast<Owner>(GetProcAddress(bridge,"mcd2_fg_ngx_owner_v1")):nullptr;
  const bool shared=active&&active()!=0;
  const int ownership=shared&&owner?owner(c.proxy_device):shared?-1:0;
  if(ownership<0){c.log<<"{\"stage\":\"shared_ngx_owner_unverified\",\"generationRetained\":true}\n";c.log.flush();return false;}
  if(ownership==1){c.log<<"{\"stage\":\"shutdown_deferred_to_streamline\",\"deviceMatched\":true,\"finalOwner\":\"Streamline\"}\n";c.log.flush();}
  else {using F=NVSDK_NGX_Result(*)(ID3D12Device*);auto fn=reinterpret_cast<F>(GetProcAddress(c.module,"NVSDK_NGX_D3D12_Shutdown1"));if(!fn||!result("shutdown_device",fn(c.resource_device)))return false;}
  c.initialized=false;
 }
 // Device references were inserted first; reverse release drops them last.
 for(auto it=c.owned.resources.rbegin();it!=c.owned.resources.rend();++it)if(*it)(*it)->Release();c.owned.resources.clear();
 c.log<<"{\"stage\":\"owned_release\",\"afterShutdown\":true,\"outsideObserverLock\":true}\n";c.log.flush();
 if(c.module)FreeLibrary(c.module);c.module=nullptr;return true;
}
static void cleanup_live_saved(a::command_queue *queue) {
 if(!live_fixture)return;if(queue)queue->wait_idle();auto &c=*live_fixture;
 if(c.feature){using F=NVSDK_NGX_Result(*)(NVSDK_NGX_Handle*);auto fn=live_export<F>("NVSDK_NGX_D3D12_ReleaseFeature");if(fn)c.log<<"{\"stage\":\"release_feature\",\"result\":"<<static_cast<uint32_t>(fn(c.feature))<<"}\n";c.feature=nullptr;}
 if(c.params){using F=NVSDK_NGX_Result(*)(NVSDK_NGX_Parameter*);auto fn=live_export<F>("NVSDK_NGX_D3D12_DestroyParameters");if(fn)c.log<<"{\"stage\":\"destroy_parameters\",\"result\":"<<static_cast<uint32_t>(fn(c.params))<<"}\n";c.params=nullptr;}
 for(auto *r:c.owned.resources)if(r)r->Release();c.owned.resources.clear();
 if(c.initialized){using F=NVSDK_NGX_Result(*)();auto fn=live_export<F>("NVSDK_NGX_D3D12_Shutdown");if(fn)c.log<<"{\"stage\":\"shutdown\",\"result\":"<<static_cast<uint32_t>(fn())<<"}\n";}
 if(c.module)FreeLibrary(c.module);live_fixture.reset();
}
static bool init_live_saved(a::command_queue *queue,std::unique_lock<std::recursive_mutex> *guard) {
 if(live_fixture)return false;live_fixture=std::make_unique<LiveFixture>();auto &c=*live_fixture;c.integration_queue=queue;
 c.log.open(root/label/"live-api.jsonl",std::ios::app);
 auto fail=[&](const char *reason){
  c.log<<"{\"stage\":\"init_failure\",\"reason\":\""<<reason<<"\"}\n";c.log.flush();
  if(!guard){cleanup_live_saved(queue);return false;}
  live_cleanup_busy=true;auto context=std::move(live_fixture);guard->unlock();
  const bool completed=!context->initialized || live_wait_generation(queue,*context);
  const bool cleaned=completed && cleanup_detached_live(*context);
  guard->lock();live_cleanup_busy=false;if(!cleaned){context->ready=false;live_fixture=std::move(context);}
  return false;
 };
 c.module=LoadLibraryW(L"nvngx.dll");if(!c.module)c.module=LoadLibraryExW(L"Z:\\run\\host\\usr\\lib\\nvidia\\wine\\nvngx.dll",nullptr,LOAD_WITH_ALTERED_SEARCH_PATH);
 if(!c.module)return fail("driver_module");
 using Init=NVSDK_NGX_Result(*)(const char*,NVSDK_NGX_EngineType,const char*,const wchar_t*,ID3D12Device*,NVSDK_NGX_Version,const NVSDK_NGX_FeatureCommonInfo*);
 using Allocate=NVSDK_NGX_Result(*)(NVSDK_NGX_Parameter**);
 using Destroy=NVSDK_NGX_Result(*)(NVSDK_NGX_Parameter*);
 using Create=NVSDK_NGX_Result(*)(ID3D12GraphicsCommandList*,NVSDK_NGX_Feature,NVSDK_NGX_Parameter*,NVSDK_NGX_Handle**);
 auto init=live_export<Init>("NVSDK_NGX_D3D12_Init_ProjectID");auto allocate=live_export<Allocate>("NVSDK_NGX_D3D12_AllocateParameters");auto caps=live_export<Allocate>("NVSDK_NGX_D3D12_GetCapabilityParameters");auto destroy=live_export<Destroy>("NVSDK_NGX_D3D12_DestroyParameters");auto create=live_export<Create>("NVSDK_NGX_D3D12_CreateFeature");
 if(!init || !allocate || !caps || !destroy || !create || !GetProcAddress(c.module,"NVSDK_NGX_D3D12_EvaluateFeature"))return fail("driver_exports");
 auto *native=reinterpret_cast<ID3D12Device*>(queue->get_device()->get_native());UINT bytes=sizeof(c.proxy_device);
 if(FAILED(native->GetPrivateData(observer_device_proxy,&bytes,&c.proxy_device)) || !c.proxy_device || bytes!=sizeof(c.proxy_device))return fail("device_proxy");
 c.proxy_device->AddRef();c.owned.keep(c.proxy_device);
 if(FAILED(c.proxy_device->QueryInterface(observer_unwrapped,reinterpret_cast<void**>(&c.resource_device))))return fail("resource_device");c.owned.keep(c.resource_device);
 const auto runtime_path=(asset_root/L"ngx-runtime").wstring(),cache_path=(root/L"ngx-cache").wstring();
 std::error_code cache_error;fs::create_directories(fs::path(cache_path),cache_error);if(cache_error)return fail("cache_directory");
 const wchar_t *paths[]={runtime_path.c_str(),L"Z:\\run\\host\\usr\\lib\\nvidia\\wine"};NVSDK_NGX_FeatureCommonInfo info{};info.PathListInfo.Path=paths;info.PathListInfo.Length=2;info.LoggingInfo.LoggingCallback=ngx_log_callback;info.LoggingInfo.MinimumLoggingLevel=NVSDK_NGX_LOGGING_LEVEL_OFF;info.LoggingInfo.DisableOtherLoggingSinks=true;
 auto result=init("27fe0b1b-1112-4466-b36e-dd330014e2ad",NVSDK_NGX_ENGINE_TYPE_CUSTOM,"mcd2-graphics-0.1",cache_path.c_str(),c.proxy_device,NVSDK_NGX_Version_API,&info);
 c.log<<"{\"stage\":\"init\",\"result\":"<<static_cast<uint32_t>(result)<<",\"wrappedDevice\":true}\n";
 if(NVSDK_NGX_FAILED(result))return fail("NGX_init");c.initialized=true;
 NVSDK_NGX_Parameter *cap_params=nullptr;result=caps(&cap_params);c.log<<"{\"stage\":\"capabilities\",\"result\":"<<static_cast<uint32_t>(result)<<"}\n";if(cap_params){
 auto getptr=mcd2_ngx_get_pointer;auto getui=mcd2_ngx_get_ui;
 auto setui=mcd2_ngx_set_ui;auto seti=mcd2_ngx_set_i;
 void *callback=nullptr;auto got=getptr(cap_params,NVSDK_NGX_Parameter_DLSSOptimalSettingsCallback,&callback);
 if(NVSDK_NGX_SUCCEED(got) && callback){
  using Optimal=NVSDK_NGX_Result(*)(NVSDK_NGX_Parameter*);
  for(int mode:{int(NVSDK_NGX_PerfQuality_Value_MaxQuality),int(NVSDK_NGX_PerfQuality_Value_Balanced),int(NVSDK_NGX_PerfQuality_Value_MaxPerf),int(NVSDK_NGX_PerfQuality_Value_UltraPerformance),int(NVSDK_NGX_PerfQuality_Value_DLAA)}){
   setui(cap_params,NVSDK_NGX_Parameter_Width,lean_outwidth);setui(cap_params,NVSDK_NGX_Parameter_Height,lean_outheight);seti(cap_params,NVSDK_NGX_Parameter_PerfQualityValue,mode);seti(cap_params,NVSDK_NGX_Parameter_RTXValue,0);
   auto optimal=reinterpret_cast<Optimal>(callback)(cap_params);unsigned ow=0,oh=0,minw=0,minh=0,maxw=0,maxh=0;
   if(NVSDK_NGX_SUCCEED(optimal)){getui(cap_params,NVSDK_NGX_Parameter_OutWidth,&ow);getui(cap_params,NVSDK_NGX_Parameter_OutHeight,&oh);minw=maxw=ow;minh=maxh=oh;getui(cap_params,NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Min_Render_Width,&minw);getui(cap_params,NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Min_Render_Height,&minh);getui(cap_params,NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Max_Render_Width,&maxw);getui(cap_params,NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Max_Render_Height,&maxh);}
   c.log<<"{\"stage\":\"optimal_settings\",\"mode\":"<<mode<<",\"result\":"<<unsigned(optimal)<<",\"optimal\":["<<ow<<","<<oh<<"],\"minimum\":["<<minw<<","<<minh<<"],\"maximum\":["<<maxw<<","<<maxh<<"]}\n";
  }
 }else c.log<<"{\"stage\":\"optimal_settings_callback_unavailable\"}\n";
 destroy(cap_params);
 }if(NVSDK_NGX_FAILED(result))return fail("capabilities");
 result=allocate(&c.params);if(NVSDK_NGX_FAILED(result) || !c.params)return fail("parameters");
 auto ui=mcd2_ngx_set_ui;auto integer=mcd2_ngx_set_i;
 c.width=lean_width;c.height=lean_height;c.outwidth=lean_outwidth;c.outheight=lean_outheight;
 ui(c.params,NVSDK_NGX_Parameter_CreationNodeMask,1);ui(c.params,NVSDK_NGX_Parameter_VisibilityNodeMask,1);ui(c.params,NVSDK_NGX_Parameter_Width,c.width);ui(c.params,NVSDK_NGX_Parameter_Height,c.height);ui(c.params,NVSDK_NGX_Parameter_OutWidth,c.outwidth);ui(c.params,NVSDK_NGX_Parameter_OutHeight,c.outheight);
 integer(c.params,NVSDK_NGX_Parameter_PerfQualityValue,c.width==c.outwidth?NVSDK_NGX_PerfQuality_Value_DLAA:(c.width*100<=c.outwidth*40?NVSDK_NGX_PerfQuality_Value_UltraPerformance:(c.width*100<=c.outwidth*51?NVSDK_NGX_PerfQuality_Value_MaxPerf:(c.width*100<=c.outwidth*60?NVSDK_NGX_PerfQuality_Value_Balanced:NVSDK_NGX_PerfQuality_Value_MaxQuality))));
 // Optional restart-only model hint; absent/invalid input preserves runtime defaults.
 wchar_t wideModel[64]{};char modelText[64]{};const auto overrideFile=(asset_root/L"DependencyOverrides.ini").wstring();
 const auto modelLength=GetPrivateProfileStringW(L"DLSS",L"ModelPreset",L"0",wideModel,64,overrideFile.c_str());
 bool validModelText=modelLength<63;for(unsigned i=0;i<modelLength;++i){if(wideModel[i]>127)validModelText=false;modelText[i]=char(wideModel[i]&127);}
 if(!validModelText)modelText[0]=0;
 const unsigned model=mcd2::dlss::model_hint(modelText);
 if(model){for(const char*key:{NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_DLAA,NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Quality,NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Balanced,NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Performance,NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_UltraPerformance,NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_UltraQuality})ui(c.params,key,model);}
 c.log<<"{\"stage\":\"model_preset_hint\",\"value\":"<<model<<"}\n";
 integer(c.params,NVSDK_NGX_Parameter_DLSS_Feature_Create_Flags,NVSDK_NGX_DLSS_Feature_Flags_IsHDR|NVSDK_NGX_DLSS_Feature_Flags_MVLowRes|NVSDK_NGX_DLSS_Feature_Flags_DepthInverted);integer(c.params,NVSDK_NGX_Parameter_DLSS_Enable_Output_Subrects,0);
 ID3D12CommandAllocator *allocator=nullptr;ID3D12GraphicsCommandList *cmd=nullptr;
 auto hr=c.proxy_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator));c.owned.keep(allocator);
 if(SUCCEEDED(hr))hr=c.proxy_device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator,nullptr,IID_PPV_ARGS(&cmd));c.owned.keep(cmd);if(FAILED(hr))return fail("owned_command_list");
 result=create(cmd,NVSDK_NGX_Feature_SuperSampling,c.params,&c.feature);c.log<<"{\"stage\":\"create_super_sampling\",\"result\":"<<static_cast<uint32_t>(result)<<",\"inputWidth\":"<<c.width<<",\"inputHeight\":"<<c.height<<",\"outputWidth\":"<<c.outwidth<<",\"outputHeight\":"<<c.outheight<<"}\n";
 if(FAILED(cmd->Close()))return fail("feature_close");if(!live_submit_creation(queue,cmd,c))return fail("feature_submission");if(NVSDK_NGX_FAILED(result) || !c.feature)return fail("feature_creation");
 c.motion=eval_texture(c.resource_device,c.owned,c.width,c.height,DXGI_FORMAT_R16G16_FLOAT,true);
 c.exposure=eval_texture(c.resource_device,c.owned,1,1,DXGI_FORMAT_R32_FLOAT,true);
 // Direct native output is set at each evaluation; no extra output texture.
 if(!c.motion || !c.exposure)return fail("lean_textures");
 // No fixture, sentinel, texture upload or initial converter dispatch.
 auto resource=mcd2_ngx_set_resource;auto f=mcd2_ngx_set_f;
 resource(c.params,NVSDK_NGX_Parameter_Color,c.colour);resource(c.params,NVSDK_NGX_Parameter_Depth,c.depth);resource(c.params,NVSDK_NGX_Parameter_MotionVectors,c.motion);resource(c.params,NVSDK_NGX_Parameter_Output,c.output);resource(c.params,NVSDK_NGX_Parameter_ExposureTexture,c.exposure);
 f(c.params,NVSDK_NGX_Parameter_Jitter_Offset_X,0.f);f(c.params,NVSDK_NGX_Parameter_Jitter_Offset_Y,0.f);f(c.params,NVSDK_NGX_Parameter_MV_Scale_X,1.f);f(c.params,NVSDK_NGX_Parameter_MV_Scale_Y,1.f);f(c.params,NVSDK_NGX_Parameter_DLSS_Pre_Exposure,1.f);f(c.params,NVSDK_NGX_Parameter_DLSS_Exposure_Scale,1.f);f(c.params,NVSDK_NGX_Parameter_FrameTimeDeltaInMsec,16.6667f);f(c.params,NVSDK_NGX_Parameter_Sharpness,0.f);
 integer(c.params,NVSDK_NGX_Parameter_Reset,1);ui(c.params,NVSDK_NGX_Parameter_DLSS_Render_Subrect_Dimensions_Width,c.width);ui(c.params,NVSDK_NGX_Parameter_DLSS_Render_Subrect_Dimensions_Height,c.height);
 if(developer_file_exists(root/"lean-inject-init-failure.txt")){fs::remove(root/"lean-inject-init-failure.txt");return fail("injected_after_feature_creation");}
 c.ready=true;c.log<<"{\"stage\":\"lean_ready\",\"fixtureInputs\":false}\n";return true;
}
static bool live_saved_gate(a::command_list *cmd,uint32_t shader,uint32_t x,uint32_t y,uint32_t z) {
 if(!live_fixture || !live_fixture->ready)return false;auto &c=*live_fixture;
 Cmd saved=commands[cmd];auto slots=resolve(cmd,true);auto out=std::find_if(slots.begin(),slots.end(),[](const Slot &s){return s.space==0 && s.reg==0 && s.binding.type==a::descriptor_type::unordered_access_view;});
 if(out==slots.end() || !saved.pipeline || !saved.compute_layout || saved.dynamic_offsets || !root_push_supported(saved.cp) || !root_push_supported(saved.gp)){records.push_back("{\"kind\":\"error\",\"message\":\"live_restore_contract_incomplete\"}");return false;}
 auto source_resource=from_view(cmd->get_device(),out->binding.view);auto *source=reinterpret_cast<ID3D12Resource*>(source_resource.handle);auto desc=source->GetDesc();
 if(desc.Format!=DXGI_FORMAT_R16G16B16A16_FLOAT || desc.Width!=3840 || desc.Height!=2160)return false;
 for(const auto &s:slots)if(s.space==0 && s.reg<6 && (s.binding.type==a::descriptor_type::shader_resource_view || s.binding.type==a::descriptor_type::buffer_shader_resource_view))if(from_view(cmd->get_device(),s.binding.view).handle==source_resource.handle)return false;
 auto *proxy=validated_game_proxy(cmd);if(!proxy){records.push_back("{\"kind\":\"error\",\"message\":\"game_proxy_identity\"}");return false;}
 ID3D12Device *device=nullptr;bool device_matches=SUCCEEDED(proxy->GetDevice(IID_PPV_ARGS(&device))) && device==c.proxy_device;if(device)device->Release();
 if(!device_matches || proxy->GetType()!=D3D12_COMMAND_LIST_TYPE_DIRECT){proxy->Release();records.push_back("{\"kind\":\"error\",\"message\":\"live_proxy_device_or_list_type\"}");return false;}
 auto *native=reinterpret_cast<ID3D12GraphicsCommandList*>(cmd->get_native());native->Dispatch(x,y,z);saved.states[source_resource.handle]=a::resource_usage::unordered_access;commands[cmd]=saved;
 D3D12_RESOURCE_BARRIER order{};order.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;order.UAV.pResource=source;native->ResourceBarrier(1,&order);snapshot(cmd,*out,shader,true,"before-live");
 eval_transition(native,c.output,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_DEST);native->CopyResource(c.output,c.sentinel);eval_transition(native,c.output,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
 using Evaluate=NVSDK_NGX_Result(*)(ID3D12GraphicsCommandList*,const NVSDK_NGX_Handle*,const NVSDK_NGX_Parameter*,void*);auto evaluate=live_export<Evaluate>("NVSDK_NGX_D3D12_EvaluateFeature");
 c.log<<"{\"stage\":\"game_command_evaluate_begin\",\"frame\":"<<frame<<",\"reset\":true,\"fixtureIndex\":0}\n";
 internal_evaluation=true;auto result=evaluate(proxy,c.feature,c.params,nullptr);internal_evaluation=false;proxy->Release();
 c.log<<"{\"stage\":\"game_command_evaluate\",\"frame\":"<<frame<<",\"result\":"<<static_cast<uint32_t>(result)<<"}\n";
 // Restore both roots after actual NGX proxy calls changed ReShade's cache.
 restore_root(cmd,saved,false,true);restore_root(cmd,saved,true,true);cmd->bind_pipeline(a::pipeline_stage::all_compute,{saved.pipeline});commands[cmd]=saved;
 native->Dispatch(x,y,z);native->ResourceBarrier(1,&order);snapshot(cmd,*out,shader,true,"after-live");
 order.UAV.pResource=c.output;native->ResourceBarrier(1,&order);eval_transition(native,c.output,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
 const auto path=root/label/(std::to_string(frame)+"-ngx-fixture-output.bin");auto read=dense_readback(c.resource_device,c.owned,native,c.output,path);eval_transition(native,c.output,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
 if(read.buffer){read.buffer->AddRef();copies.push_back({read.buffer,cmd,nullptr,read.total,read.rows,read.fp.Footprint.RowPitch,static_cast<uint32_t>(read.rowbytes),path});}
 records.push_back("{\"kind\":\"live_fixture\",\"frame\":"+std::to_string(frame)+",\"nativeDispatches\":2,\"wrappedGameCommandList\":true,\"result\":"+std::to_string(static_cast<uint32_t>(result))+",\"liveGameInputs\":false,\"displaySubstitution\":false,\"fixtureFile\":\""+path.filename().string()+"\"}");
 return true;
}
