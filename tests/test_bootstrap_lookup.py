from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


class BootstrapLookupTests(unittest.TestCase):
    def test_all_runtime_bootstrap_consumers_use_exact_application_path(self):
        for name in ('experiments/fg-streamline/guide_recon.cpp',
                     'experiments/fg-streamline/fg_controls.hpp',
                     'experiments/fg-streamline/build_game_candidate.py',
                     'experiments/fg-streamline/owned_inputs_probe.cpp',
                     'src/latency/latency_addon.cpp'):
            with self.subTest(source=name):
                source = (ROOT / name).read_text()
                self.assertNotIn('GetModuleHandleW(L"dxgi.dll")', source)
                self.assertIn('mcd2::providers::bootstrapModule()', source)
        recipe = (ROOT / 'experiments/fg-streamline/build_probe.py').read_text()
        self.assertIn("'src/providers/bootstrap_module.hpp'", recipe)

    def test_path_is_checked_heap_backed_and_handle_is_not_cached(self):
        header = (ROOT / 'src/providers/bootstrap_module.hpp').read_text()
        self.assertIn('std::make_unique<wchar_t[]>(32768)', header)
        self.assertIn('if (!length || length >= 32768)', header)
        self.assertIn('static const std::wstring path', header)
        self.assertIn('GetModuleHandleW(path.c_str())', header)
        for forbidden in ('LoadLibrary', 'FreeLibrary', 'static HMODULE', 'wchar_t buffer['):
            self.assertNotIn(forbidden, header)

    def test_normal_recording_reset_does_not_query_shared_cleanup(self):
        source = (ROOT / 'experiments/fg-streamline/guide_recon.cpp').read_text()
        self.assertIn('sharedReady=ownerClosing&&amdRetiredForClose;', source)
        self.assertIn('if(!close){if(sharedReady)try_shared_sdk_quiesce();return;}', source)


if __name__ == '__main__':
    unittest.main()
