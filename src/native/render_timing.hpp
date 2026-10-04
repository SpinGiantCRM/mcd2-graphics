// Optional bounded command-list timing. Never enabled in ordinary SR use.
// Start on the first observed pipeline/barrier event AFTER native Reset:
// ReShade's reset event is BEFORE Reset, so recording there is unsafe.
struct RenderTiming {
 struct Record {uint64_t frame=0; a::command_list *cmd=nullptr; a::command_queue *queue=nullptr; bool closed=false;unsigned type=0;};
 bool active=false;uint64_t end_frame=0;std::string name;
 ID3D12QueryHeap *heap=nullptr;ID3D12Resource *buffer=nullptr;EvalOwned owned;
 std::vector<Record> records;std::map<a::command_list*,unsigned> pending;
 uint64_t unsupported=0,overflow=0,unsubmitted=0;static constexpr unsigned capacity=32768;
} timing;
struct PresentTiming {std::string name;unsigned count=0;std::vector<std::pair<uint64_t,int64_t>> samples;} present_timing;
static void timer_present_interval(){
 LARGE_INTEGER counter{};QueryPerformanceCounter(&counter);
 if(present_timing.count){
  present_timing.samples.emplace_back(frame,counter.QuadPart);
  if(present_timing.samples.size()>=present_timing.count){
   LARGE_INTEGER frequency{};QueryPerformanceFrequency(&frequency);fs::create_directories(root/present_timing.name);
   std::ofstream out(root/present_timing.name/"present-intervals.jsonl");out<<"{\"kind\":\"scope\",\"qpcFrequency\":"<<frequency.QuadPart<<",\"GPUQueries\":false,\"pixelReadbacks\":false,\"applicationPresentIntervalNotGPUKernelTime\":true}\n";
   for(auto [f,t]:present_timing.samples)out<<"{\"frame\":"<<f<<",\"counter\":"<<t<<"}\n";
   present_timing=PresentTiming{};
  }
 }
 static ULONGLONG last_poll=0;auto now=GetTickCount64();if(now-last_poll<100)return;last_poll=now;
 if(present_timing.count || !developer_file_exists(root/"present-timing.txt"))return;
 std::ifstream f(root/"present-timing.txt");std::string name;unsigned count=0;f>>name>>count;f.close();fs::remove(root/"present-timing.txt");
 if(name.empty() || name.size()>48 || name.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789-")!=std::string::npos || count<30 || count>1200)return;
 present_timing.name=name;present_timing.count=count;present_timing.samples.reserve(count);
}
static void timer_begin(a::command_list *cmd){
 if(!timing.active || internal_evaluation || timing.pending.contains(cmd))return;
 auto *native=reinterpret_cast<ID3D12GraphicsCommandList*>(cmd->get_native());auto type=native->GetType();
 if(type!=D3D12_COMMAND_LIST_TYPE_DIRECT && type!=D3D12_COMMAND_LIST_TYPE_COMPUTE){++timing.unsupported;return;}
 if(timing.records.size()>=RenderTiming::capacity){++timing.overflow;return;}
 unsigned index=unsigned(timing.records.size());timing.records.push_back({frame,cmd,nullptr,false,unsigned(type)});timing.pending[cmd]=index;
 native->EndQuery(timing.heap,D3D12_QUERY_TYPE_TIMESTAMP,index*2);
}
static void timer_reset(a::command_list *cmd){timing.pending.erase(cmd);}
static void timer_close(a::command_list *cmd){
 std::lock_guard guard(lock);auto it=timing.pending.find(cmd);if(it==timing.pending.end())return;
 auto index=it->second;auto &r=timing.records[index];if(r.closed)return;
 auto *native=reinterpret_cast<ID3D12GraphicsCommandList*>(cmd->get_native());
 native->EndQuery(timing.heap,D3D12_QUERY_TYPE_TIMESTAMP,index*2+1);
 native->ResolveQueryData(timing.heap,D3D12_QUERY_TYPE_TIMESTAMP,index*2,2,timing.buffer,index*16ull);r.closed=true;
}
static void timer_execute(a::command_queue *q,a::command_list *cmd){
 auto it=timing.pending.find(cmd);if(it==timing.pending.end())return;auto &r=timing.records[it->second];if(r.closed)r.queue=q;
}
static void timer_present(a::command_queue *q){
 static ULONGLONG last_poll=0;auto now=GetTickCount64();
 if(timing.active && frame>=timing.end_frame){
  timing.active=false;std::map<a::command_queue*,unsigned> queues;
  for(auto &r:timing.records)if(r.queue && r.closed)queues.emplace(r.queue,unsigned(queues.size()));else ++timing.unsubmitted;
  for(auto &[queue,index]:queues)queue->wait_idle();
  fs::create_directories(root/timing.name);std::ofstream out(root/timing.name/"render-timestamps.jsonl");
  LARGE_INTEGER frequency{};QueryPerformanceFrequency(&frequency);
  out<<"{\"kind\":\"coverage\",\"firstObservedPipelineOrBarrierToClose\":true,\"resetHookBeforeNativeReset\":true,\"qpcFrequency\":"<<frequency.QuadPart<<",\"unsupportedEvents\":"<<timing.unsupported<<",\"overflow\":"<<timing.overflow<<",\"unsubmittedRecords\":"<<timing.unsubmitted<<"}\n";
  for(auto &[queue,index]:queues){
   auto *native=reinterpret_cast<ID3D12CommandQueue*>(queue->get_native());UINT64 freq=0,gpu=0,cpu=0;
   auto a=native->GetTimestampFrequency(&freq),b=native->GetClockCalibration(&gpu,&cpu);
   out<<"{\"kind\":\"queue\",\"index\":"<<index<<",\"frequency\":"<<freq<<",\"gpuCalibration\":"<<gpu<<",\"cpuCalibration\":"<<cpu<<",\"frequencyHRESULT\":"<<a<<",\"calibrationHRESULT\":"<<b<<"}\n";
  }
  void *mapped=nullptr;D3D12_RANGE range{0,timing.records.size()*16};
  if(SUCCEEDED(timing.buffer->Map(0,&range,&mapped))){
   auto *ticks=static_cast<uint64_t*>(mapped);
   for(unsigned i=0;i<timing.records.size();i++){const auto &r=timing.records[i];if(r.closed && r.queue)out<<"{\"kind\":\"list\",\"frame\":"<<r.frame<<",\"queue\":"<<queues[r.queue]<<",\"type\":"<<r.type<<",\"begin\":"<<ticks[i*2]<<",\"end\":"<<ticks[i*2+1]<<"}\n";}
   D3D12_RANGE empty{0,0};timing.buffer->Unmap(0,&empty);
  }
  out.close();for(auto *r:timing.owned.resources)if(r)r->Release();timing=RenderTiming{};
 }
 if(now-last_poll<100)return;last_poll=now;
 if(timing.active || !developer_file_exists(root/"render-timing.txt"))return;
 std::ifstream request(root/"render-timing.txt");std::string name;unsigned count=0;request>>name>>count;request.close();fs::remove(root/"render-timing.txt");
 if(name.empty() || name.size()>48 || name.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789-")!=std::string::npos || count<30 || count>600)return;
 auto *device=reinterpret_cast<ID3D12Device*>(q->get_device()->get_native());
 D3D12_QUERY_HEAP_DESC hd{};hd.Type=D3D12_QUERY_HEAP_TYPE_TIMESTAMP;hd.Count=RenderTiming::capacity*2;
 if(FAILED(device->CreateQueryHeap(&hd,IID_PPV_ARGS(&timing.heap))))return;timing.owned.keep(timing.heap);
 timing.buffer=eval_buffer(device,timing.owned,RenderTiming::capacity*16ull,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
 if(!timing.buffer){for(auto *r:timing.owned.resources)if(r)r->Release();timing=RenderTiming{};return;}
 timing.records.reserve(RenderTiming::capacity);timing.name=name;timing.end_frame=frame+count;timing.active=true;
}
