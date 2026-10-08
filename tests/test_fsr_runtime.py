from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


class FsrRuntimeSourceTests(unittest.TestCase):
    """Source guards complement codec tests and exact-binary gameplay checks."""

    def setUp(self):
        self.source = (ROOT / "src/native/fsr_runtime.hpp").read_text()
        self.record = self.source.split("static bool record(", 1)[1].split("static int load(", 1)[0]
        self.present = self.source.split("static void present(", 1)[1]

    def test_default_is_inactive_and_dependencies_are_verified_before_load(self):
        self.assertIn('#define MCD2_FSR_BRIDGE_SHA256 ""', self.source)
        self.assertIn('strlen(MCD2_FSR_BRIDGE_SHA256)!=64', self.present)
        loader = self.source.split('static int load(', 1)[1].split('static void present(', 1)[0]
        self.assertLess(loader.index('hash_matches('), loader.index('LoadLibraryExW('))
        self.assertIn('LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32', loader)

    def test_busy_snapshot_reuse_does_not_refresh_its_age(self):
        self.assertIn('now-menuReadAt<=1000', self.present)
        self.assertIn('if(valid&&!useCachedMenu){cachedMenu=menu;menuReadAt=now;}', self.present)
        self.assertIn('else if(!valid&&menuResult!=-2)cachedMenu={};', self.present)
        self.assertIn('now-cachedContext.observedAtMs<=3000', self.present)
        self.assertIn('contextAt=packet.observedAtMs', self.present)

    def test_transient_input_skips_sdk_and_resets_both_histories(self):
        self.assertIn('return transient()', self.record)
        transient = self.record.split('auto transient=', 1)[1].split('if(!current', 1)[0]
        self.assertIn('c.force_reset=true', transient)
        self.assertIn('native_reset(c,cmd,saved,*pass,pc,x,y,z)', transient)
        self.assertNotIn('c.dispatch(', transient)
        self.assertIn('c.gap||c.force_reset?1u:0u', self.record)
        self.assertIn('if(c.info(c.session,&info)||info.errors)return fail(28)', self.record)

    def test_eligibility_and_exact_source_ack_precede_real_dispatch(self):
        self.assertIn('c.quality(c.session,&quality)', self.present)
        self.assertIn('p::sourceAcknowledged(state,context)', self.record)
        self.assertIn('unsigned(pc[36])==state.renderWidth', self.record)
        self.assertIn('GetTickCount64()-c.started>15000', self.present)
        self.assertIn('1000.f/context.worldToMetersMilli', self.record)

    def test_retirement_precedes_source_restore_and_never_shuts_down_ngx(self):
        self.assertIn('if(c.history_dirty||!cache.recordings.empty()||!cache.entries.empty()||cache.blocked)return', self.present)
        self.assertLess(self.present.index('c.destroy(c.session,cache.fence,cache.next_fence)'),
                        self.present.index('state.sourceScaleMicro=100000000'))
        self.assertIn('stop(0,false)', self.record)
        self.assertNotIn('mcd2_ngx_shutdown', self.source)

    def test_continuous_path_has_no_readback_or_trial_frame_limit(self):
        for diagnostic in ['D3D12_HEAP_TYPE_READBACK', 'fsr_probe_frame_limit', 'provider-fsr-start.txt', 'c.samples']:
            self.assertNotIn(diagnostic, self.source)
        observer = (ROOT / 'src/native/observer.cpp').read_text()
        self.assertIn('if(fsr_runtime_record(cmd,x,y,z))return true', observer)
        self.assertIn('fsr_runtime_submit(q,cmd)', observer)


if __name__ == '__main__':
    unittest.main()
