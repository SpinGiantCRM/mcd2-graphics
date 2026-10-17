from pathlib import Path
import copy
import importlib.util
import unittest
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('presenter_resize',ROOT/'experiments/providers/run_presenter_resize_probe.py')
runner=importlib.util.module_from_spec(spec);spec.loader.exec_module(runner)
def evidence(on_exit=False):
    rows=[]
    for stage,count in {'owned-recording-contract':1,'load-verified-runtime':1,'create-bridge-swapchain':1,
        'owned-presenter-antilag-ready':1,'clear-antilag-before-presentation':1,'reject-unrelated-submission-queue':1,
        'set-presentation-colour':1,'configure-output-contract':1,'prepare-game-bridge-inputs':180 if on_exit else 120,
        'copy-hudless-world':180 if on_exit else 120,'configure-bridge-present':270 if on_exit else 210,'retire-request-state':1}.items():
        rows.extend(dict(stage=stage,result=0) for _ in range(count))
    for phase in range(6 if on_exit else 5):
        enabled=phase in (1,3,5);count=60 if enabled else 30
        if phase>=2:rows.append(dict(stage='presenter-handoff',route=2 if phase in (3,5) else 1,identityStable=True,privateDataPreserved=True))
        rows.append(dict(stage='phase',phase=phase,enabled=enabled,real=count,generated=count if enabled else 0,fault=0,errors=0,warnings=0))
    rows.append(dict(stage='complete',result=0));return rows
class PresenterResizeTests(unittest.TestCase):
    def test_gate_requires_complete_real_handoffs(self):
        rows=evidence();self.assertTrue(runner.gate(rows,0,False))
        self.assertFalse(runner.gate(rows,1,False));self.assertFalse(runner.gate(rows,0,True))
        for stage in {r['stage'] for r in rows}:self.assertFalse(runner.gate([r for r in rows if r['stage']!=stage],0,False),stage)
        for index,row in enumerate(rows):
            for field in ('result','phase','real','generated','fault','errors','warnings','route'):
                if field in row:
                    bad=copy.deepcopy(rows);bad[index][field]=-1;self.assertFalse(runner.gate(bad,0,False),(index,field))
            for field in ('enabled','identityStable','privateDataPreserved'):
                if field in row:
                    bad=copy.deepcopy(rows);bad[index][field]=not row[field];self.assertFalse(runner.gate(bad,0,False),(index,field))
    def test_active_sdk_retirement_is_a_separate_gate(self):
        self.assertTrue(runner.gate(evidence(True),0,False,True))
        self.assertFalse(runner.gate(evidence(),0,False,True))
        self.assertFalse(runner.gate(evidence(True),0,False))
    def test_switch_is_engine_resize_only_and_default_off(self):
        bridge=(ROOT/'experiments/providers/fsr_fg_game_bridge.cpp').read_text()
        resize=bridge[bridge.index('HRESULT resizeTarget(IDXGISwapChain4*& raw'):]
        for guard in ('!state.privateComplete','state.handleIssued','DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT','fullscreen','live.intent!=r.intent','retireUnlocked(nullptr,0)'):
            self.assertIn(guard,bridge)
        self.assertLess(resize.index('retireUnlocked(nullptr,0)'),resize.index('raw->Release();raw=nullptr;'))
        self.assertIn('anchor.allow?resizeTarget:nullptr',bridge)
        self.assertIn('L"AmdPresenterSwitch",0',bridge)
        self.assertIn('previous-mode-recovered',bridge)
        self.assertIn('raw->SetPrivateDataInterface',resize)
    def test_ui_waits_for_committed_current_session_and_restores_display(self):
        source=(ROOT/'src/ui/ModActor.cs').read_text()
        for guard in ('ProviderMenu.Pending!=null','ProviderMenu.Dirty!=0','FGRuntime.SessionId!=FGSaved.SessionId','FGRuntime.PresenterSettingsRevision!=revision','PresenterResetTicks>=6','PresenterResetSession'):
            self.assertIn(guard,source)
        self.assertIn('settings.ApplyResolutionSettings(false)',source)
        self.assertIn('settings.SetScreenResolution(PresenterOriginalResolution)',source)
        self.assertIn('Timer.Stop(this,"RestorePresenterFocus")',source)
        self.assertIn('PresenterRestoreFGFocus=GraphicsPageOpen() && displayHelp==5',source)
        focus=source[source.index('public void RestorePresenterFocus()'):source.index('public void PollPresenterSwitch()')]
        self.assertIn('!PresenterRestoreFGFocus',focus)
        self.assertNotIn('displayHelp!=5',focus)
        self.assertNotIn('PresenterDebugSave',source)
        self.assertNotIn('PresenterDiagnostic',source)
if __name__=='__main__':unittest.main()
