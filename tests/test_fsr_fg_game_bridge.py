from pathlib import Path
import copy
import hashlib
import importlib.util
import json
import tempfile
import unittest
from unittest.mock import patch

ROOT=Path(__file__).resolve().parents[1]
def load(name):
    spec=importlib.util.spec_from_file_location(name,ROOT/'experiments/providers'/(name+'.py'))
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module
recipe=load('build_fsr_fg_game_bridge');runner=load('run_fsr_fg_game_bridge_probe')
def evidence():
    rows=[]
    def event(stage,count=1,result=0):rows.extend(dict(stage=stage,result=result) for _ in range(count))
    for stage in ['owned-recording-contract','load-verified-runtime','create-bridge-swapchain','set-PQ-output']:event(stage)
    for stage in ['reject-unrelated-submission-queue','owned-presenter-antilag-ready','clear-antilag-before-presentation']:event(stage)
    for phase in range(3):
        count=60 if phase==1 else 30
        if phase==1:event('prepare-game-bridge-inputs',60);event('copy-hudless-world',60)
        event('configure-bridge-present',count)
        rows.append(dict(stage='phase',phase=phase,enabled=phase==1,real=count,
                         generated=count if phase==1 else 0,fault=0,errors=0,warnings=0))
    event('clear-antilag-after-presentation')
    event('retire-with-replayable-host-recording');event('host-recording-replay-after-SDK-retirement')
    event('idempotent-retirement');event('complete')
    return rows
class BridgeTests(unittest.TestCase):
    def test_full_counter_and_retirement_gate(self):
        rows=evidence();self.assertTrue(runner.gate(rows,0,False))
        self.assertFalse(runner.gate(rows,5,False));self.assertFalse(runner.gate(rows,0,True))
        for stage in {row['stage'] for row in rows}:
            self.assertFalse(runner.gate([row for row in rows if row['stage']!=stage],0,False),stage)
        for index,row in enumerate(rows):
            if 'result' in row:
                bad=copy.deepcopy(rows);bad[index]['result']=99;self.assertFalse(runner.gate(bad,0,False))
        bad=copy.deepcopy(rows);bad[-3],bad[-2]=bad[-2],bad[-3];self.assertFalse(runner.gate(bad,0,False))
    def test_no_counter_or_severity_substitutes(self):
        for field in ['phase','enabled','real','generated','fault','errors','warnings']:
            bad=evidence();row=next(r for r in bad if r['stage']=='phase' and r['phase']==1)
            row[field]=False if field=='enabled' else -1;self.assertFalse(runner.gate(bad,0,False),field)
        bad=evidence();next(r for r in bad if r['stage']=='phase' and r['phase']==0)['generated']=1
        self.assertFalse(runner.gate(bad,0,False))
    def test_pins_before_compile_and_complete_source_receipt(self):
        with tempfile.TemporaryDirectory() as temporary:
            root=Path(temporary)
            for name in recipe.HEADERS:
                path=root/name;path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(b'fixture')
            with patch.object(recipe.subprocess,'run') as invoke:
                with self.assertRaisesRegex(ValueError,'Pinned FSR'):recipe.build(root,root,root/'output')
                invoke.assert_not_called()
            pins={n:hashlib.sha256(b'fixture').hexdigest() for n in recipe.HEADERS};calls=[]
            def fake(command,**kwargs):
                calls.append(command)
                for item in command:
                    if item.startswith('/out:'):Path(item[5:]).write_bytes(b'fixture')
            with patch.object(recipe,'HEADERS',pins),patch.object(recipe.subprocess,'run',side_effect=fake):
                recipe.build(root,root,root/'output','compiler','linker')
            receipt=json.loads((root/'output/build-receipt.json').read_text())
            for name in ['fsr_fg_game_bridge.cpp','fsr_fg_game_bridge.h','fsr_fg_game_bridge_probe.cpp','build_fsr_fg_probe.py',
                         '../fg-streamline/fg_camera_contract.h','../../src/native/reset_epoch_contract.hpp']:
                self.assertEqual(receipt['sourceSHA256'][name],recipe.digest(ROOT/'experiments/providers'/name))
            self.assertFalse(receipt['runtimeBundled']);self.assertFalse(receipt['gameIntegrationQualified'])
            for command in calls[::2]:self.assertIn('/MT',command);self.assertIn('/clang:-Werror=frame-larger-than',command)
            self.assertEqual({p.name for p in (root/'output').glob('*.dll')},{'mcd2-fsr-fg-game-bridge.dll'})
    def test_sdk_recordings_are_private_and_retirement_is_independent(self):
        source=(ROOT/'experiments/providers/fsr_fg_game_bridge.cpp').read_text()
        guides=source.split('extern "C" int mcd2_afg_guides_v2',1)[1].split('extern "C" int mcd2_afg_present_v2',1)[0]
        for forbidden in ['s.dispatch(', 'CopyResource(', '->Reset(', 'ExecuteCommandLists(']:
            self.assertNotIn(forbidden,guides)
        self.assertIn('s.inputs.depth->AddRef()',guides)
        self.assertIn('s.inputs.colour->AddRef()',guides)
        present=source.split('extern "C" int mcd2_afg_present_v2',1)[1].split('extern "C" int mcd2_afg_state_v1',1)[0]
        self.assertIn('wanted&&!matched',present)
        self.assertIn('d.commandList=list',present)
        self.assertIn('s.queue->ExecuteCommandLists(1,batch)',present)
        self.assertIn('s.unconfirmedSubmission=true',present)
        self.assertNotIn('WaitForSingleObject',present)
        work=source.split('int beginWork(',1)[1].split('bool absolute(',1)[0]
        self.assertLess(work.index('completed<w.fence'),work.index('w.allocator->Reset()'))
        self.assertLess(work.index('w.command->Reset('),work.index('clearBorrowed(w)'))
        retirement=source.split('extern "C" int mcd2_afg_retire_v1',1)[1]
        self.assertLess(retirement.index('WaitForSingleObject'),retirement.index('release(work.command)'))
        self.assertLess(retirement.index('release(work.command)'),retirement.index('s.destroy(&s.fg'))
        self.assertNotIn('->Reset(',retirement)
        bootstrap=(ROOT/'experiments/fg-streamline/bootstrap_dxgi.cpp').read_text()
        for name in ['mcd2_afg_recording_contract_v2','mcd2_afg_guides_v2','mcd2_afg_world_v2','mcd2_afg_present_v2']:
            self.assertIn(name,bootstrap)

    def test_run_validation_does_not_accept_game_or_tamper(self):
        with tempfile.TemporaryDirectory() as temporary:
            root=Path(temporary);prefix=root/'isolated';(prefix/'pfx').mkdir(parents=True)
            for name in ['mcd2-fsr-fg-game-bridge.dll','fsr-fg-game-bridge-probe.exe',*runner.RUNTIME]:(root/name).write_bytes(b'fixture')
            receipt=dict(binarySHA256=runner.digest(root/'mcd2-fsr-fg-game-bridge.dll'),
                         probeSHA256=runner.digest(root/'fsr-fg-game-bridge-probe.exe'),runtimeBundled=False,gameIntegrationQualified=False)
            (root/'build-receipt.json').write_text(json.dumps(receipt))
            with self.assertRaisesRegex(ValueError,'isolated prefix'):runner.validate(root,root/'steamapps/compatdata/1912410')
            with self.assertRaisesRegex(ValueError,'Pinned external'):runner.validate(root,prefix)
            pins={n:runner.digest(root/n) for n in runner.RUNTIME}
            with patch.object(runner,'RUNTIME',pins):
                runner.validate(root,prefix)
                (root/'dxgi.dll').write_bytes(b'bad')
                with self.assertRaisesRegex(ValueError,'Unexpected proxy'):runner.validate(root,prefix)
                (root/'dxgi.dll').unlink();(root/'mcd2-fsr-fg-game-bridge.dll').write_bytes(b'changed')
                with self.assertRaisesRegex(ValueError,'Build receipt mismatch'):runner.validate(root,prefix)
if __name__=='__main__':unittest.main()
