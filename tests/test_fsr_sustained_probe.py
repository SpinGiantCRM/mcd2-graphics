from pathlib import Path
import importlib.util
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('fsr_sustained',ROOT/'experiments/providers/build_fsr_sustained_probe.py')
probe=importlib.util.module_from_spec(spec)
spec.loader.exec_module(probe)

class FsrSustainedProbeTests(unittest.TestCase):
    def generated(self,frames=256,replace=True):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory)
            text=probe.generate(ROOT,root,frames,replace,100.0)
            driver=(root/'src/native/fsr_probe_driver.hpp').read_text()
            observer=(root/'src/native/observer.cpp').read_text()
        return text,driver,observer

    def test_bounded_output_trial_and_native_reset_are_separate_from_ngx(self):
        text,driver,observer=self.generated()
        self.assertIn('provider-fsr-sustained-start.txt',text)
        self.assertIn('fsr_probe_frame_limit=256',driver)
        self.assertIn('fsr_probe_replace_output=true',driver)
        self.assertIn('if(guide_probe_record(cmd,x,y,z))return true',observer)
        self.assertIn('fsr_probe_native_reset(c,cmd,saved,*pass,pc,x,y,z)',text)
        self.assertIn('!c.history_dirty && cache.recordings.empty()',text)
        self.assertIn('if constexpr(fsr_probe_replace_output)',text)
        self.assertIn('fsr_probe_output(c,native,go)',text)
        self.assertNotIn('mcd2_ngx_create',text)

    def test_first_and_last_readbacks_bound_memory(self):
        text,driver,_=self.generated(512)
        self.assertIn('if(c.fsr_frames==0 || c.fsr_frames+1==fsr_probe_frame_limit)',text)
        self.assertLess(text.index('fsr_probe_evaluate(c,proxy'),text.index('if(c.fsr_frames==0 || c.fsr_frames+1==fsr_probe_frame_limit)'))
        self.assertIn('c.fsr_frames==fsr_probe_frame_limit && c.samples.size()==2',text)
        self.assertIn('c.fsr_frames==0||native_reset||s.frame_gap',driver)
        self.assertNotIn('c.samples.empty()?1u:0u',driver)

    def test_clock_is_captured_before_creation_and_uses_actual_view_stamp(self):
        text,driver,_=self.generated()
        self.assertLess(text.index('fsr_probe_frame(c,vc)'),text.index('fsr_probe_prepare(c,pc)'))
        self.assertIn('std::array<float,662>',text)
        self.assertIn('memcpy(&stamp,vc.data()+661',driver)
        self.assertIn('QPC-pre-temporal',driver)
        self.assertIn('const float delta=s.render_delta',driver)
        self.assertIn('1.f/fsr_probe_world_to_meters',driver)

    def test_observational_default_and_bad_bounds(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory)
            probe.generate(ROOT,root)
            self.assertIn('fsr_probe_replace_output=false',(root/'src/native/fsr_probe_driver.hpp').read_text())
            for frames,units in [(1,100),(513,100),(256,0),(256,float('nan'))]:
                with self.assertRaises(ValueError):probe.generate(ROOT,root,frames,False,units)

    def test_history_upload_is_immutable_and_output_copy_checks_compatibility(self):
        text=(ROOT/'experiments/providers/fsr_sustained_control.hpp').read_text()
        self.assertIn('memcpy(static_cast<char*>(mapped)+48,&reset',text)
        self.assertIn('cache.record(cmd,key,frame)',text)
        self.assertIn('D3D12_HEAP_TYPE_UPLOAD',text)
        self.assertIn('source.Format!=target.Format',text)
        self.assertIn('source.SampleDesc.Count!=target.SampleDesc.Count',text)
        self.assertIn('c.history_dirty=true',text)
        self.assertIn('c.history_dirty=false',text)

if __name__=='__main__':unittest.main()
