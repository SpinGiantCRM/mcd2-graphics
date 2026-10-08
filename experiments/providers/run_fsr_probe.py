"""Run the FSR owned-input gate in an existing isolated Proton prefix only."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import signal
import subprocess
import time

RUNTIME = {'amd_fidelityfx_upscaler_dx12.dll': 'd0dcccc74a43c44ba435b7a369b456e0970d8a4464e4bd683119b374f2c9fb46'}

def validate(output, prefix):
    if 'steamapps' in prefix.parts or '1912410' in prefix.parts or not (prefix / 'pfx').is_dir():
        raise ValueError('Existing isolated prefix required; a Steam/game prefix is forbidden')
    receipt = json.loads((output / 'build-receipt.json').read_text())
    binary = output / 'fsr-owned-inputs.exe'
    if hashlib.sha256(binary.read_bytes()).hexdigest() != receipt.get('binarySHA256'):
        raise ValueError('Probe differs from its build receipt')
    if receipt.get('runtimeBundled') is not False or receipt.get('gameIntegrationQualified') is not False:
        raise ValueError('This is an isolated experiment, never a release/game qualifier')
    for name, expected in RUNTIME.items():
        if hashlib.sha256((output / name).read_bytes()).hexdigest() != expected:
            raise ValueError('Externally acquired pinned runtime required: ' + name)
    for name in ['dxgi.dll', 'd3d12.dll', 'd3d12core.dll', 'amdxc64.dll', 'sl.interposer.dll']:
        if (output / name).exists():
            raise ValueError('Unexpected proxy/driver in isolated output: ' + name)

def gate(rows, code, timeout):
    trials = [row for row in rows if row.get('stage') == 'trial']
    values = [row for row in rows if row.get('stage') == 'readback']
    completed = [row for row in rows if row.get('stage') == 'complete']
    destroys = [row for row in rows if row.get('stage') == 'destroy-context-after-fence']
    for stage, expected in [('create-context', 10), ('confirm-active-provider', 10), ('dispatch-owned-inputs', 160)]:
        if sum(row.get('stage') == stage for row in rows) != expected:
            return False
    if timeout or code != 0 or len(trials) != 10 or len(values) != 160 or len(completed) != 1 or len(destroys) != 10:
        return False
    if any(row.get('result') != 0 for row in rows if row.get('stage') in [
        'create-context', 'confirm-active-provider', 'dispatch-owned-inputs', 'destroy-context-after-fence']):
        return False
    if completed[0].get('dispatches') != 160 or completed[0].get('SDKerrors') != 0 or completed[0].get('SDKwarnings') != 0:
        return False
    for trial, row in enumerate(trials):
        if row.get('trial') != trial or row.get('quality') != trial % 5 or row.get('HDR') is not (trial >= 5):
            return False
        batch = values[trial * 16:(trial + 1) * 16]
        if trial >= 5 and any(item.get('maximum', 0) < 7.5 for item in batch):
            return False
        if any(item.get('frame') != frame or item.get('valid') is not True or item.get('HDR') is not (trial >= 5)
               for frame, item in enumerate(batch)):
            return False
    return True

def run(proton, prefix, steam, output):
    validate(output, prefix)
    env = os.environ.copy()
    for key in ['LD_PRELOAD', 'VKD3D_CONFIG', 'VKD3D_FEATURE_LEVEL', 'VKD3D_SHADER_DEBUG',
                'PROTON_ENABLE_HDR', 'DXVK_HDR', 'PROTON_LOG', 'MANGOHUD', 'MANGOHUD_CONFIG']:
        env.pop(key, None)
    env.update(STEAM_COMPAT_DATA_PATH=str(prefix), STEAM_COMPAT_CLIENT_INSTALL_PATH=str(steam),
               STEAM_COMPAT_INSTALL_PATH=str(output), STEAM_COMPAT_APP_ID='0', SteamAppId='0', SteamGameId='0',
               WINEDEBUG='-all', PROTON_LOG='0', PROTON_ENABLE_WAYLAND='1',
               WINEDLLOVERRIDES='d3d12,d3d12core,dxgi=n')
    application = output / 'fsr-owned-inputs.jsonl'
    application.unlink(missing_ok=True)
    start = time.monotonic()
    timeout = False
    with (output / 'run-private.log').open('w') as log:
        process = subprocess.Popen([str(proton), 'runinprefix', str(output / 'fsr-owned-inputs.exe')],
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
              'pass': gate(rows, code, timeout), 'applicationRows': rows, 'isolatedPrefix': True,
              'gameFilesChanged': False, 'gameIntegrationQualified': False,
              'binarySHA256': hashlib.sha256((output / 'fsr-owned-inputs.exe').read_bytes()).hexdigest(),
              'runtimeSHA256': RUNTIME}
    (output / 'run-result.json').write_text(json.dumps(result, indent=2) + '\n')
    return result

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ['proton', 'prefix', 'steam', 'output']:
        parser.add_argument('--' + name, required=True, type=Path)
    args = parser.parse_args()
    result = run(*(getattr(args, name).resolve() for name in ['proton', 'prefix', 'steam', 'output']))
    print(json.dumps({key: value for key, value in result.items() if key != 'applicationRows'}))
    raise SystemExit(0 if result['pass'] else 1)

if __name__ == '__main__':
    main()
