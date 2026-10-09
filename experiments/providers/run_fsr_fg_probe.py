"""Run the FSR FG owned-input gate in an existing isolated Proton prefix only."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import signal
import subprocess
import time

RUNTIME = {'amd_fidelityfx_framegeneration_dx12.dll': '02297beedd285e822d3a64f314cf00faf378dcec0edc47ff0c4dd71b3a8c2f18'}
FRAMEWORK = 'ce808bb1494415586cfb2ec5638a3ab0f0cea3879e45146a34976740b59fcf48'

def validate(output, prefix, framework=False, presentation=False):
    if any(part in ('steamapps','1912410') for candidate in [prefix.resolve(), (prefix/'pfx').resolve()] for part in candidate.parts) or not (prefix / 'pfx').is_dir():
        raise ValueError('Existing isolated prefix required; a Steam/game prefix is forbidden')
    receipt = json.loads((output / 'build-receipt.json').read_text())
    name = 'fsr-fg-presentation' if presentation else 'fsr-fg-owned-inputs'
    binary = output / (name + '.exe')
    if receipt.get('probeKind') != ('presentation' if presentation else 'owned-inputs'):
        raise ValueError('Probe kind differs from its build receipt')
    if hashlib.sha256(binary.read_bytes()).hexdigest() != receipt.get('binarySHA256'):
        raise ValueError('Probe differs from its build receipt')
    if receipt.get('runtimeBundled') is not False or receipt.get('gameIntegrationQualified') is not False:
        raise ValueError('This is an isolated experiment, never a release/game qualifier')
    for name, expected in RUNTIME.items():
        if hashlib.sha256((output / name).read_bytes()).hexdigest() != expected:
            raise ValueError('Externally acquired pinned runtime required: ' + name)
    for name in ['dxgi.dll', 'd3d12.dll', 'd3d12core.dll', 'amdxc64.dll', 'sl.interposer.dll']:
        if name == 'dxgi.dll' and framework:
            if hashlib.sha256((output / name).read_bytes()).hexdigest() != FRAMEWORK:
                raise ValueError('Unqualified ReShade framework')
            continue
        if (output / name).exists():
            raise ValueError('Unexpected proxy/driver in isolated output: ' + name)

def gate(rows, code, timeout, framework=False, presentation=False):
    if framework:
        declarations=[r for r in rows if r.get("stage")=="framework"]
        if len(declarations)!=1 or not all(declarations[0].get(key) is True for key in ["requested","exports","deviceWrapped"]):return False
    active = [r for r in rows if r.get('stage') == 'active-provider']
    if len(active) != (1 if presentation else 2) or any(r.get('id') != 17726168133342859270 for r in active):
        return False
    providers = [r for r in rows if r.get('stage') == 'provider']
    if not any(r.get('id') == 17726168133342859270 and r.get('name') == '3.1.6' for r in providers):
        return False
    if not retirement_gate(rows, 1 if presentation else 2, presentation):
        return False
    if presentation:return presentation_gate(rows, code, timeout)
    trials = [r for r in rows if r.get('stage') == 'trial']
    values = [r for r in rows if r.get('stage') == 'readback']
    complete = [r for r in rows if r.get('stage') == 'complete']
    if timeout or code != 0 or len(trials) != 2 or len(values) != 32 or len(complete) != 1:
        return False
    for stage, expected in [('create-context', 2), ('confirm-active-provider', 2), ('configure-frame', 32),
                            ('prepare-owned-inputs', 32), ('generate-owned-output', 32),
                            ('disable-before-destroy', 2), ('destroy-context-after-fence', 2)]:
        events = [r for r in rows if r.get('stage') == stage]
        if len(events) != expected or any(r.get('result') != 0 for r in events):
            return False
    if any(complete[0].get(key) != 0 for key in ['SDKerrors', 'SDKwarnings']) or complete[0].get('dispatches') != 32:
        return False
    for trial, row in enumerate(trials):
        if row.get('trial') != trial or row.get('HDR') is not (trial == 1) or row.get('swapchain') is not False:
            return False
        batch = values[trial * 16:(trial + 1) * 16]
        if any(r.get('frame') != frame or r.get('valid') is not True or r.get('HDR') is not (trial == 1)
               for frame, r in enumerate(batch)):
            return False
        if trial == 1 and any(r.get('maximum', 0) < 7.5 for r in batch):
            return False
    return complete[0].get('presentationQualified') is False

def retirement_gate(rows, expected, presentation):
    stages = [r.get('stage') for r in rows]
    order = ['invalidate-owned-recording', 'post-invalidation-fence', 'disable-before-destroy',
             'destroy-context-after-fence']
    if presentation:
        order = ['wait-for-presents', *order, 'destroy-swapchain-after-presents']
    for stage in order:
        events = [r for r in rows if r.get('stage') == stage]
        if len(events) != expected or any(r.get('result') != 0 for r in events):
            return False
    for generation in range(expected):
        positions = [[i for i, s in enumerate(stages) if s == stage][generation] for stage in order]
        if positions != sorted(positions):
            return False
        releases = [i for i, s in enumerate(stages) if s == 'release-owned-image' and i > positions[-1]]
        if not releases or (generation + 1 < expected and releases[0] >
                            [i for i, s in enumerate(stages) if s == order[0]][generation+1]):
            return False
    return True

def presentation_gate(rows, code, timeout):
    phases=[r for r in rows if r.get('stage')=='phase']
    complete=[r for r in rows if r.get('stage')=='complete']
    if timeout or code!=0 or len(phases)!=3 or len(complete)!=1:return False
    for i,r in enumerate(phases):
        real=60 if i==1 else 30
        if r.get('phase')!=i or r.get('enabled') is not (i==1) or r.get('realSubmissions')!=real or r.get('realCallbacks')!=real or r.get('callbackErrors')!=0:return False
        generated=r.get('generatedCallbacks',-1)
        if type(generated) is not int:return False
        if i==1 and not real//2 <= generated <= real:return False
        if i!=1 and generated!=0:return False
        if r.get('DXGIPresentDelta')!=real+generated:return False
    provider = [r for r in rows if r.get('stage') == 'swapchain-provider']
    if len(provider)!=1 or provider[0].get('id')!=17752306900579389447 or provider[0].get('name')!='3.1.7':return False
    for name,expected in [('create-context',1),('confirm-active-provider',1),('configure-frame',120),('prepare-owned-inputs',60),('present-real-frame',120),('query-before-present-count',3),('query-present-count',3),('create-owned-swapchain',1),('confirm-swapchain-provider',1),('wait-phase-presents',3),('disable-before-destroy',1),('destroy-context-after-fence',1),('wait-for-presents',1),('destroy-swapchain-after-presents',1)]:
        events=[r for r in rows if r.get('stage')==name]
        if len(events)!=expected or any(r.get('result')!=0 for r in events):return False
    return complete[0].get('realSubmissions')==120 and complete[0].get('SDKerrors')==0 and complete[0].get('SDKwarnings')==0 and complete[0].get('presentationQualified') is True

def run(proton, prefix, steam, output, framework=False, presentation=False):
    validate(output, prefix, framework, presentation)
    name = "fsr-fg-presentation" if presentation else "fsr-fg-owned-inputs"
    env = os.environ.copy()
    for key in ['LD_PRELOAD', 'VKD3D_CONFIG', 'VKD3D_FEATURE_LEVEL', 'VKD3D_SHADER_DEBUG',
                'PROTON_ENABLE_HDR', 'DXVK_HDR', 'PROTON_LOG', 'MANGOHUD', 'MANGOHUD_CONFIG', 'MCD2_FSR_EXPECT_RESHADE']:
        env.pop(key, None)
    env.update(STEAM_COMPAT_DATA_PATH=str(prefix), STEAM_COMPAT_CLIENT_INSTALL_PATH=str(steam),
               STEAM_COMPAT_INSTALL_PATH=str(output), STEAM_COMPAT_APP_ID='0', SteamAppId='0', SteamGameId='0',
               WINEDEBUG='-all', PROTON_LOG='0', PROTON_ENABLE_WAYLAND='1', PROTON_FSR4_UPGRADE='0',
               WINEDLLOVERRIDES='d3d12,d3d12core,dxgi=n')
    if framework:
        env['MCD2_FSR_EXPECT_RESHADE'] = '1'
    application = output / (name + '.jsonl')
    application.unlink(missing_ok=True)
    start = time.monotonic()
    timeout = False
    with (output / 'run-private.log').open('w') as log:
        process = subprocess.Popen([str(proton), 'runinprefix', str(output / (name + '.exe'))],
                                   cwd=output, env=env, stdout=log, stderr=subprocess.STDOUT, start_new_session=True)
        try:
            code = process.wait(timeout=50)
        except subprocess.TimeoutExpired:
            timeout = True
            # Only this experiment's newly created process group; never a game/server kill.
            os.killpg(process.pid, signal.SIGTERM)
            try:
                code = process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                os.killpg(process.pid, signal.SIGKILL)
                code = process.wait(timeout=5)
    rows = [json.loads(line) for line in application.read_text().splitlines()] if application.exists() else []
    result = {'exitCode': code, 'timeout': timeout, 'seconds': round(time.monotonic() - start, 3),
              'pass': gate(rows, code, timeout, framework, presentation), 'applicationRows': rows, 'isolatedPrefix': True,
              'gameFilesChanged': False, 'gameIntegrationQualified': False,
              'binarySHA256': hashlib.sha256((output / (name + '.exe')).read_bytes()).hexdigest(),
              'runtimeSHA256': RUNTIME}
    result['frameworkSHA256'] = FRAMEWORK if framework else None
    result['probeKind'] = 'presentation' if presentation else 'owned-inputs'
    (output / 'run-result.json').write_text(json.dumps(result, indent=2) + '\n')
    return result

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ['proton', 'prefix', 'steam', 'output']:
        parser.add_argument('--' + name, required=True, type=Path)
    parser.add_argument('--presentation', action='store_true')
    parser.add_argument('--qualified-reshade', action='store_true')
    args = parser.parse_args()
    result = run(*(getattr(args, name).resolve() for name in ['proton', 'prefix', 'steam', 'output']), framework=args.qualified_reshade, presentation=args.presentation)
    print(json.dumps({key: value for key, value in result.items() if key != 'applicationRows'}))
    raise SystemExit(0 if result['pass'] else 1)

if __name__ == '__main__':
    main()
