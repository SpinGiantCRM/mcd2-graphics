import hashlib
from pathlib import Path, PureWindowsPath
import tempfile
import unittest
from unittest.mock import Mock
from source_receipts import provider_sources, source_hashes


class SourceReceiptTests(unittest.TestCase):
    def test_windows_provider_keys_are_posix(self):
        root = PureWindowsPath('D:/checkout')
        repository = Mock()
        repository.__truediv__ = Mock(return_value=Mock(glob=Mock(return_value=[
            root / 'src/providers/graphics_intent.hpp'])))
        # The returned path receives the actual root in relative_to.
        class Repository:
            def __truediv__(self, _):
                return repository.__truediv__()
            def __fspath__(self):
                return str(root)
        self.assertEqual(provider_sources(Repository()), ['src/providers/graphics_intent.hpp'])

    def test_repository_and_experiment_sources_use_their_own_roots(self):
        with tempfile.TemporaryDirectory() as tmp:
            repository = Path(tmp)
            experiment = repository / 'experiments/fg-streamline'
            experiment.mkdir(parents=True)
            (repository / 'src/providers').mkdir(parents=True)
            (repository / 'src/providers/test.hpp').write_bytes(b'header')
            (experiment / 'bootstrap.cpp').write_bytes(b'bootstrap')
            hashes = source_hashes(repository, experiment,
                ['src\\providers\\test.hpp', 'bootstrap.cpp'])
            self.assertEqual(hashes, {
                'src/providers/test.hpp': hashlib.sha256(b'header').hexdigest(),
                'bootstrap.cpp': hashlib.sha256(b'bootstrap').hexdigest()})
            self.assertEqual(provider_sources(repository), ['src/providers/test.hpp'])


if __name__ == '__main__':
    unittest.main()
