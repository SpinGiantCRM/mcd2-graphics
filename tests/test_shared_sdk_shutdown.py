from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


class SharedSdkShutdownTests(unittest.TestCase):
    def test_native_readiness_refuses_terminal_busy_or_retained_generations(self):
        source = (ROOT / 'src/native/observer.cpp').read_text()
        body = source.split('int mcd2_sr_ngx_retired_v1() {', 1)[1].split('\n}', 1)[0]
        self.assertLess(body.index('terminating()'), body.index('std::unique_lock'))
        self.assertIn('std::try_to_lock', body)
        self.assertIn('if(!guard.owns_lock())return -1;', body)
        self.assertIn('return live_fixture?0:1;', body)
        self.assertNotIn('SrProvider', body)

    def test_sdk_readiness_is_atomic_and_published_only_after_device_binding(self):
        source = (ROOT / 'experiments/fg-streamline/streamline_probe_bridge.cpp').read_text()
        self.assertIn('std::atomic<bool> sharedShutdownReady{false};', source)
        query = source.split('int mcd2_fg_shared_shutdown_ready_v1(){', 1)[1].split('\n}', 1)[0]
        self.assertIn('sharedShutdownReady.load(std::memory_order_acquire)', query)
        for unsafe in ('boundDevice', 'initialized', 'nvidiaFgLoaded'):
            self.assertNotIn(unsafe, query)
        self.assertIn('if(!result){boundDevice=device;sharedShutdownReady.store(!nvidiaFgLoaded,std::memory_order_release);}', source)
        shutdown = source.split('int mcd2_sl_shutdown(){', 1)[1].split('\n', 1)[0]
        self.assertLess(shutdown.index('sharedShutdownReady.store(false'), shutdown.index('shutdown()'))

    def test_guide_requires_both_owners_and_quiesces_outside_its_mutex(self):
        source = (ROOT / 'experiments/fg-streamline/guide_recon.cpp').read_text()
        body = source.split('void try_shared_sdk_quiesce(){', 1)[1].split('\nvoid try_retirement()', 1)[0]
        self.assertTrue(body.lstrip().startswith('if(mcd2::process_exit::terminating())return;'))
        self.assertIn('admission.nativeNgxRetired=retired&&retired()==1;', body)
        self.assertIn('admission.sdkHasDeviceWithoutNvidiaFg=ready&&ready()==1;', body)
        self.assertIn('!quiesce||!mcd2::providers::admitSharedSdkShutdown(admission)||sharedQuiesceIssued.exchange(true)', body)
        self.assertIn('admission.amdPresenterRetired=amdRetiredForClose;}', body)
        # Query and callback sites are between separate mutex scopes, avoiding
        # guide -> native/latency -> guide lock inversion during SDK retirement.
        self.assertLess(body.index('amdRetiredForClose;}'), body.index('retired()==1'))
        self.assertLess(body.index('const int result=quiesce();'), body.index('std::lock_guard lock(mutex);privateLog'))
        retire = source.split('void try_retirement(){', 1)[1].split('\nint token_identity', 1)[0]
        self.assertIn('if(!result){retiring=false;amdRetiredForClose=ownerClosing;}', retire)
        self.assertIn('if(!result)try_shared_sdk_quiesce();return;', retire)
        notify = source.split('void mcd2_fg_sr_retired(){', 1)[1].split('\nextern "C"', 1)[0]
        self.assertIn('if(amd_owner()){try_shared_sdk_quiesce();return;}', notify)
        self.assertIn('const int result=quiesce?quiesce():-1;', notify)  # Existing NVIDIA path.
        swap = source.split('void swap_destroy(', 1)[1].split('\nvoid targets', 1)[0]
        self.assertIn('ownerClosing=!resize;', swap)
        self.assertIn('if(resize){std::lock_guard lock(mutex);retiring=false;return;}', swap)

    def test_exports_and_header_receipts_survive_both_build_hosts(self):
        recipe = (ROOT / 'experiments/fg-streamline/build_probe.py').read_text()
        self.assertIn("exports.append('mcd2_fg_shared_shutdown_ready_v1')", recipe)
        self.assertIn("'src/providers/shared_sdk_shutdown.hpp'", recipe)
        workflow = (ROOT / '.github/workflows/portable-checks.yml').read_text()
        self.assertEqual(workflow.count('tests/shared_sdk_shutdown_test.cpp'), 2)


if __name__ == '__main__':
    unittest.main()
