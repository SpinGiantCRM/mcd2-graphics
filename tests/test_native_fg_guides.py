from pathlib import Path
import importlib.util,os,re,shutil,subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
class NativeFgTests(unittest.TestCase):
 def test_admission_executable(self):
  if os.name=='nt':self.skipTest('Windows CI runs the native admission executable in its configured C++ toolchain step')
  compiler=shutil.which('g++') or shutil.which('clang++')
  if not compiler:self.skipTest('Portable C++ compiler unavailable')
  with tempfile.TemporaryDirectory() as tmp:
   exe=Path(tmp)/'native-fg-test.exe'
   subprocess.run([compiler,'-std=c++20','-Wall','-Wextra','-Werror',str(ROOT/'tests/native_fg_admission_test.cpp'),'-o',str(exe)],check=True)
   subprocess.run([str(exe)],check=True)
 def test_independent_consumer_does_not_replace_native_output(self):
  s=(ROOT/'src/native/native_fg_guides.hpp').read_text()
  for forbidden in ('NVSDK_NGX','c.dispatch(', 'CopyResource(', 'wait_idle(', 'D3D12_HEAP_TYPE_READBACK','ofstream','ExecuteCommandLists('):self.assertNotIn(forbidden,s)
  self.assertIn('#define MCD2_NATIVE_FG_GUIDES 0',s)
  self.assertIn('struct Generation : SrGuideGeneration',s)
  self.assertIn('record_sr_guides(c,c.borrows,cmd,proxy,saved,input)',s)
  self.assertIn('nativeFgContext(authority',s)
  self.assertIn('viewport.width!=unsigned(pc[44])',s)
  self.assertIn('c.gap||reset||stamp!=c.previous+1||c.world!=fsr_runtime::context.world',s)
  self.assertIn('live_fixture||fsr_runtime_owns_source()',s)
 def test_retirement_and_queue_identity_are_required(self):
  s=(ROOT/'src/native/native_fg_guides.hpp').read_text()
  self.assertIn('cache.recordings.empty()&&cache.entries.empty()&&!cache.blocked',s)
  self.assertIn('cache.confirm_reset(cmd,epoch)',s)
  self.assertIn('done==UINT64_MAX',s)
  self.assertIn('native_fg::generation->queue!=q',s)
  self.assertIn('cache.retire_ready(done,frame)',s)
 def test_events_are_wired_and_native_temporal_pass_still_runs(self):
  observer=(ROOT/'src/native/observer.cpp').read_text();lean=(ROOT/'src/native/ngx_lean.hpp').read_text()
  self.assertIn('native_fg_record(cmd);return lean_gate',observer)
  for hook in ('submit(q,cmd)','present(q,guard)','destroy_queue(q)'):self.assertIn('native_fg_'+hook,observer)
  for hook in ('reset(cmd)','begin(cmd)','forget(cmd)'):self.assertIn('native_fg_'+hook,lean)
 def test_isolated_include_closure(self):
  spec=importlib.util.spec_from_file_location('sr_source',ROOT/'experiments/fg-streamline/sr_source_tree.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
  with tempfile.TemporaryDirectory() as tmp:
   source=Path(tmp);module.copy_sr_sources(ROOT,source)
   for file in (source/'src').rglob('*'):
    if file.suffix not in ('.hpp','.h','.cpp'):continue
    for include in re.findall(r'#include "([^"]+)"',file.read_text()):
     if include.startswith('../'):self.assertTrue((file.parent/include).resolve().is_file(),str(file)+' '+include)
  recipe=(ROOT/'experiments/fg-streamline/build_sr_candidate.py').read_text()
  self.assertIn('#define MCD2_NATIVE_FG_GUIDES 1',recipe)
  self.assertNotIn("source/'src/native'/name",recipe)
  self.assertEqual(recipe.count('../../experiments/fg-streamline/fg_camera_math.h'),2)
if __name__=='__main__':unittest.main()
