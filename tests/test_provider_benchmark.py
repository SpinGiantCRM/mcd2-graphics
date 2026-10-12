import copy
import importlib.util
from pathlib import Path
import unittest

path = Path(__file__).resolve().parents[1] / 'experiments/providers/analyze_present_samples.py'
spec = importlib.util.spec_from_file_location('provider_samples', path)
samples = importlib.util.module_from_spec(spec)
spec.loader.exec_module(samples)


def fixture(enabled=True, owned=True):
    scope = dict(kind='scope', qpcFrequency=1000, applicationPresentIntervals=True,
                 physicalDisplayFrames=False, gpuQueries=False, pixelReadbacks=False)
    before = dict(kind='before', module=True, result=0, active=int(enabled), ready=int(owned),
                  fault=0, errors=0, warnings=0, providerFrame=10, prepared=10, images=10,
                  realPresents=10, generatedPresents=10)
    after = dict(before, kind='after', providerFrame=39, prepared=39, images=39,
                 realPresents=39 if owned else 10, generatedPresents=39 if enabled else 10)
    return [scope, before, after] + [dict(kind='sample', counter=n * 10) for n in range(30)]


class ProviderBenchmarkTests(unittest.TestCase):
    def test_guide_records_actual_colour_and_indexed_draw_metadata(self):
        source = (path.parent.parent / 'fg-streamline/guide_recon.cpp').read_text()
        self.assertIn('unsigned(swap->get_color_space())', source)
        self.assertIn('if(captureEnabled){static unsigned lastColour=UINT32_MAX;', source)
        self.assertIn('draw_common(cmd,vertices,instances,false)', source)
        self.assertIn('draw_common(cmd,indices,instances,true)', source)
        self.assertIn('observe(cmd,false,elements,instances,indexed)', source)
        self.assertIn('blend.blend_enable[0]', source)
        self.assertIn('blend.render_target_write_mask[0]', source)

    def test_coverage_observation_is_optional_and_guards_locks(self):
        source = (path.parent.parent / 'fg-streamline/guide_recon.cpp').read_text()
        for name in ('diagnostic_viewports', 'diagnostic_scissors'):
            body = source.split('void ' + name + '(', 1)[1].split('\n', 1)[0]
            self.assertLess(body.index('if(mcd2::process_exit::terminating())return;'),
                            body.index('std::lock_guard lock(mutex);'))
            self.assertLess(body.index('if(!captureEnabled||first!=0)return;'),
                            body.index('std::lock_guard lock(mutex);'))
            self.assertIn('count==1&&values', body)
            self.assertNotIn('cmd->', body)
        self.assertIn('diagnosticViewportKnown=false,diagnosticScissorKnown=false', source)

    def test_sampler_skips_terminal_callbacks_and_preserves_ordinary_unregister(self):
        source = (path.parent / 'present_sampler.cpp').read_text()
        finish = source.split('void finish(', 1)[1].split('extern "C"', 1)[0]
        self.assertLess(finish.index('if(mcd2::process_exit::terminating())return;'),
                        finish.index('std::lock_guard lock(mutex);'))
        self.assertIn('bool AddonInit(HMODULE,HMODULE){mcd2::process_exit::initialize();', source)
        self.assertIn('LPVOID reserved)', source)
        self.assertIn('if(reserved)mcd2::process_exit::mark_terminating();else reshade::unregister_addon(module);', source)
        self.assertIn('src/latency/process_exit.hpp', source)

    def test_sdk_counts_and_cpu_intervals_are_separate_from_physical_fps(self):
        result = samples.analyze(fixture(), True)
        self.assertEqual(result['applicationPresentRate'], 100)
        self.assertEqual(result['SDKCountEquivalentTotal'], 200)
        self.assertFalse(result['physicalDisplayFPSMeasured'])

    def test_retained_off_has_real_counts_but_no_generated_total_claim(self):
        result = samples.analyze(fixture(False), False)
        self.assertEqual(result['SDKRealPresentRate'], 100)
        self.assertEqual(result['SDKGeneratedPresentRate'], 0)
        self.assertIsNone(result['SDKCountEquivalentTotal'])

    def test_clean_off_does_not_report_unowned_sdk_zero_as_measured_fps(self):
        result = samples.analyze(fixture(False, False), False)
        self.assertEqual(result['applicationPresentRate'], 100)
        self.assertIsNone(result['SDKRealPresentRate'])

    def test_missing_faulted_inactive_or_nonadvancing_provider_is_rejected(self):
        for key, value in [('module', False), ('result', -1), ('active', 0), ('ready', 0),
                           ('fault', 1), ('errors', 1), ('warnings', 1), ('generatedPresents', 10)]:
            with self.subTest(key=key):
                rows = fixture()
                rows[2][key] = value
                with self.assertRaises(ValueError):
                    samples.analyze(rows, True)

    def test_multiswapchain_or_wrong_multiplier_counts_are_rejected(self):
        for key, value in [('realPresents', 70), ('generatedPresents', 70)]:
            rows = fixture()
            rows[2][key] = value
            with self.assertRaises(ValueError):
                samples.analyze(rows, True)

    def test_off_generation_is_rejected(self):
        rows = fixture(False)
        rows[2]['generatedPresents'] += 1
        with self.assertRaises(ValueError):
            samples.analyze(rows, False)

    def test_off_ownership_invalid_regressing_or_mismatched_counts_are_rejected(self):
        for key, value in [('ready', 0), ('realPresents', -1), ('realPresents', True),
                           ('realPresents', 9), ('realPresents', 100),
                           ('generatedPresents', '10')]:
            with self.subTest(key=key, value=value):
                rows = fixture(False)
                rows[2][key] = value
                with self.assertRaises(ValueError):
                    samples.analyze(rows, False)

    def test_invalid_scope_frequency_sample_count_and_clock_are_rejected(self):
        base = fixture()
        variants = [base[:32], base + [dict(kind='sample', counter=999)] * 1200]
        for index, key, value in [(0, 'physicalDisplayFrames', True), (0, 'qpcFrequency', 0),
                                  (0, 'qpcFrequency', True), (1, 'kind', 'after'),
                                  (4, 'counter', 0), (4, 'counter', '10')]:
            rows = copy.deepcopy(base)
            rows[index][key] = value
            variants.append(rows)
        for rows in variants:
            with self.assertRaises(ValueError):
                samples.analyze(rows, True)


if __name__ == '__main__':
    unittest.main()
