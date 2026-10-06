"""Package only our FG test binaries, receipts and reversible trial helpers."""
import argparse
import hashlib
import json
from pathlib import Path
import zipfile


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('probe', 'candidate', 'sr', 'ui', 'output', 'framework-receipt'):
        parser.add_argument('--' + name, required=True, type=Path)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    probe = json.loads((args.probe / 'build-receipt.json').read_text())
    latency = json.loads((args.candidate / 'build-receipt.json').read_text())
    sr = json.loads((args.sr / 'build-receipt.json').read_text())
    framework = json.loads(args.framework_receipt.read_text())
    if not probe['earlyBootstrap'] or latency['FGEnabled'] or sr['WindowsQualified']:
        raise ValueError('Expected an unqualified FG-Off test candidate')
    if latency['probeBuild'] != probe:
        raise ValueError('Latency receipt references a different probe build')
    if not framework.get('sourceClean') or not framework.get('commit'):
        raise ValueError('A clean experimental framework receipt is required')
    if sr.get('privateGuideInspection', False):
        raise ValueError('Private guide inspection builds cannot be packaged')
    ui = json.loads((args.ui / 'build-receipt.json').read_text())
    if ui.get('privateProbe') is not False:
        raise ValueError('Expected a clean UI with no private probe')
    files = {}
    for name, expected in probe['ownBinariesSHA256'].items():
        if Path(name).name != name:
            raise ValueError('Unsafe binary name')
        path = args.probe / name
        if digest(path) != expected:
            raise ValueError('Own probe binary mismatch: ' + name)
        files['probe/' + name] = path
    for folder, receipt, name in ((args.candidate, latency, 'mcd2-display-latency.addon64'),
                                  (args.sr, sr, 'mcd2-graphics.addon64')):
        if digest(folder / name) != receipt['addonSHA256']:
            raise ValueError('Addon mismatch: ' + name)
        key = 'latency' if folder == args.candidate else 'sr'
        files[key + '/' + name] = folder / name
        files[key + '/build-receipt.json'] = folder / 'build-receipt.json'
    for name, expected in ui['payloadSHA256'].items():
        if name not in ('MCD2Graphics_P.pak', 'MCD2Graphics_P.utoc', 'MCD2Graphics_P.ucas'):
            raise ValueError('Unexpected UI payload')
        path = args.ui / 'Pak' / name
        if digest(path) != expected:
            raise ValueError('UI payload mismatch: ' + name)
        files['ui/Pak/' + name] = path
    if len(ui['payloadSHA256']) != 3:
        raise ValueError('Expected all three UI containers')
    files['ui/build-receipt.json'] = args.ui / 'build-receipt.json'
    files['probe/build-receipt.json'] = args.probe / 'build-receipt.json'
    files['latency/FGBootstrap.ini'] = args.candidate / 'FGBootstrap.ini'
    for name in ('install.py', 'dependencies.lock.json'):
        files[name] = root / name
    for name in ('game_trial.py', 'bounded_trial.py', 'hydrate_runtime.py', 'store_probe.py',
                 'STORE_INVESTIGATION_2026-10-06.md', 'LINUX_BOUNDED_VALIDATION_2026-10-06.json', 'LINUX_DLSS_FOLIAGE_FG_CANDIDATE_2026-10-06.json', 'WINDOWS_FG_CLEARANCE_2026-10-06.md', 'LINUX_FG_PERFORMANCE_2026-10-06.json', 'LINUX_FG_PERFORMANCE_2026-10-06.md', 'README.md'):
        files['experiments/fg-streamline/' + name] = Path(__file__).parent / name
    manifest = {
        'purpose': 'FG and DLSS foliage Windows test candidate; not a release',
        'FGDefault': 'Off', 'nativeUIToggle': True, 'restartAfterModeChange': True,
        'legacyMaximumFGFrames': 600, 'nativeUIToggleHasExpiry': False,
        'WindowsQualified': False, 'releaseQualified': False,
        'framework': {'commit': framework['commit'], 'binarySHA256': framework['binarySHA256']},
        'requiredSeparateRuntimeFiles': probe['files'],
        'files': {name: digest(path) for name, path in files.items()},
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(args.output, 'x', compression=zipfile.ZIP_DEFLATED) as archive:
        for name, path in files.items():
            archive.writestr(name, path.read_bytes())
        archive.writestr('candidate-manifest.json', json.dumps(manifest, indent=2) + '\n')
    print(json.dumps({'SHA256': digest(args.output), 'ownFileCount': len(files),
                      'vendorRuntimesBundled': False, 'WindowsQualified': False}))


if __name__ == '__main__':
    main()
