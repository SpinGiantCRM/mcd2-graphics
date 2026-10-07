"""Keep the tested framework dependency exact and historical archives frozen."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
import zipfile

ROOT = Path(__file__).resolve().parents[1]
QUAL = ROOT / 'qualification/fg-release'


class FrameworkDependencyTests(unittest.TestCase):
    def reproduce(self, lock_path, readme, archive_name, *, corrupt=False):
        lock = json.loads(lock_path.read_text())['dependencies']['ReShade']
        archive = QUAL / 'dependencies' / archive_name
        self.assertEqual(hashlib.sha256(archive.read_bytes()).hexdigest(), lock['archiveSHA256'])
        with tempfile.TemporaryDirectory() as directory:
            work = Path(directory)
            with zipfile.ZipFile(archive) as zipped:
                self.assertEqual(set(zipped.namelist()),
                                 {'LICENSE.md', 'README.md', 'ReShade64.dll', 'build-receipt.json'})
                for name in zipped.namelist():
                    (work / name).write_bytes(zipped.read(name))
            self.assertEqual(hashlib.sha256((work / 'ReShade64.dll').read_bytes()).hexdigest(),
                             lock['sha256'])
            if corrupt:
                with (work / 'ReShade64.dll').open('ab') as stream:
                    stream.write(b'changed after qualification')
            result = subprocess.run([
                sys.executable, str(QUAL / 'build_framework_dependency.py'),
                '--runtime', str(work / 'ReShade64.dll'),
                '--receipt', str(work / 'build-receipt.json'),
                '--license', str(work / 'LICENSE.md'), '--lock', str(lock_path),
                '--readme', str(readme), '--output', str(work / 'rebuilt.zip'),
            ], capture_output=True, text=True)
            if corrupt:
                self.assertNotEqual(result.returncode, 0)
                self.assertFalse((work / 'rebuilt.zip').exists())
            else:
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual((work / 'rebuilt.zip').read_bytes(), archive.read_bytes())

    def test_msvc_candidate_reproduces_exact_dependency(self):
        self.reproduce(ROOT / 'dependencies.lock.json', QUAL / 'ReShade-dependency-README.md',
                       'ReShade-PR435-4eb9056-reset-epoch-msvc-x64.zip')

    def test_original_failing_archive_keeps_exact_frozen_inputs(self):
        lock = json.loads((QUAL / 'original-framework-dependency.json').read_text())['dependencies']['ReShade']
        archive = QUAL / 'dependencies/ReShade-PR435-4eb9056-reset-epoch-x64.zip'
        self.assertEqual(hashlib.sha256(archive.read_bytes()).hexdigest(), lock['archiveSHA256'])
        with zipfile.ZipFile(archive) as zipped:
            self.assertEqual(set(zipped.namelist()),
                             {'LICENSE.md', 'README.md', 'ReShade64.dll', 'build-receipt.json'})
            self.assertEqual(hashlib.sha256(zipped.read('ReShade64.dll')).hexdigest(), lock['sha256'])
            self.assertEqual(zipped.read('README.md'), (QUAL / 'original-framework-README.md').read_bytes())
            self.assertEqual(json.loads(zipped.read('build-receipt.json'))['patchSHA256'], lock['patchSHA256'])

    def test_changed_runtime_cannot_reuse_qualification(self):
        self.reproduce(ROOT / 'dependencies.lock.json', QUAL / 'ReShade-dependency-README.md',
                       'ReShade-PR435-4eb9056-reset-epoch-msvc-x64.zip', corrupt=True)

    def test_candidate_preserves_reset_source_and_build_provenance(self):
        lock = json.loads((ROOT / 'dependencies.lock.json').read_text())['dependencies']['ReShade']
        receipt_bytes = (QUAL / 'msvc-framework-build-receipt.json').read_bytes()
        receipt = json.loads(receipt_bytes)
        self.assertEqual(hashlib.sha256(receipt_bytes).hexdigest(), lock['buildReceiptSHA256'])
        self.assertEqual(receipt['binarySHA256'], lock['sha256'])
        self.assertEqual(receipt['patchSHA256'], hashlib.sha256(
            (QUAL / 'reshade-reset-epoch.patch').read_bytes()).hexdigest())
        self.assertEqual(receipt['sourceDiffSHA256'], hashlib.sha256(
            (QUAL / 'msvc-framework-source.patch').read_bytes()).hexdigest())
        self.assertTrue(receipt['baseCleanBeforePatch'] and receipt['approvedPatchOnly'])
        self.assertEqual(len(receipt['submodules']), 11)
        self.assertNotIn(lock['file'], json.loads((ROOT / 'manifest.json').read_text())['files'])


if __name__ == '__main__':
    unittest.main()
