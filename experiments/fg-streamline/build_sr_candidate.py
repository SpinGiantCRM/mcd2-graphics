"""Generate a temporary SR guide notification without editing released sources."""
from pathlib import Path
import argparse,hashlib,json,shutil,subprocess,sys
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--output',required=True,type=Path);p.add_argument('--reshade-headers',required=True,type=Path);p.add_argument('--ngx-headers',required=True,type=Path)
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
 if(notify){std::array<float,662> extended{};MCD2FGCamera camera{};uint32_t stamp=0,nativeReset=0;
  if(read_upload(*view,sizeof(extended),extended.data())){memcpy(&stamp,extended.data()+661,4);memcpy(&nativeReset,pc.data()+12,4);
   if(mcd2_fg::camera(extended.data(),stamp,aw,ah,c.evaluated==0||c.reset_pending||nativeReset,camera))notify(proxy,c.current_depth,c.motion,&camera);
  }
 }
'''+needle
text=text.replace(needle,replacement)
text='#include "fg_camera_math.h"\n'+text;lean.write_text(text)
observer=source/'src/native/observer.cpp';observer_text=observer.read_text()
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
(out/'build-receipt.json').write_text(json.dumps({'purpose':'temporary game FG guide notification','releasedSourcesChanged':False,'addonSHA256':hashlib.sha256(path.read_bytes()).hexdigest(),'leanBaseSHA256':hashlib.sha256((repo/'src/native/ngx_lean.hpp').read_bytes()).hexdigest(),'leanGeneratedSHA256':hashlib.sha256(text.encode()).hexdigest(),'observerBaseSHA256':hashlib.sha256((repo/'src/native/observer.cpp').read_bytes()).hexdigest(),'observerGeneratedSHA256':hashlib.sha256(observer_text.encode()).hexdigest(),'generatorSHA256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'WindowsQualified':False},indent=2)+'\n')
print('Temporary SR observer adapter built; released SR sources unchanged.')
