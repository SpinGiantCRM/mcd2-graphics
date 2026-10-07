"""Regress the mixed directory names in a real Windows Kits installation."""
import importlib.util
from pathlib import Path
import tempfile
import unittest
import subprocess
import json
from unittest.mock import patch

spec = importlib.util.spec_from_file_location('windows_candidate', Path(__file__).with_name('build_windows_candidate.py'))
candidate = importlib.util.module_from_spec(spec)
spec.loader.exec_module(candidate)


class WindowsBuildChecks(unittest.TestCase):
    def test_current_package_ui_is_reused_with_metadata_provenance(self):
        with tempfile.TemporaryDirectory() as temporary:
            artifact = Path(temporary) / 'artifact'
            # Exercise git's canonical blobs, as the build does. Windows CRLF
            # working-tree bytes are not the source bytes recorded by the receipt.
            candidate.copy_current_ui(candidate.REPO, artifact)
            receipt = json.loads((artifact / 'ui/build-receipt.json').read_text())
            for name, expected in receipt['payloadSHA256'].items():
                self.assertEqual(candidate.sha(artifact / 'ui/Pak' / name), expected)

    def test_stale_metadata_source_is_refused_before_writing(self):
        with tempfile.TemporaryDirectory() as temporary:
            artifact = Path(temporary) / 'artifact'
            with patch.object(candidate.subprocess, 'check_output', return_value=b'stale source'):
                with self.assertRaisesRegex(RuntimeError, 'source changed'):
                    candidate.copy_current_ui(candidate.REPO, artifact)
            self.assertFalse(artifact.exists())

    def test_framework_patch_has_canonical_checkout_and_hash_bytes(self):
        relative = 'qualification/fg-release/reshade-reset-epoch.patch'
        self.assertNotIn(b'\r', (candidate.REPO / relative).read_bytes())
        attributes = subprocess.check_output(
            ['git', '-C', str(candidate.REPO), 'check-attr', 'eol', relative], text=True)
        self.assertEqual(attributes.strip(), relative + ': eol: lf')

    def test_unversioned_components_and_incomplete_versions_are_ignored(self):
        with tempfile.TemporaryDirectory() as temporary:
            parent = Path(temporary)
            for name in ['wdf', '10.0.9.0', '10.0.26100.0', '10.0.99999.0']:
                (parent / name).mkdir()
            for name in ['10.0.9.0', '10.0.26100.0']:
                (parent / name / 'um').mkdir()
            self.assertEqual(candidate.latest_version(parent, ['um']).name, '10.0.26100.0')

    def test_missing_complete_toolchain_fails(self):
        with tempfile.TemporaryDirectory() as temporary:
            parent = Path(temporary)
            (parent / 'wdf').mkdir()
            with self.assertRaises(RuntimeError):
                candidate.latest_version(parent, ['um'])


if __name__ == '__main__':
    unittest.main()
