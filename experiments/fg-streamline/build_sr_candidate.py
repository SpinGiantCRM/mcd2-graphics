"""Generate a temporary SR guide notification without editing released sources."""
from pathlib import Path
import argparse,hashlib,json,shutil,subprocess,sys
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--output',required=True,type=Path);p.add_argument('--reshade-headers',required=True,type=Path);p.add_argument('--ngx-headers',required=True,type=Path)
p.add_argument('--guide-inspection',action='store_true',help='Private developer capture build; never package this artifact')
p.add_argument('--clang-cxx',default='clang++');p.add_argument('--mingw-cxx',default='x86_64-w64-mingw32-g++')
a=p.parse_args();repo=Path(__file__).resolve().parents[2];out=a.output.resolve();source=out/'source';source.mkdir(parents=True,exist_ok=True)
shutil.copytree(repo/'src/native',source/'src/native',dirs_exist_ok=True)
for name in ('build.py','build_toolchain.py'):shutil.copyfile(repo/name,source/name)
for name in ('fg_camera_contract.h','fg_camera_math.h'):shutil.copyfile(Path(__file__).parent/name,source/'src/native'/name)
lean=source/'src/native/ngx_lean.hpp';text=lean.read_text();needle=' restore_root(cmd,saved,false,true);restore_root(cmd,saved,true,true);cmd->bind_pipeline(a::pipeline_stage::all_compute,{saved.pipeline});commands[cmd]=saved;\n auto resource=mcd2_ngx_set_resource;'
assert text.count(needle)==1
replacement=''' // Temporary observer receives current guides before NGX. It never owns SR state.
 auto fgObserver=GetModuleHandleW(L"mcd2-fg-guide-recon.addon64");
 auto notify=fgObserver?reinterpret_cast<void(*)(void*,void*,void*,const MCD2FGCamera*)>(GetProcAddress(fgObserver,"mcd2_fg_observe_sr")):nullptr;
 auto enabled=fgObserver?reinterpret_cast<int(*)()>(GetProcAddress(fgObserver,"mcd2_fg_inputs_wanted")):nullptr;
 if(notify && enabled && enabled()){std::array<float,662> extended{};MCD2FGCamera camera{};uint32_t stamp=0,nativeReset=0;
  if(read_upload(*view,sizeof(extended),extended.data())){memcpy(&stamp,extended.data()+661,4);memcpy(&nativeReset,pc.data()+12,4);
   if(mcd2_fg::camera(extended.data(),stamp,aw,ah,c.evaluated==0||c.reset_pending||nativeReset,camera))notify(proxy,c.current_depth,c.motion,&camera);
  }
 }
'''+needle
text=text.replace(needle,replacement)
# Existing developer-triggered capture now records the actual FG/SR guides too.
# No additional copy, readback or validation work runs without lean-capture.txt.
capture_needle=' if(lean.capture_requested){\n'
assert text.count(capture_needle)==1
capture_extra=r''' if(lean.capture_requested){
  for(auto pair:{std::pair<ID3D12Resource*,const char*>{c.current_depth,"fg-depth"},{c.motion,"fg-motion"}}){
   auto path=root/label/(std::to_string(frame)+"-"+pair.second+".bin");
   auto read=dense_readback(c.resource_device,c.owned,native,pair.first,path);
   if(read.buffer){read.buffer->AddRef();copies.push_back({read.buffer,cmd,nullptr,read.total,read.rows,read.fp.Footprint.RowPitch,static_cast<uint32_t>(read.rowbytes),path});}
  }
  std::ofstream(root/label/(std::to_string(frame)+"-fg-guide-size.json"))<<"{\"width\":"<<aw<<",\"height\":"<<ah<<",\"motionFormat\":34,\"depthFormat\":41}";
'''
if a.guide_inspection:
 text=text.replace(capture_needle,capture_extra)
 build=source/'build.py';build_text=build.read_text();assert build_text.count('-DMCD2_ENABLE_DIAGNOSTICS=0')==1
 build.write_text(build_text.replace('-DMCD2_ENABLE_DIAGNOSTICS=0','-DMCD2_ENABLE_DIAGNOSTICS=1'))
if a.guide_inspection:
 start=text.index(' if(!copies.empty()){',text.index('static void lean_present'))
 end=text.index(' if(live_fixture && !seen_taa)',start)
 text=text[:start]+r''' if(!copies.empty()){
  // FG owns additional queues/pacing. Do not wait_idle from its present callback.
  // Signal after observed submission and map only on a later completed fence.
  for(auto &copy:copies)if(copy.queue){
   if(!copy.inspectionFence){auto queue=reinterpret_cast<ID3D12CommandQueue*>(copy.queue->get_native());ID3D12Device*device=nullptr;
    auto hr=queue->GetDevice(IID_PPV_ARGS(&device));if(SUCCEEDED(hr))hr=device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&copy.inspectionFence));if(device)device->Release();
    if(SUCCEEDED(hr))hr=queue->Signal(copy.inspectionFence,1);
    if(FAILED(hr)){if(copy.inspectionFence)copy.inspectionFence->Release();copy.inspectionFence=nullptr;continue;}
   }
   const auto complete=copy.inspectionFence->GetCompletedValue();if(complete==UINT64_MAX||complete<1)continue;
   void*mapped=nullptr;D3D12_RANGE range{0,size_t(copy.bytes)};
   if(SUCCEEDED(copy.readback->Map(0,&range,&mapped))){std::ofstream out(copy.path,std::ios::binary);out.write(static_cast<char*>(mapped),copy.bytes);D3D12_RANGE empty{0,0};copy.readback->Unmap(0,&empty);}
   copy.readback->Release();copy.readback=nullptr;copy.inspectionFence->Release();copy.inspectionFence=nullptr;
  }
  copies.erase(std::remove_if(copies.begin(),copies.end(),[](const Copy&c){return c.readback==nullptr;}),copies.end());
 }
'''+text[end:]
text='#include "fg_camera_math.h"\n'+text;lean.write_text(text)
observer=source/'src/native/observer.cpp';observer_text=observer.read_text()
if a.guide_inspection:
 marker=' fs::path path;\n};\nstatic std::vector<Copy> copies;'
 assert observer_text.count(marker)==1
 observer_text=observer_text.replace(marker,' fs::path path;\n ID3D12Fence*inspectionFence=nullptr;\n};\nstatic std::vector<Copy> copies;')
 marker='for(auto &c:copies)if(c.readback)c.readback->Release();copies.clear();'
 assert observer_text.count(marker)==1
 observer_text=observer_text.replace(marker,'for(auto &c:copies){if(c.readback)c.readback->Release();if(c.inspectionFence)c.inspectionFence->Release();}copies.clear();')

needle=' const bool success=lean_cleanup(q,guard,true);'
assert observer_text.count(needle)==1
observer_text=observer_text.replace(needle,needle+'''
 // Shared NGX teardown: SR must release its feature before Streamline stops.
 // Never call the other module while holding the SR observer mutex.
 if(success){guard.unlock();auto observer=GetModuleHandleW(L"mcd2-fg-guide-recon.addon64");
  auto retired=observer?reinterpret_cast<void(*)()>(GetProcAddress(observer,"mcd2_fg_sr_retired")):nullptr;
  if(retired)retired();guard.lock();}
''')
observer.write_text(observer_text)
with (out/'build-private.log').open('w') as log:
 subprocess.run([sys.executable,str(source/'build.py'),'--native-only','--clang-cxx',a.clang_cxx,'--mingw-cxx',a.mingw_cxx,'--reshade-include',str(a.reshade_headers.resolve()),'--ngx-include',str(a.ngx_headers.resolve()),'--output',str(out/'native')],stdout=log,stderr=subprocess.STDOUT,check=True)
path=out/'native/mcd2-graphics.addon64';shutil.copyfile(path,out/path.name)
(out/'build-receipt.json').write_text(json.dumps({'purpose':'temporary game FG guide notification','releasedSourcesChanged':False,'privateGuideInspection':a.guide_inspection,'addonSHA256':hashlib.sha256(path.read_bytes()).hexdigest(),'leanBaseSHA256':hashlib.sha256((repo/'src/native/ngx_lean.hpp').read_bytes()).hexdigest(),'leanGeneratedSHA256':hashlib.sha256(text.encode()).hexdigest(),'observerBaseSHA256':hashlib.sha256((repo/'src/native/observer.cpp').read_bytes()).hexdigest(),'observerGeneratedSHA256':hashlib.sha256(observer_text.encode()).hexdigest(),'generatorSHA256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'WindowsQualified':False},indent=2)+'\n')
print('Temporary SR observer adapter built; released SR sources unchanged.')
