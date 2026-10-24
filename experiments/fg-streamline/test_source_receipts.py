import ast
import types
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
            (repository / 'src/providers/runtime_diagnostics.h').write_bytes(b'ABI')
            (experiment / 'bootstrap.cpp').write_bytes(b'bootstrap')
            hashes = source_hashes(repository, experiment,
                ['src\\providers\\test.hpp', 'bootstrap.cpp'])
            self.assertEqual(hashes, {
                'src/providers/test.hpp': hashlib.sha256(b'header').hexdigest(),
                'bootstrap.cpp': hashlib.sha256(b'bootstrap').hexdigest()})
            self.assertEqual(provider_sources(repository), ['src/providers/runtime_diagnostics.h', 'src/providers/test.hpp'])

    def test_each_settings_consumer_records_transitive_parser_bytes(self):
        experiment=Path(__file__).resolve().parent
        repository=experiment.parents[1]
        tree=ast.parse((experiment/'build_probe.py').read_text())
        start=next(i for i,node in enumerate(tree.body) if isinstance(node,ast.Assign)
                   and any(isinstance(t,ast.Name) and t.id=='sources' for t in node.targets))
        statements=[]
        for node in tree.body[start:]:
            if isinstance(node,ast.Expr):break
            statements.append(node)
        recipe=compile(ast.Module(body=statements,type_ignores=[]),'receipt-source-selection','exec')
        for bootstrap,guide in ((True,False),(False,True),(True,True)):
            with self.subTest(bootstrap=bootstrap,guide=guide):
                values={'a':types.SimpleNamespace(probe='owned',reshade_headers=True,
                    early_bootstrap=bootstrap,guide_recon=guide,guide_mingw_cxx=False),
                    'host':'owned_inputs_probe.cpp','repo':repository,'provider_sources':provider_sources}
                exec(recipe,values)
                hashes=source_hashes(repository,experiment,values['sources'])
                for name in ('src/latency/display_protocol.hpp','src/native/settings_bridge.hpp'):
                    self.assertIn(name,hashes)
                    self.assertEqual(hashes[name],hashlib.sha256((repository/name).read_bytes()).hexdigest())


if __name__ == '__main__':
    unittest.main()
