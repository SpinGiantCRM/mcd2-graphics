"""Assemble a development-only installer input from a verified Windows artifact.

No network, game writes, vendor DLL bundling or qualification claims. Pass the
archive digest and source commit from an independently checked CI receipt.
"""
import argparse
import copy
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import zipfile

ROOT = Path(__file__).resolve().parents[1]
BIN = 'Dungeons/Binaries/Win64/'
PAK = 'Dungeons/Content/Paks/~mods/MCD2Graphics/'
OWN = {
    'probe/dxgi.dll': BIN+'dxgi.dll',
    'probe/fg-sdk-bridge.dll': BIN+'fg-sdk-bridge.dll',
    'probe/mcd2-fg-guide-recon.addon64': BIN+'mcd2-fg-guide-recon.addon64',
    'probe/FG_UI_ALPHA.cso': BIN+'FG_UI_ALPHA.cso',
    'latency/mcd2-display-latency.addon64': BIN+'mcd2-display-latency.addon64',
    'amd-latency/mcd2-antilag2-bridge.dll': BIN+'mcd2-antilag2-bridge.dll',
    'sr/mcd2-graphics.addon64': BIN+'mcd2-graphics.addon64',
    'sr/live_dense.cso': BIN+'MCD2Graphics/live_dense.cso',
    'sr/fsr_dense.cso': BIN+'MCD2Graphics/fsr_dense.cso',
    'isolated-fsr-game-bridge/mcd2-fsr-game-bridge.dll': BIN+'MCD2Graphics/fsr/mcd2-fsr-game-bridge.dll',
    'isolated-fsr-fg-game-bridge/mcd2-fsr-fg-game-bridge.dll': BIN+'mcd2-fsr-fg-game-bridge.dll',
    **{'ui/Pak/'+n: PAK+n for n in ('MCD2Graphics_P.pak','MCD2Graphics_P.ucas','MCD2Graphics_P.utoc')},
}
CONFIG = {
    BIN+'FGBootstrap.ini': b'[Experiment]\nEnableSDK=1\nFactoryRouting=1\nTraceNvapi=0\n[Providers]\nConsolidatedMenuTransport=1\nObserveLegacySettings=0\nAmdAntiLag2=1\n',
    BIN+'FGGuideCapture.ini': b'[Capture]\nEnabled=0\nGuideTags=1\nImageTags=1\nEnableFG=0\nTrialRevision=0\nNativeUIToggle=1\n',
}
FSR_COMMIT = '60f4ea81909200d8542eca14dccb2628b763a9a3'
GAME = {'steamBuildID':'25754144', 'exeSHA256':'3a8703406fd50520f83c4f70a0212c000cb3b584ef28eb38032902230c01ebdd'}
FSR_RUNTIME = {
    'FSRUpscaler': (BIN+'MCD2Graphics/fsr/amd_fidelityfx_upscaler_dx12.dll', 'd0dcccc74a43c44ba435b7a369b456e0970d8a4464e4bd683119b374f2c9fb46'),
    'FSRFrameGeneration': (BIN+'amd_fidelityfx_framegeneration_dx12.dll', '02297beedd285e822d3a64f314cf00faf378dcec0edc47ff0c4dd71b3a8c2f18'),
}

def digest(data):
    return hashlib.sha256(data).hexdigest()

def safe_name(name):
    p = PurePosixPath(name)
    if not name or ':' in name or '\\' in name or p.is_absolute() or any(x in ('', '.', '..') for x in name.split('/')):
        raise ValueError('Unsafe artifact member')

def assemble(archive, archive_sha, source_commit, output, version, artifact_url):
    if not re.fullmatch('[a-f0-9]{64}', archive_sha) or not re.fullmatch('[a-f0-9]{40}', source_commit):
        raise ValueError('Supply exact archive and source pins')
    if not re.fullmatch(r'\d+\.\d+\.\d+-amd\.dev\.\d+', version):
        raise ValueError('Use a distinct AMD development version')
    if not re.fullmatch(r'https://github\.com/SpinGiantCRM/mcd2-graphics/actions/runs/\d+/artifacts/\d+', artifact_url):
        raise ValueError('Supply the CI artifact page, not a signed download URL')
    data = archive.read_bytes()
    if digest(data) != archive_sha:
        raise ValueError('Artifact archive hash mismatch')
    with zipfile.ZipFile(archive) as z:
        infos = z.infolist()
        names = [i.filename for i in infos]
        if len(set(names)) != len(names):
            raise ValueError('Duplicate artifact member')
        total = 0
        for i in infos:
            safe_name(i.filename)
            total += i.file_size
            if i.file_size > 512*1024*1024 or total > 2*1024*1024*1024 or (i.external_attr >> 16 & 0xf000) == 0xa000:
                raise ValueError('Artifact exceeds file limits or contains a symlink')
        receipt = json.loads(z.read('windows-build-receipt.json'))
        if receipt['sourceCommit'] != source_commit:
            raise ValueError('Artifact source commit mismatch')
        pins = receipt['files']
        if set(names) != set(pins) | {'windows-build-receipt.json'}:
            raise ValueError('Artifact has unrecorded or missing files')
        content = {}
        for name, pin in pins.items():
            safe_name(name)
            b = z.read(name)
            if digest(b) != pin:
                raise ValueError('Artifact member hash mismatch: '+name)
            content[name] = b
        sr = json.loads(content['sr/build-receipt.json'])
        if sr['fsrBridgeSHA256'] != pins['isolated-fsr-game-bridge/mcd2-fsr-game-bridge.dll']:
            raise ValueError('SR/FSR bridge binding mismatch')
        ui = json.loads(content['ui/build-receipt.json'])
        for name, pin in ui['payloadSHA256'].items():
            if pins.get('ui/Pak/'+name) != pin:
                raise ValueError('UI payload receipt mismatch')
        payload = {dest: content[src] for src, dest in OWN.items()}
        payload.update(CONFIG)
        manifest = json.loads((ROOT/'manifest.json').read_bytes())
        prior = copy.deepcopy(manifest['upgradeFrom'])
        # Released installs own their private NGX copy as well as the mod files.
        baseline_lock = json.loads((ROOT/'dependencies.lock.json').read_bytes())
        old_files = dict(manifest['files'])
        runtime = baseline_lock['dependencies']['DLSSRuntime']
        old_files[runtime['file']] = runtime['sha256']
        prior[manifest['version']] = old_files
        manifest = {'version': version, 'candidateOnly': True,
                    'description': 'Development candidate: FSR upscaling and AMD Frame Generation; Anti-Lag 2 requires a supported Windows Radeon driver. Windows GPU qualification is pending.',
                    'uiMetadataVersion': ui['version'],
                    'files': {n: digest(b) for n,b in sorted(payload.items())}, 'upgradeFrom': prior}
        lock = copy.deepcopy(baseline_lock)
        # This candidate implements the explicitly signature-gated updated map.
        # The old released installer lock remains untouched.
        lock['game'] = dict(GAME)
        framework = lock['dependencies']['ReShade']
        previous_framework_pin = framework['sha256']
        # The framework remains a separately selected dependency, never payload.
        for key in ('download','archiveSHA256','archiveMembers','buildReceiptSHA256'):
            framework.pop(key, None)
        framework.update(sha256=pins['experimental-reshade/ReShade64.dll'], url=artifact_url,
                         version='PR435 / exact Windows candidate build',
                         upgradeFromFiles={framework['file']:previous_framework_pin},
                         compatibilityNote='Select experimental-reshade/ReShade64.dll from the pinned CI artifact. Installed separately as d3d12.asi; active Windows qualification is pending.')
        for ident,(path,pin) in FSR_RUNTIME.items():
            lock['dependencies'][ident] = {'version':'SDK 2.3 analytical', 'file':path, 'sha256':pin,
                'sdkCommit': FSR_COMMIT, 'bundled':False,
                'url':'https://github.com/amd/FidelityFX-SDK/tree/'+FSR_COMMIT,
                'compatibilityNote':'Official pinned analytical runtime. An installer override does not relax the native runtime hash/API or actual hardware checks.'}
        metadata = {'schemaVersion':1, 'candidateOnly':True, 'sourceCommit':source_commit,
            'artifactUrl':artifact_url, 'archiveSHA256':archive_sha,
            'verifiedArtifactFiles':len(pins), 'payload':manifest['files'],
            'uiMetadataVersion':ui['version'], 'vendorRuntimeBundled':False,
            'frameworkBundled':False, 'windowsRuntimeQualified':False,
            'excludedDiagnostics': ['probe/fg-chain-observer.addon64', 'isolated probe executables/addons'],
            'sourceBuildReceipts': {n: pins[n] for n in pins if n.endswith('build-receipt.json')}}
    notices = 'AMD candidate bridge notices\n\nVendor runtimes and ReShade are acquired separately; none is bundled in the own payload.\n\n'
    for name in ('third-party/FSR-SDK-HEADERS-LICENSE.txt','third-party/AntiLag2/LICENSE.txt'):
        notices += '\n--- '+name+' ---\n\n'+(ROOT/name).read_text(encoding='utf-8')
    # Complete all validation, including license availability, before any output.
    if output.exists() and any(output.iterdir()):
        raise ValueError('Use an empty candidate output directory')
    output.mkdir(parents=True,exist_ok=True)
    for name,b in payload.items():
        p = output/'payload'/name; p.parent.mkdir(parents=True,exist_ok=True); p.write_bytes(b)
    for name,obj in [('manifest.json',manifest),('dependencies.lock.json',lock)]:
        (output/name).write_text(json.dumps(obj,indent=2)+'\n',encoding='utf-8')
    (output/'AMD_BRIDGE_NOTICES.txt').write_text(notices,encoding='utf-8')
    receipts = output/'provenance'; receipts.mkdir()
    for name,b in content.items():
        if name.endswith('build-receipt.json'):
            p=receipts/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(b)
    (receipts/'windows-build-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    (output/'assembly-receipt.json').write_text(json.dumps(metadata,indent=2)+'\n',encoding='utf-8')
    return metadata

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--artifact',type=Path,required=True)
    p.add_argument('--archive-sha256',required=True)
    p.add_argument('--source-commit',required=True)
    p.add_argument('--artifact-url',required=True)
    p.add_argument('--version',required=True)
    p.add_argument('--output',type=Path,required=True)
    a=p.parse_args()
    r=assemble(a.artifact,a.archive_sha256,a.source_commit,a.output,a.version,a.artifact_url)
    print('Assembled',len(r['payload']),'own files. Development inputs only; Windows GPU qualification remains required.')

if __name__=='__main__': main()
