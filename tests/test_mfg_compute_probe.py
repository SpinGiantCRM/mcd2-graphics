"""Behavioral contracts for the isolated probe runner, never a GPU test."""
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('mfg_runner',ROOT/'experiments/providers/run_mfg_compute_probe.py')
runner=importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)


def samples(multiplier=4):
    rows=[{'stage':'adapter','vendor':0x10de,'device':1,'software':False},
          {'stage':'capability','generatedMax':5},
          {'stage':'init','result':1},{'stage':'create','result':1},
          {'stage':'populate','result':1},{'stage':'scratch','result':1}]
    rows.extend({'stage':'evaluate','result':1} for _ in range(12*(multiplier-1)))
    for frame in range(2,12):
        for slot in range(1,multiplier):
            rows.append({'stage':'image','epoch':1,'previous':frame,'current':frame+1,
                         'slot':slot,'denominator':multiplier,'result':1,'finite':True,
                         'nonzero':True,'hash':f'{frame*10+slot:016x}',
                         'bestOffset':round(8*slot/multiplier),'meanAbsoluteError':.01})
    rows += [{'stage':'release','result':1},{'stage':'shutdown','result':1},
             {'stage':'complete','images':10*(multiplier-1),'swapchain':False,
              'gameIntegrationQualified':False,'moduleRetainedUntilExit':True}]
    return rows


class MfgProbeTests(unittest.TestCase):
    def test_changed_sdk_headers_refused_before_compiler_or_output(self):
        spec=importlib.util.spec_from_file_location('mfg_build',ROOT/'experiments/providers/build_mfg_compute_probe.py')
        recipe=importlib.util.module_from_spec(spec); spec.loader.exec_module(recipe)
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp); sdk=root/'sdk'; sdk.mkdir()
            for name in recipe.HEADERS: (sdk/name).write_bytes(b'changed declaration')
            with self.assertRaises(ValueError): recipe.build(sdk,root/'output',compiler='must-not-launch')
            self.assertFalse((root/'output').exists())

    def test_accepts_only_complete_ordered_distinct_images(self):
        for n in range(2,7): self.assertTrue(runner.image_gate(samples(n),0,False,n))
        good=samples()
        self.assertFalse(runner.image_gate(good,1,False,4))
        self.assertFalse(runner.image_gate(good,0,True,4))
        for key,value in [('finite',False),('nonzero',False),('epoch',2),('previous',1),
                          ('current',2),('slot',2),('denominator',3),('result',0),('hash','x'),
                          ('bestOffset',0),('meanAbsoluteError',.25)]:
            rows=copy.deepcopy(good); next(r for r in rows if r['stage']=='image')[key]=value
            with self.subTest(key=key): self.assertFalse(runner.image_gate(rows,0,False,4))
        rows=copy.deepcopy(good); images=[r for r in rows if r['stage']=='image']; images[1]['hash']=images[0]['hash']
        self.assertFalse(runner.image_gate(rows,0,False,4))
        self.assertFalse(runner.image_gate(good[:-2],0,False,4))
        rows=copy.deepcopy(good); rows[0]['software']=True
        self.assertFalse(runner.image_gate(rows,0,False,4))
        rows=copy.deepcopy(good); rows[1]['generatedMax']=1
        self.assertFalse(runner.image_gate(rows,0,False,4))
        rows=copy.deepcopy(good); rows[-1]['images']=1
        self.assertFalse(runner.image_gate(rows,0,False,4))
        rows=copy.deepcopy(good); rows[-2]['result']=0
        self.assertFalse(runner.image_gate(rows,0,False,4))
        self.assertFalse(runner.image_gate(good+good,0,False,4))

    def test_build_and_external_module_identity_fail_closed(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp); build=root/'build'; build.mkdir()
            binary=build/'mfg-compute-probe.exe'; binary.write_bytes(b'own compiled probe')
            runtime=root/'download.dll'; runtime.write_bytes(b'external')
            receipt={'probeKind':'mfg-owned-inputs','runtimeBundled':False,'gameIntegrationQualified':False,
                     'binarySHA256':hashlib.sha256(binary.read_bytes()).hexdigest()}
            (build/'build-receipt.json').write_text(json.dumps(receipt))
            self.assertEqual(runner.validate_build(build)['binarySHA256'],receipt['binarySHA256'])
            binary.write_bytes(b'changed')
            with self.assertRaises(ValueError): runner.validate_build(build)
            binary.write_bytes(b'own compiled probe')
            # Runtime is never accepted merely because it has the desired name.
            with self.assertRaises(ValueError): runner.validate(build,runtime)
            pinned=root/'dlss-enabler-headless.dll'; pinned.write_bytes(b'also external')
            with self.assertRaises(ValueError): runner.validate(build,pinned)
            receipt['runtimeBundled']=True
            (build/'build-receipt.json').write_text(json.dumps(receipt))
            with self.assertRaises(ValueError): runner.validate(build,pinned)

    def test_isolation_follows_links_and_refuses_game_prefixes(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp); isolated=root/'isolated'; (isolated/'pfx').mkdir(parents=True)
            self.assertEqual(runner.validate_prefix(isolated),isolated.resolve())
            game=root/'SteamApps/compatdata/1912410'; (game/'pfx').mkdir(parents=True)
            with self.assertRaises(ValueError): runner.validate_prefix(game)
            link=root/'alias'; link.symlink_to(game,target_is_directory=True)
            with self.assertRaises(ValueError): runner.validate_prefix(link)
            with self.assertRaises(ValueError): runner.validate_prefix(root/'missing')


if __name__=='__main__': unittest.main()
