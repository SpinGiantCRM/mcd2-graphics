from pathlib import Path
import copy
import hashlib
import importlib.util
import json
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]

def load(name):
    spec = importlib.util.spec_from_file_location(name, ROOT / 'experiments/providers' / (name + '.py'))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module

recipe, runner = load('build_fsr_probe'), load('run_fsr_probe')

def evidence():
    rows = []
    for trial in range(10):
        hdr = trial >= 5
        rows.append({'stage': 'trial', 'trial': trial, 'quality': trial % 5, 'HDR': hdr})
        rows += [{'stage': name, 'result': 0} for name in ['create-context', 'confirm-active-provider']]
        for frame in range(16):
            rows += [{'stage': 'dispatch-owned-inputs', 'result': 0},
                     {'stage': 'readback', 'frame': frame, 'HDR': hdr, 'valid': True, 'maximum': 8 if hdr else 1}]
        rows.append({'stage': 'destroy-context-after-fence', 'result': 0})
    rows.append({'stage': 'complete', 'dispatches': 160, 'SDKerrors': 0, 'SDKwarnings': 0})
    return rows

class FsrProbeTests(unittest.TestCase):
    def test_full_matrix_gate_and_failures(self):
        rows = evidence()
        self.assertTrue(runner.gate(rows, 0, False))
        self.assertFalse(runner.gate(rows, 5, False))
        self.assertFalse(runner.gate(rows, 0, True))
        for index, row in enumerate(rows):
            if row['stage'] in ['readback', 'create-context', 'dispatch-owned-inputs', 'destroy-context-after-fence']:
                changed = copy.deepcopy(rows)
                changed[index]['valid' if row['stage'] == 'readback' else 'result'] = False if row['stage'] == 'readback' else 3
                self.assertFalse(runner.gate(changed, 0, False))
        for kind in ['trial', 'readback', 'dispatch-owned-inputs', 'destroy-context-after-fence', 'complete']:
            self.assertFalse(runner.gate([r for r in rows if r['stage'] != kind], 0, False))
        bad = copy.deepcopy(rows); bad[-1]['SDKwarnings'] = 1
        self.assertFalse(runner.gate(bad, 0, False))
        bad = copy.deepcopy(rows)
        for row in bad:
            if row['stage'] == 'readback' and row['HDR']:
                row['maximum'] = 1
        self.assertFalse(runner.gate(bad, 0, False))

    def test_pinned_headers_checked_before_compilation(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary); first = root / next(iter(recipe.HEADERS))
            first.parent.mkdir(parents=True); first.write_bytes(b'changed')
            with patch.object(recipe.subprocess, 'run') as run:
                with self.assertRaisesRegex(ValueError, 'Pinned FSR'):
                    recipe.build(root, root, root / 'out')
                run.assert_not_called()

    def test_recipe_frame_guard_and_no_runtime_download_or_copy(self):
        calls = []
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            for name in recipe.HEADERS:
                path = root / name; path.parent.mkdir(parents=True, exist_ok=True); path.write_bytes(b'fixture')
            pins = {name: hashlib.sha256(b'fixture').hexdigest() for name in recipe.HEADERS}
            def fake_run(command, **kwargs):
                calls.append(command)
                for part in command:
                    if part.startswith('/out:'):
                        Path(part[5:]).write_bytes(b'fixture-executable')
            with patch.object(recipe, 'HEADERS', pins), patch.object(recipe.subprocess, 'run', side_effect=fake_run):
                recipe.build(root, root, root / 'out', 'compiler', 'linker')
            self.assertEqual(len(calls), 2)
            self.assertIn('/MT', calls[0]); self.assertIn('/clang:-Werror=frame-larger-than', calls[0])
            receipt = json.loads((root / 'out/build-receipt.json').read_text())
            self.assertFalse(receipt['runtimeBundled']); self.assertFalse(receipt['gameIntegrationQualified'])
            self.assertFalse(any(p.suffix == '.dll' for p in (root / 'out').iterdir()))

    def test_runtime_tamper_and_game_prefix_refused(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary); prefix = root / 'experiment'; (prefix / 'pfx').mkdir(parents=True)
            for forbidden in [root / 'steamapps/compatdata/1912410', root / '1912410']:
                with self.assertRaisesRegex(ValueError, 'isolated prefix'):
                    runner.validate(root, forbidden)
            (root / 'fsr-owned-inputs.exe').write_bytes(b'fixture')
            receipt = {'binarySHA256': hashlib.sha256(b'fixture').hexdigest(), 'runtimeBundled': False, 'gameIntegrationQualified': False}
            (root / 'build-receipt.json').write_text(json.dumps(receipt))
            (root / 'amd_fidelityfx_upscaler_dx12.dll').write_bytes(b'changed')
            with self.assertRaisesRegex(ValueError, 'pinned runtime'):
                runner.validate(root, prefix)
            with patch.object(runner, 'RUNTIME', {'amd_fidelityfx_upscaler_dx12.dll': hashlib.sha256(b'changed').hexdigest()}):
                runner.validate(root, prefix)
                for name in ['dxgi.dll', 'd3d12.dll', 'd3d12core.dll', 'amdxc64.dll', 'sl.interposer.dll']:
                    (root / name).write_bytes(b'unsafe')
                    with self.assertRaisesRegex(ValueError, 'Unexpected proxy'):
                        runner.validate(root, prefix)
                    (root / name).unlink()

if __name__ == '__main__':
    unittest.main()
