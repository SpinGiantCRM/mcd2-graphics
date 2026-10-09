from pathlib import Path
import hashlib, json, tempfile, unittest
from unittest.mock import patch
import build_amd_latency as recipe

class AntiLagBuildTests(unittest.TestCase):
    def test_unmodified_pinned_sdk(self):
        for name, expected in recipe.PINNED.items():
            self.assertEqual(hashlib.sha256((recipe.ROOT / 'third-party/AntiLag2' / name).read_bytes()).hexdigest(), expected)

    def test_sdk_tamper_refused_before_compilation(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);(root/'third-party/AntiLag2').mkdir(parents=True)
            (root/'third-party/AntiLag2/ffx_antilag2_dx12.h').write_bytes(b'changed')
            with patch.object(recipe,'ROOT',root),patch.object(recipe.subprocess,'run') as run:
                with self.assertRaisesRegex(ValueError,'Pinned'):recipe.build(root,root/'out')
                run.assert_not_called()

    def test_bridge_frame_guard_and_fixture_isolation(self):
        calls=[]
        with tempfile.TemporaryDirectory() as directory:
            out=Path(directory);sysroot=out/'sysroot'
            def fake_run(command,**kwargs):
                calls.append(command)
                for part in command:
                    if part.startswith('/out:'):Path(part[5:]).write_bytes(b'fixture')
            with patch.object(recipe.subprocess,'run',side_effect=fake_run):
                binary=recipe.build(sysroot,out,'clang','link',True)
            self.assertEqual(binary.name,'mcd2-antilag2-bridge.dll')
            self.assertEqual(len(calls),8)
            self.assertTrue(all('/MT' in c and '/clang:-Werror=frame-larger-than' in c for c in calls if c[0]=='clang'))
            self.assertIn('/export:mcd2_al2_abi',calls[1])
            self.assertEqual(calls[-1][0],'link')
            record=json.loads((out/'build-receipt.json').read_text())
            self.assertFalse(record['runtimeQualified']);self.assertFalse(record['vendorRuntimeBundled'])
            self.assertEqual(len(record['testSourceSHA256']),3)
            self.assertEqual(record['contractSHA256'],hashlib.sha256((recipe.ROOT/'src/latency/amd_fg_private_data.hpp').read_bytes()).hexdigest())
        source=(recipe.ROOT/'experiments/fg-streamline/build_windows_candidate.py').read_text()
        # Explicit copy allowlist, not a glob of the directory containing the fake driver.
        self.assertIn("for name in ['mcd2-antilag2-bridge.dll', 'build-receipt.json']:",source)
        self.assertIn("run(amd_latency / 'amd-antilag-test.exe')",source)
        self.assertNotIn("amd_latency.rglob",source)

if __name__=='__main__':unittest.main()
