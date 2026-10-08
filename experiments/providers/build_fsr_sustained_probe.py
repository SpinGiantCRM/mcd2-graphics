"""Build a bounded FSR output trial with independent Native history reset."""
from pathlib import Path
import argparse
import hashlib
import importlib.util
import json
import math
import shutil
import subprocess
import sys

HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('first_fsr_probe', HERE/'build_fsr_game_probe.py')
base = importlib.util.module_from_spec(spec)
spec.loader.exec_module(base)

def generate(repo, source, frames=256, replace_output=False, world_to_meters=100.0,
             quality_mode=-1, fail_after=0):
    if not 2 <= frames <= 512 or not math.isfinite(world_to_meters) or world_to_meters <= 0:
        raise ValueError('Bounded frame count and measured positive world scale required')
    if quality_mode not in range(-1,5) or fail_after<0 or fail_after>=frames:
        raise ValueError('Valid AMD quality mode and bounded failure index required')
    text = base.generate(repo, source)
    patch = base.patch
    shutil.copyfile(HERE/'fsr_sustained_control.hpp', source/'src/native/fsr_sustained_control.hpp')
    text = patch(text, '#include "fsr_probe_driver.hpp"', '#include "fsr_probe_driver.hpp"\n#include "fsr_sustained_control.hpp"')
    text = patch(text, ' FsrProbeState fsr;', ' FsrProbeState fsr;\n unsigned fsr_frames=0;bool history_dirty=false;')
    start = text.index('static void guide_probe_record(')
    end = text.index('static void guide_probe_present(', start)
    record = text[start:end].replace('static void guide_probe_record(a::command_list *cmd)',
                                    'static bool guide_probe_record(a::command_list *cmd,uint32_t x,uint32_t y,uint32_t z)')
    record = record.replace('return;', 'return false;')
    record = patch(record, 'if(!guide_probe || guide_probe->busy || guide_probe->failed || guide_probe->samples.size()>=2)return false;',
                   'if(!guide_probe || guide_probe->busy)return false;')
    record = record.replace('std::array<float,632>', 'std::array<float,662>')
    record = record.replace('<<index<<', '<<(c.fsr_frames-1)<<')
    record = patch(record, ' std::array<float,84> pc{};std::array<float,662> vc{};', '''
 // Every unsuccessful frame must resolve dirty Native history before dispatch.
 // A missing source/view binding does not prevent resetting a valid Native pass.
 auto fail=[&](const char *reason){
  guide_probe_failure(reason);if(!c.history_dirty)return false;
  std::array<float,84> reset_data{};
  if(pass&&saved.pipeline&&saved.compute_layout&&!saved.dynamic_offsets
   &&root_push_supported(saved.cp)&&root_push_supported(saved.gp)
   &&read_upload(*pass,sizeof(reset_data),reset_data.data()))
   if(fsr_probe_native_reset(c,cmd,saved,*pass,reset_data,x,y,z))return true;
  // Retain the context/resources and suppress this Native dispatch until a
  // valid reset can be recorded; never feed contaminated history back into TAA.
  return true;
 };
 std::array<float,84> pc{};std::array<float,662> vc{};''')
    record = patch(record, ' for(unsigned i:{36u,37u,44u,45u})', '''
 if(c.failed || c.fsr_frames>=fsr_probe_frame_limit){
  if(c.history_dirty){if(fsr_probe_native_reset(c,cmd,saved,*pass,pc,x,y,z))return true;
   return fail("native_history_reset_unavailable");}
  return false;
 }
 for(unsigned i:{36u,37u,44u,45u})''')
    record = patch(record, ' if(!fsr_probe_camera(pc,vc)||live_fixture)', '''
 if(!fsr_probe_frame(c,vc)){if(c.history_dirty)return fail("frame_clock");return false;}
 if(!fsr_probe_camera(pc,vc)||live_fixture)''')
    record = patch(record, ' auto *native=reinterpret_cast<ID3D12GraphicsCommandList*>(cmd->get_native());', '')
    # Keep only first and last readbacks, regardless of how many SDK frames run.
    record = patch(record, ' GuideProbeSample sample;sample.recording=cmd;unsigned index=unsigned(c.samples.size());', '')
    record = patch(record, ' const int result=fsr_probe_evaluate', ' auto *native=reinterpret_cast<ID3D12GraphicsCommandList*>(cmd->get_native());\n const int result=fsr_probe_evaluate')
    record = patch(record, ' std::array<ID3D12Resource*,5> textures', ' if(c.fsr_frames==0 || c.fsr_frames+1==fsr_probe_frame_limit){\n GuideProbeSample sample;sample.recording=cmd;unsigned index=unsigned(c.samples.size());\n std::array<ID3D12Resource*,5> textures')
    record = patch(record, ' c.samples.push_back(sample);', ' c.samples.push_back(sample);\n }\n ++c.fsr_frames;')
    record = patch(record, 'c.log.flush();\n}', '''c.log.flush();
 if constexpr(fsr_probe_replace_output){
  if(!fsr_probe_output(c,native,go)){guide_probe_failure("fsr_output_contract");return false;}
  c.log<<"{\\\"stage\\\":\\\"fsr_output_replaced\\\",\\\"presentSequence\\\":"<<frame
   <<",\\\"successfulEvaluations\\\":"<<c.fsr_frames<<",\\\"nativeTemporalSkipped\\\":true}\\n";c.log.flush();return true;
 }
 return false;
}''')
    # Route all current-frame failures through the same independent reset.
    import re
    record=re.sub(r'guide_probe_failure\(("[^"\n]+"|error)\);return false;',r'return fail(\1);',record)
    text = text[:start]+record+text[end:]
    text = patch(text, 'if(!c.failed && c.samples.size()<2 && GetTickCount64()-c.started>15000)',
                 'if(!c.failed && c.fsr_frames<fsr_probe_frame_limit && GetTickCount64()-c.started>60000)')
    text = patch(text, 'if(!c.failed && c.samples.size()==2 && !c.signed_capture',
                 'if(!c.failed && c.fsr_frames==fsr_probe_frame_limit && c.samples.size()==2 && !c.signed_capture')
    text = patch(text, 'if((c.saved||c.failed) && cache.recordings.empty()',
                 'if((c.saved||c.failed) && !c.history_dirty && cache.recordings.empty()')
    text = text.replace('provider-fsr-start.txt', 'provider-fsr-sustained-start.txt')
    (source/'src/native/sr_guide_probe.hpp').write_text(text)
    driver_path = source/'src/native/fsr_probe_driver.hpp'
    driver = driver_path.read_text().replace('std::array<float,632>', 'std::array<float,662>')
    driver = patch(driver, ' LARGE_INTEGER previous{},frequency{};', ''' LARGE_INTEGER previous{},frequency{};
 float render_delta=0;uint32_t frame_id=0,previous_id=0;bool frame_gap=false;
''')
    driver = patch(driver, 'static bool fsr_probe_camera', f'''static constexpr unsigned fsr_probe_frame_limit={frames};
static constexpr bool fsr_probe_replace_output={'true' if replace_output else 'false'};
static constexpr float fsr_probe_world_to_meters={world_to_meters!r}f;
static constexpr int fsr_probe_quality_mode={quality_mode};
static constexpr unsigned fsr_probe_fail_after={fail_after};
template<class Capture>
static bool fsr_probe_frame(Capture &c,const std::array<float,662> &vc){{
 auto &s=c.fsr;LARGE_INTEGER now{{}};
 if(!QueryPerformanceFrequency(&s.frequency)||s.frequency.QuadPart<=0||!QueryPerformanceCounter(&now))return false;
 uint32_t stamp=0;memcpy(&stamp,vc.data()+661,sizeof(stamp));
 const bool first=s.previous.QuadPart==0;
 const float delta=first?0.f:float(1000.*double(now.QuadPart-s.previous.QuadPart)/double(s.frequency.QuadPart));
 s.frame_gap=!first&&(stamp!=uint32_t(s.previous_id+1)||delta>250.f);
 s.previous=now;s.previous_id=stamp;s.frame_id=stamp;s.render_delta=delta;
 return !first&&std::isfinite(delta)&&delta>0&&delta<=250.f;
}}
static bool fsr_probe_camera''')
    driver=patch(driver,' decltype(&mcd2_fsr_info_v1) info=nullptr;',
                 ' decltype(&mcd2_fsr_info_v1) info=nullptr;\n decltype(&mcd2_fsr_quality_v1) quality=nullptr;')
    driver=patch(driver,' s.info=reinterpret_cast<decltype(s.info)>(GetProcAddress(s.bridge,"mcd2_fsr_info_v1"));',
                 ' s.info=reinterpret_cast<decltype(s.info)>(GetProcAddress(s.bridge,"mcd2_fsr_info_v1"));\n s.quality=reinterpret_cast<decltype(s.quality)>(GetProcAddress(s.bridge,"mcd2_fsr_quality_v1"));')
    driver=patch(driver,'if(!s.create||!s.dispatch||!s.info||!s.destroy)',
                 'if(!s.create||!s.dispatch||!s.info||!s.quality||!s.destroy)')
    driver=patch(driver,' if(code)return code;\n s.output=eval_texture', ''' if(code)return code;
 for(unsigned mode=0;mode<=4;++mode){
  MCD2FsrQualityV1 q{sizeof(q),mode,s.output_width,s.output_height};
  const int queried=s.quality(s.session,&q);
  c.log<<"{\\\"stage\\\":\\\"fsr_quality_query\\\",\\\"code\\\":"<<queried<<",\\\"mode\\\":"<<mode
   <<",\\\"render\\\":["<<q.renderWidth<<","<<q.renderHeight<<"],\\\"output\\\":["<<q.displayWidth<<","<<q.displayHeight
   <<"],\\\"ratio\\\":"<<q.upscaleRatio<<",\\\"providerId\\\":"<<q.providerId<<"}\\n";c.log.flush();
  if(queried)return queried;
  if(int(mode)==fsr_probe_quality_mode&&(q.renderWidth!=c.width||q.renderHeight!=c.height))return -1104;
 }
 s.output=eval_texture''')
    driver = patch(driver, ' QueryPerformanceCounter(&s.previous);s.ready=true;return 0;', ' s.ready=true;return 0;')
    driver = patch(driver, ''' auto &s=c.fsr;LARGE_INTEGER now{};QueryPerformanceCounter(&now);
 float delta=float(1000.*double(now.QuadPart-s.previous.QuadPart)/double(s.frequency.QuadPart));s.previous=now;''',
                   ' auto &s=c.fsr;const float delta=s.render_delta;uint32_t native_reset=0;memcpy(&native_reset,pc.data()+12,4);')
    driver = driver.replace('c.samples.empty()?1u:0u', '(c.fsr_frames==0||native_reset||s.frame_gap)?1u:0u')
    driver = patch(driver, '.01f};', '(1.f/fsr_probe_world_to_meters)};')
    driver=patch(driver,' if(s.readable)eval_transition', ''' if constexpr(fsr_probe_fail_after>0){
  if(c.fsr_frames==fsr_probe_fail_after)p.size=0; // deterministic bridge rejection; no invalid SDK call
 }
 if(s.readable)eval_transition''')
    driver = driver.replace('c.samples.size()', 'c.fsr_frames')
    driver = driver.replace('worldUnitsToMetersTrial\\\":0.01,\\\"worldUnitsVerified\\\":false',
                            'worldToMeters\\\":'+str(world_to_meters)+',\\\"worldUnitsSource\\\":\\\"game-world-settings\\\"')
    driver = patch(driver, '''  <<",\\\"frameTimeMs\\\":"<<delta''',
                   '''  <<",\\\"viewFrameStamp\\\":"<<s.frame_id<<",\\\"timingSource\\\":\\\"QPC-pre-temporal\\\",\\\"frameTimeMs\\\":"<<delta''')
    driver_path.write_text(driver)
    observer_path=source/'src/native/observer.cpp'
    observer=observer_path.read_text()
    observer=patch(observer,'static void guide_probe_record(a::command_list*);',
                   'static bool guide_probe_record(a::command_list*,uint32_t,uint32_t,uint32_t);')
    observer=patch(observer,'guide_probe_record(cmd);return lean_gate(cmd,shader,x,y,z);',
                   'if(guide_probe_record(cmd,x,y,z))return true;return lean_gate(cmd,shader,x,y,z);')
    observer_path.write_text(observer)
    return text

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('output','reshade-include','ngx-include'):p.add_argument('--'+name,type=Path,required=True)
    p.add_argument('--frames',type=int,default=256)
    p.add_argument('--replace-output',action='store_true')
    p.add_argument('--world-to-meters',type=float,required=True,help='Value observed from this game world, not an assumption')
    p.add_argument('--quality-mode',type=int,default=-1,help='-1 arbitrary scale; 0 Native AA, 1 Quality, 2 Balanced, 3 Performance, 4 Ultra Performance; checked against the active AMD SDK query')
    p.add_argument('--fail-after',type=int,default=0,help='Private deterministic bridge rejection after N successful evaluations; zero disables')
    p.add_argument('--clang-cxx',default='clang++')
    p.add_argument('--mingw-cxx',default='x86_64-w64-mingw32-g++')
    args=p.parse_args();repo=HERE.parents[1];out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
    generate(repo,out/'source',args.frames,args.replace_output,args.world_to_meters,args.quality_mode,args.fail_after)
    with (out/'build-private.log').open('w') as log:
        subprocess.run([sys.executable,str(out/'source/build.py'),'--native-only','--developer-controls',
                        '--reshade-include',str(args.reshade_include.resolve()),'--ngx-include',str(args.ngx_include.resolve()),
                        '--clang-cxx',args.clang_cxx,'--mingw-cxx',args.mingw_cxx,
                        '--output',str(out/'native')],stdout=log,stderr=subprocess.STDOUT,check=True)
    sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
    receipt={'trialOnly':True,'sourceBase':subprocess.check_output(['git','rev-parse','HEAD'],cwd=repo,text=True).strip(),
             'frames':args.frames,'outputReplacement':args.replace_output,'worldToMeters':args.world_to_meters,
             'qualityMode':args.quality_mode,'failAfter':args.fail_after,
             'binarySHA256':sha(out/'native/mcd2-graphics.addon64'),'recipeSHA256':sha(Path(__file__)),
             'generatedSources':{name:sha(out/'source/src/native'/name) for name in
                                 ('sr_guide_probe.hpp','fsr_probe_driver.hpp','fsr_sustained_control.hpp','observer.cpp')},
             'vendorRuntimeBundled':False,'WindowsQualified':False}
    (out/'build-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print('Bounded sustained FSR trial compiled; no installer payload changed.')

if __name__=='__main__':main()
