from pathlib import Path
import importlib.util
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('fsr_game_probe', ROOT/'experiments/providers/build_fsr_game_probe.py')
probe = importlib.util.module_from_spec(spec)
spec.loader.exec_module(probe)

class FsrGameProbeTests(unittest.TestCase):
    def test_generated_probe_owns_an_independent_capture_and_retirement(self):
        with tempfile.TemporaryDirectory() as directory:
            text = probe.generate(ROOT, Path(directory))
        self.assertIn('provider-fsr-start.txt', text)
        self.assertNotIn('provider-guide-start.txt', text)
        self.assertIn('guide_probe->busy || guide_probe->failed', text)
        self.assertIn('guard.unlock();const int result=fsr_probe_create(c);guard.lock()', text)
        self.assertIn('cache.recordings.empty() && cache.entries.empty()', text)
        self.assertLess(text.index('fsr_probe_retire(c)'),text.index('retired->borrows.clear_after_idle()'))
        self.assertIn('if(destroy_result){cache.blocked=true', text)
        self.assertIn('std::array<DenseReadback,5>',text)
        self.assertIn('"fsr-output","source-colour"',text)
        self.assertIn('fsr_probe_camera(pc,vc)||live_fixture',text)

    def test_changed_source_contract_fails_closed(self):
        with self.assertRaises(ValueError):probe.patch('unrelated','expected','replacement')
        with self.assertRaises(ValueError):probe.patch('expected expected','expected','replacement')

    def test_sdk_bridge_requires_bounded_inputs_and_completed_retirement(self):
        text=(ROOT/'experiments/providers/fsr_game_bridge.cpp').read_text()
        self.assertIn('completed==UINT64_MAX||completed<required',text)
        self.assertIn('if(s->required&~supplied)return -1010',text)
        self.assertIn('active.versionId!=s->provider',text)
        self.assertIn('LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32',text)
        self.assertIn('std::strstr(names[i],"3.1.5")',text)
        self.assertIn('p->reset>1',text)
        self.assertNotIn('ffxConfigure',text)

if __name__=='__main__':unittest.main()
