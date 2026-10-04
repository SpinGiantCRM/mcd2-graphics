"""Portable installer transaction checks using synthetic, isolated dependencies."""
import contextlib
import io
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import install


class InstallerFilesystemTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='mcd2-installer-')
        self.addCleanup(self.temp.cleanup)
        base = Path(self.temp.name)
        self.source = base / 'release'
        self.root = base / 'game'
        self.source.mkdir()
        self.root.mkdir()
        manifest = json.loads((install.HERE / 'manifest.json').read_text())
        manifest['version'] = 'test-windows-candidate'
        lock = json.loads((install.HERE / 'dependencies.lock.json').read_text())
        for name in manifest['files']:
            file = self.source / 'package' / name
            file.parent.mkdir(parents=True, exist_ok=True)
            file.write_bytes(('Synthetic mod payload: ' + name).encode())
            manifest['files'][name] = install.digest(file)
        self.required = {}
        game_name = 'Dungeons/Binaries/Win64/Dungeons-Win64-Shipping.exe'
        lock['game']['exeSHA256'] = self.dependency(game_name)
        for key in ('ReShade', 'RenoDXUEExtended'):
            dep = lock['dependencies'][key]
            dep['sha256'] = self.dependency(dep['file'])
        for name in lock['dependencies']['BlueprintLoader']['files']:
            lock['dependencies']['BlueprintLoader']['files'][name] = self.dependency(name)
        self.runtime = base / 'nvngx_dlss.dll'
        self.runtime.write_bytes(b'Synthetic official-runtime fixture')
        lock['dependencies']['DLSSRuntime']['sha256'] = install.digest(self.runtime)
        (self.source / 'manifest.json').write_text(json.dumps(manifest))
        (self.source / 'dependencies.lock.json').write_text(json.dumps(lock))
        self.owned = list(manifest['files']) + ['Dungeons/Binaries/Win64/MCD2Graphics/ngx-runtime/nvngx_dlss.dll']
        self.marker = self.root / 'Dungeons/Binaries/Win64/MCD2Graphics/install-manifest.json'
        self.protected = self.root / 'Dungeons/Content/Paks/~mods/Unrelated/readme.txt'
        self.protected.parent.mkdir(parents=True)
        self.protected.write_bytes(b'Unrelated user data')
        self.addCleanup(patch.stopall)
        patch.object(install, 'HERE', self.source).start()

    def dependency(self, name):
        file = self.root / name
        file.parent.mkdir(parents=True, exist_ok=True)
        file.write_bytes(('Synthetic dependency: ' + name).encode())
        self.required[name] = install.digest(file)
        return self.required[name]

    def run_install(self):
        with contextlib.redirect_stdout(io.StringIO()):
            install.install(self.root, self.runtime)

    def assert_protected(self):
        self.assertEqual(self.protected.read_bytes(), b'Unrelated user data')
        for name, digest in self.required.items():
            self.assertEqual(install.digest(self.root / name), digest)

    def test_install_duplicate_refusal_uninstall_and_reinstall(self):
        self.run_install()
        marker = json.loads(self.marker.read_text())
        self.assertEqual(marker['version'], 'test-windows-candidate')
        recorded = marker['files']
        self.assertEqual(set(recorded), set(self.owned))
        for name, digest in recorded.items():
            self.assertEqual(install.digest(self.root / name), digest)
        with self.assertRaisesRegex(ValueError, 'Already installed'):
            self.run_install()
        with contextlib.redirect_stdout(io.StringIO()):
            install.uninstall(self.root)
        self.assertFalse(self.marker.exists())
        self.assertTrue(all(not (self.root / name).exists() for name in self.owned))
        self.assert_protected()
        self.run_install()
        self.assert_protected()

    def test_missing_dependency_refused_without_payload_writes(self):
        (self.root / next(iter(self.required))).unlink()
        with self.assertRaisesRegex(ValueError, 'Missing or unsupported'):
            self.run_install()
        self.assertTrue(all(not (self.root / name).exists() for name in self.owned))
        self.assertFalse(self.marker.exists())

    def test_modified_owned_file_retained_without_removing_other_files(self):
        self.run_install()
        addon = self.root / 'Dungeons/Binaries/Win64/mcd2-graphics.addon64'
        addon.write_bytes(addon.read_bytes() + b'Modified')
        with self.assertRaisesRegex(ValueError, 'Modified file retained'):
            install.uninstall(self.root)
        self.assertTrue(all((self.root / name).exists() for name in self.owned))
        self.assertTrue(self.marker.exists())
        self.assert_protected()


if __name__ == '__main__':
    unittest.main()
