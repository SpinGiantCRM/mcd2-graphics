"""Generate an opt-in real-game FSR capture probe; no output replacement or vendor downloads."""
from pathlib import Path
import argparse, hashlib, json, shutil, subprocess, sys

def patch(text, before, after):
    if text.count(before) != 1:
        raise ValueError('Source contract changed: ' + before[:80])
    return text.replace(before, after)

def generate(repo, source):
    shutil.copytree(repo/'src/native', source/'src/native', dirs_exist_ok=True)
    shutil.copytree(repo/'src/providers', source/'src/providers', dirs_exist_ok=True)
    (source/'experiments/providers').mkdir(parents=True,exist_ok=True)
    shutil.copyfile(Path(__file__).with_name('fsr_game_bridge.h'),source/'experiments/providers/fsr_game_bridge.h')
    for name in ['build.py','build_toolchain.py']:
        shutil.copyfile(repo/name,source/name)
    for name in ['fsr_game_bridge.h','fsr_probe_driver.hpp']:
        shutil.copyfile(Path(__file__).with_name(name),source/'src/native'/name)
    probe=source/'src/native/sr_guide_probe.hpp'
    text=probe.read_text()
    text=patch(text,'struct GuideProbeSample {','#include "fsr_probe_driver.hpp"\nstruct GuideProbeSample {')
    text=patch(text,'std::array<DenseReadback,3> readbacks{};','std::array<DenseReadback,5> readbacks{};')
    text=patch(text,'struct GuideProbe : SrGuideGeneration {','struct GuideProbe : SrGuideGeneration {\n FsrProbeState fsr;')
    text=patch(text,'if(!guide_probe || guide_probe->failed || guide_probe->samples.size()>=2)return;',
               'if(!guide_probe || guide_probe->busy || guide_probe->failed || guide_probe->samples.size()>=2)return;')
    text=patch(text,' auto *proxy=validated_game_proxy(cmd);', ''' if(!fsr_probe_camera(pc,vc)||live_fixture){guide_probe_failure("fsr_camera_or_native_contract");return;}
 if(!fsr_probe_prepare(c,pc))return;
 auto *proxy=validated_game_proxy(cmd);''')
    text=patch(text,'const auto *error=record_sr_guides(c,c.borrows,cmd,proxy,saved,input);proxy->Release();',
               'const auto *error=record_sr_guides(c,c.borrows,cmd,proxy,saved,input);')
    text=patch(text,' if(error){guide_probe_failure(error);return;}',
               ' if(error){proxy->Release();guide_probe_failure(error);return;}')
    text=patch(text,' std::array<ID3D12Resource*,3> textures{c.motion,c.current_depth,c.exposure};', ''' const int result=fsr_probe_evaluate(c,proxy,native,gc,pc,vc);proxy->Release();
 restore_root(cmd,saved,false,true);restore_root(cmd,saved,true,true);cmd->bind_pipeline(a::pipeline_stage::all_compute,{saved.pipeline});commands[cmd]=saved;
 if(result){guide_probe_failure("fsr_dispatch");return;}
 std::array<ID3D12Resource*,5> textures{c.motion,c.current_depth,c.exposure,c.fsr.output,gc};''')
    text=patch(text,'const char *names[]={"motion","depth","exposure"};','const char *names[]={"motion","depth","exposure","fsr-output","source-colour"};')
    text=patch(text,'for(unsigned i=0;i<3;++i){','for(unsigned i=0;i<5;++i){')
    text=text.replace('provider-guide-start.txt','provider-fsr-start.txt')
    text=text.replace('SDKCalls','guideConverterSDKCalls')
    text=text.replace('"files\\\":6','"files\\\":10')
    text=patch(text,'auto &c=*guide_probe;if(c.queue!=q||c.busy)return;auto &cache=c.borrows;', '''auto &c=*guide_probe;if(c.queue!=q||c.busy)return;auto &cache=c.borrows;
 if(c.fsr.pending&&!c.fsr.ready&&!c.failed){
  c.busy=true;guard.unlock();const int result=fsr_probe_create(c);guard.lock();c.busy=false;
  fsr_probe_log(c,"fsr_create",result);if(result)guide_probe_failure("fsr_create");
 }''')
    text=patch(text,'  auto retired=std::move(guide_probe);guard.unlock();retired->borrows.clear_after_idle();', '''  fsr_probe_log(c,"fsr_before_destroy",0);
  c.busy=true;guard.unlock();const int destroy_result=fsr_probe_retire(c);guard.lock();c.busy=false;
  c.log<<"{\\\"stage\\\":\\\"fsr_destroy\\\",\\\"code\\\":"<<destroy_result<<"}\\n";c.log.flush();
  if(destroy_result){cache.blocked=true;guide_probe_failure("fsr_destroy");return;}
  auto retired=std::move(guide_probe);guard.unlock();retired->borrows.clear_after_idle();''')
    probe.write_text(text)
    return text

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--reshade-include',type=Path,required=True)
    p.add_argument('--ngx-include',type=Path,required=True)
    a=p.parse_args();repo=Path(__file__).resolve().parents[2];out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
    text=generate(repo,out/'source')
    with (out/'build-private.log').open('w') as log:
        subprocess.run([sys.executable,str(out/'source/build.py'),'--native-only','--developer-controls',
                        '--reshade-include',str(a.reshade_include.resolve()),'--ngx-include',str(a.ngx_include.resolve()),
                        '--output',str(out/'native')],stdout=log,stderr=subprocess.STDOUT,check=True)
    digest=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
    receipt={'purpose':'bounded real-game FSR evaluation before Native TAA; output not replaced',
             'sourceBase':subprocess.check_output(['git','rev-parse','HEAD'],cwd=repo,text=True).strip(),
             'binarySHA256':digest(out/'native/mcd2-graphics.addon64'),
             'recipeSHA256':digest(Path(__file__)),
             'sourceSHA256':{f:digest(Path(__file__).with_name(f)) for f in ['fsr_probe_driver.hpp','fsr_game_bridge.h']},
             'generatedProbeSHA256':hashlib.sha256(text.encode()).hexdigest(),
             'vendorRuntimeBundled':False,'WindowsQualified':False,'worldUnitsToMetersVerified':False}
    (out/'build-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print('Bounded real-game FSR capture probe compiled; output replacement remains disabled.')
