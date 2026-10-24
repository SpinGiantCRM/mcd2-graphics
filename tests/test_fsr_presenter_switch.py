from pathlib import Path
import copy
import importlib.util
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('presenter_switch_runner', ROOT/'experiments/providers/run_presenter_switch_probe.py')
runner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)


def evidence():
    rows = []
    def event(stage, count=1):
        rows.extend(dict(stage=stage, result=0) for _ in range(count))
    for stage in ('owned-recording-contract', 'load-verified-runtime', 'create-bridge-swapchain',
                  'reject-empty-target', 'owned-presenter-antilag-ready', 'clear-antilag-before-presentation',
                  'reject-unrelated-submission-queue', 'set-presentation-colour', 'configure-output-contract'):
        event(stage)
    for phase in range(6):
        sdk = phase in (0, 2, 4)
        enabled = phase in (2, 4)
        count = 60 if enabled else 30
        if phase:
            rows.append(dict(stage='reject-unsafe-switch', cases=4))
            rows.append(dict(stage='switch', generation=phase+1, sdk=sdk, identityStable=True, milliseconds=10))
        if enabled:
            event('prepare-game-bridge-inputs', count)
            event('copy-hudless-world', count)
        if sdk:
            event('configure-bridge-present', count)
        rows.append(dict(stage='phase', phase=phase, sdkOwner=sdk, enabled=enabled, real=count,
                         generated=count if enabled else 0, fault=0, errors=0, warnings=0))
    rows.append(dict(stage='concurrent-observer', samples=120, failures=0))
    event('complete')
    return rows


class PresenterSwitchTests(unittest.TestCase):
    def test_complete_handoff_gate(self):
        rows = evidence()
        self.assertTrue(runner.gate(rows, 0, False))
        self.assertFalse(runner.gate(rows, 1, False))
        self.assertFalse(runner.gate(rows, 0, True))
        for stage in {row['stage'] for row in rows}:
            self.assertFalse(runner.gate([row for row in rows if row['stage'] != stage], 0, False), stage)
        for index, row in enumerate(rows):
            if 'result' in row:
                bad = copy.deepcopy(rows)
                bad[index]['result'] = 99
                self.assertFalse(runner.gate(bad, 0, False))

    def test_rejects_counter_identity_owner_or_admission_substitutes(self):
        for stage, fields in {'phase': ('phase', 'sdkOwner', 'enabled', 'real', 'generated', 'fault', 'errors', 'warnings'),
                              'switch': ('generation', 'sdk', 'identityStable', 'milliseconds'),
                              'reject-unsafe-switch': ('cases',),
                              'concurrent-observer': ('samples', 'failures')}.items():
            for field in fields:
                for index, row in enumerate(evidence()):
                    if row['stage'] != stage:
                        continue
                    bad = evidence()
                    bad[index][field] = not row[field] if isinstance(row[field], bool) else -1
                    self.assertFalse(runner.gate(bad, 0, False), (stage, index, field))
        bad = evidence()
        first = next(i for i, row in enumerate(bad) if row['stage'] == 'switch')
        bad[first-1], bad[first] = bad[first], bad[first-1]
        self.assertFalse(runner.gate(bad, 0, False))

    def test_live_implementation_not_replaced_by_experiment(self):
        bridge = (ROOT/'experiments/providers/fsr_fg_game_bridge.cpp').read_text()
        wrapper = (ROOT/'experiments/providers/owned_presenter_switch_probe.hpp').read_text()
        probe = (ROOT/'experiments/providers/presenter_switch_probe.cpp').read_text()
        self.assertNotIn('OwnedPresenterSwitchProbe', bridge)
        for guard in ('!gpuComplete', 'cachedBuffers', '!latencyHandleClosed', '!recordingReset'):
            self.assertIn(guard, wrapper)
        self.assertLess(probe.index('if(!wait()||!reset()||FAILED(cmd->Close()))'), probe.index('drop();if(waitable)'))
        self.assertLess(probe.index('drop();if(waitable)'), probe.index('auto code=chain->rebuild'))
        self.assertIn('if(sdkOwner)', probe)
        self.assertIn('if(present(queue,identity+100,0)||retire(nullptr,0))return E_FAIL;', probe)
        self.assertLess(probe.index('retire(nullptr,0)'), probe.index('if(raw){raw->Release();raw=nullptr;}'))
        self.assertIn('factory->CreateSwapChainForHwnd(queue,window', probe)
        self.assertIn('raw->SetColorSpace1(', probe)
        self.assertNotIn('SetEvent(', probe)
        self.assertNotIn('fence->Signal(', probe)
        self.assertIn('queue->Signal(fence,value)', probe)


if __name__ == '__main__':
    unittest.main()
