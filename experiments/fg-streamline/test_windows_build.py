"""Regress the mixed directory names in a real Windows Kits installation."""
import importlib.util
from pathlib import Path
import tempfile
import unittest
import subprocess
import json
import re
import shutil
from unittest.mock import patch

spec = importlib.util.spec_from_file_location('windows_candidate', Path(__file__).with_name('build_windows_candidate.py'))
candidate = importlib.util.module_from_spec(spec)
spec.loader.exec_module(candidate)
source_spec = importlib.util.spec_from_file_location('sr_source_tree', Path(__file__).with_name('sr_source_tree.py'))
source_tree = importlib.util.module_from_spec(source_spec)
source_spec.loader.exec_module(source_tree)


class WindowsBuildChecks(unittest.TestCase):
    def sr_build_fixture(self, root):
        repo = root / 'repo'
        sources = repo / 'src/shaders'
        sources.mkdir(parents=True)
        (sources / 'live_dense.hlsl').write_bytes(b'owned shader')
        (sources / 'fsr_dense.hlsl').write_bytes(b'#include "live_dense.hlsl"')
        bridge, dxc = root / 'bridge.dll', root / 'dxc.exe'
        bridge.write_bytes(b'Windows-built bridge')
        dxc.write_bytes(b'pinned compiler')
        return repo, root / 'sr', bridge, dxc

    def run_sr_fixture(self, repo, output, bridge, dxc):
        candidate.build_continuous_sr(repo, output, 'reshade/include', 'ngx/include',
                                      'clang++.exe', 'mingw.exe', dxc, bridge)

    def test_continuous_sr_pins_windows_bridge_and_packages_both_converters(self):
        with tempfile.TemporaryDirectory() as temporary:
            repo, output, bridge, dxc = self.sr_build_fixture(Path(temporary))
            commands = []
            def build(*args):
                commands.append(args)
                output.mkdir(exist_ok=True)
                if '--fsr-bridge-sha256' in args:
                    pin = args[args.index('--fsr-bridge-sha256') + 1]
                    (output / 'build-receipt.json').write_text(json.dumps({'fsrBridgeSHA256': pin}))
                else:
                    Path(args[-1]).write_bytes(b'compiled ' + Path(args[-3]).name.encode())
            with patch.object(candidate, 'run', side_effect=build):
                self.run_sr_fixture(repo, output, bridge, dxc)
            self.assertEqual(len(commands), 3)
            self.assertEqual(commands[0][-2:], ('--fsr-bridge-sha256', candidate.sha(bridge)))
            for command in commands[1:]:
                self.assertEqual(command[:5], (dxc, '-T', 'cs_6_0', '-E', 'main'))
            receipt = json.loads((output / 'conversion-shaders-receipt.json').read_text())
            self.assertEqual(receipt['fsrBridgeSHA256'], candidate.sha(bridge))
            self.assertEqual(receipt['compilerSHA256'], candidate.sha(dxc))
            self.assertEqual(set(receipt['sourceSHA256']), {'src/shaders/live_dense.hlsl', 'src/shaders/fsr_dense.hlsl'})
            self.assertEqual(set(receipt['binarySHA256']), {'live_dense.cso', 'fsr_dense.cso'})
            for name, expected in receipt['binarySHA256'].items():
                self.assertEqual(candidate.sha(output / name), expected)
            self.assertFalse(receipt['runtimeQualified'])

    def test_continuous_sr_refuses_missing_or_empty_bridge_before_build(self):
        with tempfile.TemporaryDirectory() as temporary, patch.object(candidate, 'run') as build:
            repo, output, bridge, dxc = self.sr_build_fixture(Path(temporary))
            bridge.unlink()
            for empty in (False, True):
                if empty:
                    bridge.write_bytes(b'')
                with self.assertRaisesRegex(RuntimeError, 'Missing source-built FSR bridge'):
                    self.run_sr_fixture(repo, output, bridge, dxc)
            build.assert_not_called()
            self.assertFalse(output.exists())

    def test_continuous_sr_refuses_wrong_receipt_pin(self):
        with tempfile.TemporaryDirectory() as temporary:
            repo, output, bridge, dxc = self.sr_build_fixture(Path(temporary))
            output.mkdir()
            (output / 'build-receipt.json').write_text(json.dumps({'fsrBridgeSHA256': ''}))
            with patch.object(candidate, 'run') as build:
                with self.assertRaisesRegex(RuntimeError, 'does not match'):
                    self.run_sr_fixture(repo, output, bridge, dxc)
            self.assertEqual(build.call_count, 1)
            self.assertFalse((output / 'conversion-shaders-receipt.json').exists())

    def test_continuous_sr_refuses_absent_or_empty_compiler_output(self):
        for empty in (False, True):
            with self.subTest(empty=empty), tempfile.TemporaryDirectory() as temporary:
                repo, output, bridge, dxc = self.sr_build_fixture(Path(temporary))
                output.mkdir()
                (output / 'build-receipt.json').write_text(json.dumps({'fsrBridgeSHA256': candidate.sha(bridge)}))
                def build(*args):
                    if '-Fo' in args and empty:
                        Path(args[-1]).write_bytes(b'')
                with patch.object(candidate, 'run', side_effect=build):
                    with self.assertRaisesRegex(RuntimeError, 'Missing compiled SR conversion shader'):
                        self.run_sr_fixture(repo, output, bridge, dxc)
                self.assertFalse((output / 'conversion-shaders-receipt.json').exists())

    def test_copied_fsr_abi_is_guarded_across_independent_include_paths(self):
        # The sustained probe and production runtime include separate copies.
        # pragma once alone does not deduplicate those files with Clang.
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary).resolve()
            content = (candidate.REPO / 'experiments/providers/fsr_game_bridge.h').read_bytes()
            for name in ('first.h', 'second.h'):
                (root / name).write_bytes(content)
            (root / 'test.cpp').write_text('#include "first.h"\n#include "second.h"\nint main(){}\n')
            compiler = shutil.which('clang++') or shutil.which('g++')
            self.assertIsNotNone(compiler, 'C++ compiler required by both CI platforms')
            subprocess.run([compiler, '-std=c++20', '-fsyntax-only', str(root / 'test.cpp')], check=True)

    def test_isolated_sr_source_contains_its_relative_include_closure(self):
        with tempfile.TemporaryDirectory() as temporary:
            # Windows runners may expose the temp directory through an 8.3
            # alias. Normalize the existing parent before constructing paths.
            source = Path(temporary).resolve() / 'source'
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
