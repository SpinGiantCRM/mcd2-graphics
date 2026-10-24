import importlib.util
from pathlib import Path
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('gdk_build',ROOT/'build_gdk_save_bridge.py')
recipe=importlib.util.module_from_spec(spec);spec.loader.exec_module(recipe)

class GdkBuildTests(unittest.TestCase):
 def test_sdk_layout_resolution_refuses_missing_or_ambiguous_versions(self):
  with tempfile.TemporaryDirectory() as d:
   root=Path(d)
   with self.assertRaises(ValueError):recipe.resolve_gdk_include(root)
   modern=root/'native/251005/windows/include';modern.mkdir(parents=True)
   for name in ['XGameSave.h','XGameRuntimeInit.h']:(modern/name).write_text('fixture')
   self.assertEqual(recipe.resolve_gdk_include(root),modern.resolve())
   self.assertEqual(recipe.resolve_gdk_include(modern),modern.resolve())
   old=root/'250400/GRDK/GameKit/Include';old.mkdir(parents=True)
   for name in ['XGameSave.h','XGameRuntimeInit.h']:(old/name).write_text('fixture')
   with self.assertRaises(ValueError):recipe.resolve_gdk_include(root)
   self.assertEqual(recipe.resolve_gdk_include(root/'250400'),old.resolve())
