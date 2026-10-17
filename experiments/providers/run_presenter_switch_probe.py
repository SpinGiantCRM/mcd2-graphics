"""Exercise destructive presenter handoffs in an isolated owned host, not the game."""
from pathlib import Path
import argparse
import importlib.util
import json
import os
import signal
import subprocess
import time

_spec = importlib.util.spec_from_file_location('_bridge_probe', Path(__file__).with_name('run_fsr_fg_game_bridge_probe.py'))
_base = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_base)


def validate(output, prefix):
    _base.validate(output, prefix)
    receipt = json.loads((output/'build-receipt.json').read_text()).get('presenterSwitchProbe', {})
    if (_base.digest(output/'presenter-switch-probe.exe') != receipt.get('binarySHA256') or
            receipt.get('width') != 3840 or receipt.get('height') != 2160 or
            receipt.get('gameIntegrationQualified') is not False or
            receipt.get('performanceQualified') is not False):
        raise ValueError('Owned presenter experiment receipt mismatch')


def gate(rows, code, timeout):
    if code != 0 or timeout or not rows or rows[-1] != {'stage': 'complete', 'result': 0}:
        return False
    phases = [row for row in rows if row.get('stage') == 'phase']
    switches = [row for row in rows if row.get('stage') == 'switch']
    rejected = [row for row in rows if row.get('stage') == 'reject-unsafe-switch']
    if len(phases) != 6 or len(switches) != 5 or len(rejected) != 5:
        return False
    for index, row in enumerate(phases):
        enabled = index in (2, 4)
        sdk = index in (0, 2, 4)
        count = 60 if enabled else 30
        if (row.get('phase') != index or row.get('enabled') is not enabled or
                row.get('sdkOwner') is not sdk or row.get('real') != count or
                row.get('generated') != (count if enabled else 0) or
                any(row.get(key) != 0 for key in ('fault', 'errors', 'warnings'))):
            return False
    for index, row in enumerate(switches):
        if (row.get('generation') != index + 2 or row.get('sdk') is not (index % 2 == 1) or
                row.get('identityStable') is not True or
                not isinstance(row.get('milliseconds'), int) or row['milliseconds'] < 0):
            return False
    if any(row.get('cases') != 4 for row in rejected):
        return False
    observers = [row for row in rows if row.get('stage') == 'concurrent-observer']
    if (len(observers) != 1 or observers[0].get('failures') != 0 or
            not isinstance(observers[0].get('samples'), int) or observers[0]['samples'] < 1):
        return False
    counts = {'owned-recording-contract': 1, 'load-verified-runtime': 1,
              'create-bridge-swapchain': 1, 'reject-empty-target': 1,
              'owned-presenter-antilag-ready': 1, 'clear-antilag-before-presentation': 1,
              'reject-unrelated-submission-queue': 1, 'set-presentation-colour': 1,
              'configure-output-contract': 1, 'prepare-game-bridge-inputs': 120,
              'copy-hudless-world': 120, 'configure-bridge-present': 150, 'complete': 1}
    for stage, count in counts.items():
        events = [row for row in rows if row.get('stage') == stage]
        if len(events) != count or any(row.get('result') != 0 for row in events):
            return False
    # Each handoff requires all four refusals before a new target is exposed.
    order = [row['stage'] for row in rows if row.get('stage') in
             ('phase', 'reject-unsafe-switch', 'switch')]
    return order == ['phase'] + ['reject-unsafe-switch', 'switch', 'phase'] * 5


def run(proton, prefix, steam, output, sdr=False):
    validate(output, prefix)
    env = os.environ.copy()
    for key in ('LD_PRELOAD', 'VKD3D_CONFIG', 'VKD3D_FEATURE_LEVEL', 'VKD3D_SHADER_DEBUG',
                'PROTON_ENABLE_HDR', 'DXVK_HDR', 'MANGOHUD', 'MANGOHUD_CONFIG'):
        env.pop(key, None)
    env.update(STEAM_COMPAT_DATA_PATH=str(prefix), STEAM_COMPAT_CLIENT_INSTALL_PATH=str(steam),
               STEAM_COMPAT_INSTALL_PATH=str(output), STEAM_COMPAT_APP_ID='0', SteamAppId='0', SteamGameId='0',
               WINEDEBUG='-all', PROTON_LOG='0', PROTON_ENABLE_WAYLAND='1', PROTON_FSR4_UPGRADE='0',
               WINEDLLOVERRIDES='d3d12,d3d12core,dxgi=n')
    application = output/'presenter-switch-probe.jsonl'
    application.unlink(missing_ok=True)
    timeout = False
    start = time.monotonic()
    with (output/'presenter-switch-run-private.log').open('w') as log:
        process = subprocess.Popen([str(proton), 'runinprefix', str(output/'presenter-switch-probe.exe'),
                                    *(['sdr'] if sdr else [])], cwd=output, env=env,
                                   stdout=log, stderr=subprocess.STDOUT, start_new_session=True)
        try:
            code = process.wait(timeout=50)
        except subprocess.TimeoutExpired:
            # This process group belongs exclusively to this isolated test host.
            timeout = True
            os.killpg(process.pid, signal.SIGTERM)
            try:
                code = process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                os.killpg(process.pid, signal.SIGKILL)
                code = process.wait(timeout=5)
    rows = [json.loads(line) for line in application.read_text().splitlines()] if application.exists() else []
    result = dict(exitCode=code, timeout=timeout, seconds=round(time.monotonic()-start, 3),
                  passed=gate(rows, code, timeout), applicationRows=rows,
                  gameIntegrationQualified=False, performanceQualified=False,
                  configuredColour='sRGB' if sdr else 'PQ', width=3840, height=2160,
                  nativePresentCountIsHostCalls=True,
                  probeSHA256=_base.digest(output/'presenter-switch-probe.exe'),
                  bridgeSHA256=_base.digest(output/'mcd2-fsr-fg-game-bridge.dll'), runtimeSHA256=_base.RUNTIME)
    (output/'presenter-switch-result.json').write_text(json.dumps(result, indent=2)+'\n')
    return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('proton', 'prefix', 'steam', 'output'):
        parser.add_argument('--'+name, required=True, type=Path)
    parser.add_argument('--sdr', action='store_true')
    args = parser.parse_args()
    result = run(*(getattr(args, name).resolve() for name in ('proton', 'prefix', 'steam', 'output')), sdr=args.sdr)
    print(json.dumps({key: value for key, value in result.items() if key != 'applicationRows'}))
    raise SystemExit(0 if result['passed'] else 1)
