from pathlib import Path
import copy
import hashlib
import importlib.util
import json
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
def load(name):
    spec = importlib.util.spec_from_file_location(name, ROOT / 'experiments/providers' / (name + '.py'))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module
recipe, runner = load('build_fsr_fg_probe'), load('run_fsr_fg_probe')

def evidence(presentation=False):
    rows = [{'stage':'provider','id':17726168133342859270,'name':'3.1.6'}]
    def event(name, count=1):
        rows.extend({'stage':name,'result':0} for _ in range(count))
    for trial in range(1 if presentation else 2):
        event('create-context'); event('confirm-active-provider')
        rows.append({'stage':'active-provider','id':17726168133342859270})
        if presentation:
            event('create-owned-swapchain');event('confirm-swapchain-provider')
            rows.append({'stage':'swapchain-provider','id':17752306900579389447,'name':'3.1.7'})
            for phase in range(3):
                real = 60 if phase==1 else 30
                event('query-before-present-count');event('configure-frame',real)
                if phase==1:event('prepare-owned-inputs',real)
                event('present-real-frame',real);event('wait-phase-presents');event('query-present-count')
                rows.append({'stage':'phase','phase':phase,'enabled':phase==1,
                             'realSubmissions':real,'realCallbacks':real,'generatedCallbacks':real if phase==1 else 0,
                             'DXGIPresentDelta':real*2 if phase==1 else real,'callbackErrors':0})
            event('wait-for-presents')
        else:
            rows.append({'stage':'trial','trial':trial,'HDR':trial==1,'swapchain':False})
            for frame in range(16):
                event('configure-frame');event('prepare-owned-inputs');event('generate-owned-output')
                rows.append({'stage':'readback','frame':frame,'HDR':trial==1,'valid':True,
                             'maximum':8 if trial==1 else 1})
        for name in ['invalidate-owned-recording','post-invalidation-fence','disable-before-destroy','destroy-context-after-fence']:
            event(name)
        if presentation:event('destroy-swapchain-after-presents')
        event('release-owned-image',7)
    rows.append({'stage':'complete','SDKerrors':0,'SDKwarnings':0,'dispatches':32,
                 'realSubmissions':120,'gameFilesChanged':False,'presentationQualified':presentation})
    return rows

class FsrFgTests(unittest.TestCase):
    def test_owned_and_presentation_full_gates(self):
        for presentation in [False,True]:
            rows=evidence(presentation)
            self.assertTrue(runner.gate(rows,0,False,presentation=presentation))
            self.assertFalse(runner.gate(rows,5,False,presentation=presentation))
            self.assertFalse(runner.gate(rows,0,True,presentation=presentation))
            for index,row in enumerate(rows):
                if 'result' in row and row['stage']!='release-owned-image':
                    bad=copy.deepcopy(rows);bad[index]['result']=3
                    self.assertFalse(runner.gate(bad,0,False,presentation=presentation),row['stage'])
            for stage in set(r['stage'] for r in rows):
                self.assertFalse(runner.gate([r for r in rows if r['stage']!=stage],0,False,presentation=presentation),stage)
            for field in ['SDKerrors','SDKwarnings']:
                bad=copy.deepcopy(rows);bad[-1][field]=1
                self.assertFalse(runner.gate(bad,0,False,presentation=presentation))

    def test_wrong_provider_and_retirement_order(self):
        for presentation in [False,True]:
            rows=evidence(presentation)
            for index,row in enumerate(rows):
                if row['stage'] in ['provider','active-provider','swapchain-provider']:
                    bad=copy.deepcopy(rows);bad[index]['id']=0
                    self.assertFalse(runner.gate(bad,0,False,presentation=presentation))
            bad=copy.deepcopy(rows)
            a=next(i for i,r in enumerate(bad) if r['stage']=='post-invalidation-fence')
            b=next(i for i,r in enumerate(bad) if r['stage']=='destroy-context-after-fence')
            bad[a],bad[b]=bad[b],bad[a]
            self.assertFalse(runner.gate(bad,0,False,presentation=presentation))

    def test_real_generated_and_dxgi_counts_are_independent(self):
        for key in ['realSubmissions','realCallbacks','generatedCallbacks','DXGIPresentDelta','callbackErrors','phase','enabled']:
            bad=evidence(True);row=next(r for r in bad if r['stage']=='phase' and r['phase']==1)
            row[key]=False if key=='enabled' else -1
            self.assertFalse(runner.gate(bad,0,False,presentation=True),key)
        for phase in [0,2]:
            bad=evidence(True);row=next(r for r in bad if r['stage']=='phase' and r['phase']==phase)
            row['generatedCallbacks']=1;row['DXGIPresentDelta']=31
            self.assertFalse(runner.gate(bad,0,False,presentation=True))

    def test_hdr_readback_and_frame_order(self):
        for key,value in [('maximum',1),('frame',99),('HDR',False),('valid',False)]:
            bad=evidence();row=next(r for r in bad if r['stage']=='readback' and r['HDR'])
            row[key]=value
            self.assertFalse(runner.gate(bad,0,False),key)

    def test_framework_requires_actual_wrapped_device(self):
        rows=evidence(True)
        declaration={'stage':'framework','requested':True,'exports':True,'deviceWrapped':True}
        self.assertFalse(runner.gate(rows,0,False,True,True))
        self.assertTrue(runner.gate([declaration,*rows],0,False,True,True))
        for key in ['requested','exports','deviceWrapped']:
            bad=dict(declaration);bad[key]=False
            self.assertFalse(runner.gate([bad,*rows],0,False,True,True))
        self.assertFalse(runner.gate([declaration,declaration,*rows],0,False,True,True))

    def test_pin_before_compile_and_no_bundled_runtime(self):
        with tempfile.TemporaryDirectory() as temporary:
            root=Path(temporary)
            for name in recipe.HEADERS:
                path=root/name;path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(b'fixture')
            with patch.object(recipe.subprocess,'run') as invoke:
                with self.assertRaisesRegex(ValueError,'Pinned FSR'):recipe.build(root,root,root/'out')
                invoke.assert_not_called()
            pins={name:hashlib.sha256(b'fixture').hexdigest() for name in recipe.HEADERS}
            calls=[]
            def fake_run(command,**kwargs):
                calls.append(command)
                for part in command:
                    if part.startswith('/out:'):Path(part[5:]).write_bytes(b'fixture')
            with patch.object(recipe,'HEADERS',pins),patch.object(recipe.subprocess,'run',side_effect=fake_run):
                for presentation in [False,True]:
                    out=root/('present' if presentation else 'owned')
                    recipe.build(root,root,out,'compiler','linker',presentation)
                    receipt=json.loads((out/'build-receipt.json').read_text())
                    self.assertEqual(receipt['probeKind'],'presentation' if presentation else 'owned-inputs')
                    self.assertFalse(receipt['runtimeBundled']);self.assertFalse(receipt['gameIntegrationQualified'])
                    self.assertFalse(any(p.suffix=='.dll' for p in out.iterdir()))
            for command in calls[::2]:
                self.assertIn('/MT',command);self.assertIn('/clang:-Werror=frame-larger-than',command)

    def test_runner_refuses_tamper_kind_game_prefix_and_proxy(self):
        with tempfile.TemporaryDirectory() as temporary:
            root=Path(temporary);prefix=root/'isolated';(prefix/'pfx').mkdir(parents=True)
            (root/'fsr-fg-owned-inputs.exe').write_bytes(b'fixture')
            receipt={'binarySHA256':hashlib.sha256(b'fixture').hexdigest(),'runtimeBundled':False,
                     'gameIntegrationQualified':False,'probeKind':'owned-inputs'}
            def save(): (root/'build-receipt.json').write_text(json.dumps(receipt))
            save()
            for forbidden in [root/'steamapps/compatdata/1912410',root/'1912410']:
                with self.assertRaisesRegex(ValueError,'isolated prefix'):runner.validate(root,forbidden)
            receipt['probeKind']='presentation';save()
            with self.assertRaisesRegex(ValueError,'Probe kind'):runner.validate(root,prefix)
            receipt['probeKind']='owned-inputs';receipt['binarySHA256']='wrong';save()
            with self.assertRaisesRegex(ValueError,'build receipt'):runner.validate(root,prefix)
            receipt['binarySHA256']=hashlib.sha256(b'fixture').hexdigest();save()
            dll=root/next(iter(runner.RUNTIME));dll.write_bytes(b'fixture')
            with self.assertRaisesRegex(ValueError,'pinned runtime'):runner.validate(root,prefix)
            with patch.object(runner,'RUNTIME',{dll.name:hashlib.sha256(b'fixture').hexdigest()}):
                runner.validate(root,prefix)
                for name in ['dxgi.dll','d3d12.dll','d3d12core.dll','amdxc64.dll','sl.interposer.dll']:
                    (root/name).write_bytes(b'bad')
                    with self.assertRaisesRegex(ValueError,'Unexpected proxy'):runner.validate(root,prefix)
                    if name=='dxgi.dll':
                        with self.assertRaisesRegex(ValueError,'Unqualified ReShade'):runner.validate(root,prefix,True)
                    (root/name).unlink()

if __name__=='__main__':unittest.main()
