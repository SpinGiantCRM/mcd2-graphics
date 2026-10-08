"""Regress the mixed directory names in a real Windows Kits installation."""
import importlib.util
from pathlib import Path
import tempfile
import unittest
import subprocess
import json
import re
from unittest.mock import patch

spec = importlib.util.spec_from_file_location('windows_candidate', Path(__file__).with_name('build_windows_candidate.py'))
candidate = importlib.util.module_from_spec(spec)
spec.loader.exec_module(candidate)
source_spec = importlib.util.spec_from_file_location('sr_source_tree', Path(__file__).with_name('sr_source_tree.py'))
source_tree = importlib.util.module_from_spec(source_spec)
source_spec.loader.exec_module(source_tree)


class WindowsBuildChecks(unittest.TestCase):
    def test_isolated_sr_source_contains_its_relative_include_closure(self):
        with tempfile.TemporaryDirectory() as temporary:
            source = Path(temporary) / 'source'
            source_tree.copy_sr_sources(candidate.REPO, source)
            # Check real relative includes recursively, including FSR's C ABI
            # and the provider snapshots consumed by the native observer.
            pending = [source / 'src/native/observer.cpp']
            visited = set()
            while pending:
                path = pending.pop().resolve()
                if path in visited:
                    continue
                visited.add(path)
                for name in re.findall(r'^\s*#include "([^"]+)"', path.read_text(), re.M):
                    if name in ('reshade.hpp', 'ngx-research/nvsdk_ngx.h'):
                        continue  # separately verified external SDK include
                    dependency = (path.parent / name).resolve()
                    self.assertTrue(dependency.is_relative_to(source.resolve()), name)
                    self.assertTrue(dependency.is_file(), str(dependency.relative_to(source)))
                    pending.append(dependency)
            self.assertIn((source / 'experiments/providers/fsr_game_bridge.h').resolve(), visited)
            self.assertIn((source / 'src/providers/sr_runtime_snapshot.h').resolve(), visited)
            self.assertFalse(list(source.rglob('*.dll')))

    def test_current_ui_is_reused_with_metadata_provenance(self):
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

    def test_candidate_checks_all_sources_and_keeps_release_payload_frozen(self):
        path = candidate.REPO / 'qualification/providers/current-ui-build-receipt.json'
        receipt = json.loads(path.read_text())
        self.assertTrue(receipt['candidateOnly'])
        self.assertIn('src/ui/ProviderRuntimeClient.cs', receipt['sourceFileSHA256'])
        release = json.loads((candidate.REPO / 'qualification/fg-release/ui-build-receipt.json').read_text())
        manifest = json.loads((candidate.REPO / 'manifest.json').read_text())
        for name, expected in release['payloadSHA256'].items():
            self.assertEqual(manifest['files']['Dungeons/Content/Paks/~mods/MCD2Graphics/' + name], expected)
        original = subprocess.check_output
        def stale_runtime(args, **kwargs):
            if args[-1] == 'HEAD:src/ui/ProviderRuntimeClient.cs':
                return b'stale runtime client'
            return original(args, **kwargs)
        with tempfile.TemporaryDirectory() as temporary, patch.object(candidate.subprocess, 'check_output', side_effect=stale_runtime):
            artifact = Path(temporary) / 'artifact'
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
