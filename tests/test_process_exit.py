from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


class ProcessExitTests(unittest.TestCase):
    def test_terminal_guards_precede_locks_and_sdk_cleanup(self):
        source = (ROOT / 'src/latency/latency_addon.cpp').read_text()
        self.assertIn('void cleanup(){\n if(mcd2::process_exit::terminating())return;', source)
        self.assertIn('void AddonUninit(HMODULE,HMODULE){\n if(mcd2::process_exit::terminating())return;', source)
        self.assertIn('if(reserved)mcd2::process_exit::mark_terminating();else reshade::unregister_addon(h)', source)
        # Ordinary unload keeps its existing worker join, SDK drains and unload.
        self.assertIn('if(settingsWorker.joinable())settingsWorker.join();cleanup();amdProvider.unload();', source)
        self.assertIn('if(c){c->shutdown();c->drain();}provider.shutdown();', source)
        for name in ('coordinatorLifetime', 'amdCoordinatorLifetime', 'settingsWorkerLifetime'):
            self.assertIn(name, source)

    def test_query_is_resolved_before_callbacks_not_in_dllmain(self):
        source = (ROOT / 'src/latency/latency_addon.cpp').read_text()
        self.assertIn('bool AddonInit(HMODULE module,HMODULE){\n mcd2::process_exit::initialize();', source)
        header = (ROOT / 'src/latency/process_exit.hpp').read_text()
        self.assertIn('RtlDllShutdownInProgress', header)
        self.assertNotIn('LoadLibrary', header)
        self.assertNotIn('WaitFor', header)
        self.assertNotIn('Sleep(', header)
        self.assertNotIn('ExitProcess(', header)

    def test_generated_candidate_retains_guards_and_header_provenance(self):
        recipe = (ROOT / 'experiments/fg-streamline/build_game_candidate.py').read_text()
        self.assertIn("('process_exit.hpp','process_lifetime.hpp')", recipe)
        self.assertNotIn("source.replace(' if(mcd2::process_exit::terminating())return;'", recipe)


if __name__ == '__main__':
    unittest.main()
