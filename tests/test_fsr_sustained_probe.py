from pathlib import Path
import importlib.util
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('fsr_sustained',ROOT/'experiments/providers/build_fsr_sustained_probe.py')
probe=importlib.util.module_from_spec(spec)
spec.loader.exec_module(probe)

class FsrSustainedProbeTests(unittest.TestCase):
    def generated(self,frames=256,replace=True,quality=-1,fail_after=0):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory)
            text=probe.generate(ROOT,root,frames,replace,100.0,quality,fail_after)
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

    def test_every_failed_frame_resolves_dirty_history_before_native(self):
        text,_,_=self.generated()
        record=text[text.index('static bool guide_probe_record'):text.index('static void guide_probe_present')]
        self.assertIn('if(!c.history_dirty)return false',record)
        self.assertIn('read_upload(*pass,sizeof(reset_data),reset_data.data())',record)
        self.assertIn('fsr_probe_native_reset(c,cmd,saved,*pass,reset_data,x,y,z)',record)
        self.assertIn('return fail("source_binding_contract")',record)
        self.assertIn('if(result){return fail("fsr_dispatch");}',record)
        self.assertIn('return fail("fsr_output_contract")',record)
        self.assertIn('return fail(error)',record)
        self.assertIn('if(c.history_dirty)return fail("frame_clock")',record)
        self.assertNotIn('guide_probe_failure("',record)
        self.assertLess(record.index('c.fsr_frames>=fsr_probe_frame_limit'),record.index('input_dimensions'))

    def test_quality_is_queried_from_the_selected_sdk(self):
        text,driver,_=self.generated(256,True,1)
        self.assertIn('fsr_probe_quality_mode=1',driver)
        self.assertIn('GetProcAddress(s.bridge,"mcd2_fsr_quality_v1")',driver)
        self.assertIn('const int queried=s.quality(s.session,&q)',driver)
        self.assertIn('q.renderWidth!=c.width||q.renderHeight!=c.height',driver)
        self.assertIn('return -1104',driver)
        self.assertLess(driver.index('s.quality(s.session'),driver.index('s.output=eval_texture'))
        bridge=(ROOT/'experiments/providers/fsr_game_bridge.cpp').read_text()
        self.assertIn('s->query(&s->context,&render.header)',bridge)
        self.assertIn('!std::isfinite(ratio)||ratio<1',bridge)
        self.assertIn('p->providerId=s->provider',bridge)

    def test_rejection_is_bounded_and_does_not_send_invalid_input_to_sdk(self):
        _,driver,_=self.generated(256,True,1,128)
        self.assertIn('fsr_probe_fail_after=128',driver)
        self.assertIn('if(c.fsr_frames==fsr_probe_fail_after)p.size=0',driver)
        bridge=(ROOT/'experiments/providers/fsr_game_bridge.cpp').read_text()
        self.assertLess(bridge.index('p->size!=sizeof(*p)'),bridge.index('s->dispatch(&s->context'))
        with tempfile.TemporaryDirectory() as directory:
            for quality,fail_after in [(-2,0),(5,0),(-1,-1),(-1,256)]:
                with self.assertRaises(ValueError):probe.generate(ROOT,Path(directory),256,True,100,quality,fail_after)

if __name__=='__main__':unittest.main()
