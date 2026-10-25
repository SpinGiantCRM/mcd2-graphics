"""Evaluate actual clean-build command construction for both guide compilers."""
import ast
from pathlib import Path
import sys, tempfile, types, unittest
from unittest.mock import patch

ROOT=Path(__file__).resolve().parents[2]

class ProbeCommands(unittest.TestCase):
    def test_every_guide_route_builds_the_scene_probe_named_in_receipt(self):
        script=Path(__file__).with_name('build_probe.py')
        tree=ast.parse(script.read_text());nodes=[];collect=False
        for node in tree.body:
            if isinstance(node,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='commands' for t in node.targets):collect=True
            if collect:
                if isinstance(node,ast.With):break
                nodes.append(node)
        for mingw in (False,True):
            with self.subTest(mingw=mingw),tempfile.TemporaryDirectory() as tmp:
                out=Path(tmp)
                args=types.SimpleNamespace(guide_recon=True,guide_mingw_cxx=out/'clang++' if mingw else None,
                    dxc=out/'dxc',sdk=out,sysroot=out,early_bootstrap=True,probe='owned',reshade_headers=out)
                namespace=dict(a=args,out=out,src=script.parent,repo=ROOT,host='owned_inputs_probe.cpp',sys=sys,Path=Path)
                sys.path.insert(0,str(ROOT))
                try:
                    with patch('build_toolchain.native_frame_guard',return_value=['-Wframe-larger-than=16384']):
                        exec(compile(ast.Module(body=nodes,type_ignores=[]),str(script),'exec'),namespace)
                finally:sys.path.remove(str(ROOT))
                commands=namespace['commands']
                self.assertTrue(any(str(script.parent/'fg_scene_copy_probe.cpp') in c for c in commands))
                self.assertTrue(any('/out:'+str(out/'fg-scene-copy-probe.exe') in c for c in commands))

if __name__=='__main__':unittest.main()
