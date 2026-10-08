"""Build an isolated Windows FG trial artifact; never publish vendor runtimes."""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import urllib.request
import zipfile
from framework_provenance import RESHADE_COMMIT

REPO = Path(__file__).resolve().parents[2]


def run(*args):
    subprocess.run([str(arg) for arg in args], check=True)


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def copy_current_ui(repo, artifact):
    # UI logic and metadata have separate provenance. Never reuse the old v27
    # metadata just because ModActor.cs still matches its original receipt.
    candidate_receipt = repo / 'qualification/providers/current-ui-build-receipt.json'
    is_candidate = candidate_receipt.exists()
    receipt_path = candidate_receipt if is_candidate else repo / 'qualification/fg-release/ui-build-receipt.json'
    ui = json.loads(receipt_path.read_text())
    if is_candidate:
        expected_sources = {'src/ui/' + name for name in ('ModActor.cs', 'ProviderSaveWords.cs',
                            'ProviderMenuClient.cs', 'ProviderRuntimeWords.cs', 'ProviderRuntimeClient.cs')}
        expected_sources.add('tools/ModInfoBuilder/Program.cs')
        if ui.get('candidateOnly') is not True or set(ui.get('sourceFileSHA256', {})) != expected_sources:
            raise RuntimeError('Incomplete candidate UI source provenance')
        source_hashes = ui['sourceFileSHA256']
        archive_path = repo / 'qualification/providers/current-ui-payload.zip'
        if sha(archive_path) != ui['archiveSHA256']:
            raise RuntimeError('Candidate UI archive hash mismatch')
    else:
        source_hashes = {'src/ui/ModActor.cs': ui['sourceSHA256'],
                         'tools/ModInfoBuilder/Program.cs': ui['modInfoBuilderSHA256']}
        archive_path = repo / 'qualification/fg-release/own-payload.zip'
    for source_path, expected in source_hashes.items():
        source = subprocess.check_output(['git', '-C', str(repo), 'show', 'HEAD:' + source_path])
        if hashlib.sha256(source).hexdigest() != expected:
            raise RuntimeError('UI source changed: rebuild UI/metadata instead of reusing the payload')
    manifest = json.loads((repo / 'manifest.json').read_text())
    if ui['version'] != manifest['version']:
        raise RuntimeError('UI metadata version differs from the manifest')
    blobs = {}
    with zipfile.ZipFile(archive_path) as archive:
        if is_candidate and set(archive.namelist()) != {'Dungeons/Content/Paks/~mods/MCD2Graphics/' + name
                                                       for name in ui['payloadSHA256']}:
            raise RuntimeError('Unexpected candidate UI archive member')
        for name, expected in ui['payloadSHA256'].items():
            relative = 'Dungeons/Content/Paks/~mods/MCD2Graphics/' + name
            if name not in {'MCD2Graphics_P.pak', 'MCD2Graphics_P.ucas', 'MCD2Graphics_P.utoc'}:
                raise RuntimeError('Unexpected UI payload member')
            if not is_candidate and manifest['files'].get(relative) != expected:
                raise RuntimeError('UI receipt differs from the package manifest')
            if archive.namelist().count(relative) != 1:
                raise RuntimeError('Missing or duplicate UI payload member')
            blob = archive.read(relative)
            if hashlib.sha256(blob).hexdigest() != expected:
                raise RuntimeError('UI payload hash mismatch')
            blobs[name] = blob
    if len(blobs) != 3:
        raise RuntimeError('Incomplete UI receipt')
    target = artifact / 'ui/Pak'
    target.mkdir(parents=True)
    for name, blob in blobs.items():
        (target / name).write_bytes(blob)
    shutil.copyfile(receipt_path, artifact / 'ui/build-receipt.json')


def download(url, target, expected):
    urllib.request.urlretrieve(url, target)
    if sha(target) != expected:
        raise RuntimeError('Downloaded archive hash mismatch: ' + target.name)


def junction(target, source):
    # A private sysroot adapts the installed VS layout to the existing recipe.
    # No files or settings in the installed compiler/SDK are modified.
    target.parent.mkdir(parents=True, exist_ok=True)
    run('cmd', '/c', 'mklink', '/J', target, source)


def latest_version(parent, required):
    """Windows Kits also contains unversioned components such as wdf."""
    candidates = [path for path in parent.iterdir()
                  if path.is_dir() and all(part.isdecimal() for part in path.name.split('.'))
                  and all((path / name).exists() for name in required)]
    if not candidates:
        raise RuntimeError('No complete versioned toolchain in ' + str(parent))
    return max(candidates, key=lambda path: tuple(map(int, path.name.split('.'))))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    artifact = out / 'artifact'
    artifact.mkdir()
    lock = json.loads((REPO / 'dependencies.lock.json').read_text())
    sdk_zip = out / 'streamline.zip'
    stream = lock['dependencies']['Streamline']
    download(stream['url'], sdk_zip, stream['archiveSHA256'])
    sdk = out / 'streamline'
    with zipfile.ZipFile(sdk_zip) as archive:
        archive.extractall(sdk)
    mingw_zip = out / 'mingw.zip'
    download('https://github.com/mstorsjo/llvm-mingw/releases/download/20260922/llvm-mingw-20260922-ucrt-x86_64.zip', mingw_zip,
             'e3ad77d117a4bea19a7a3b333341824d79a5a371004a10e25b8504e7b3047666')
    with zipfile.ZipFile(mingw_zip) as archive:
        archive.extractall(out)
    mingw = out / 'llvm-mingw-20260922-ucrt-x86_64/bin/clang++.exe'
    dxc_zip = out / 'dxc.zip'
    download('https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.9.2609/dxc_2026_09_29.zip', dxc_zip,
             'ad31b1fc8443175d204f77a611fdb3ef2ec42759bdc2f1167368de24a4a7e7f1')
    with zipfile.ZipFile(dxc_zip) as archive:
        archive.extractall(out / 'dxc')
    reshade = out / 'reshade'
    run('git', 'clone', '--no-checkout', 'https://github.com/JoeyDelp/reshade.git', reshade)
    run('git', '-C', reshade, 'checkout', RESHADE_COMMIT)
    run('git', '-C', reshade, 'submodule', 'update', '--init', '--recursive')
    patch = REPO / 'qualification/fg-release/reshade-reset-epoch.patch'
    if subprocess.check_output(['git', '-C', str(reshade), 'status', '--porcelain']).strip():
        raise RuntimeError('Framework base checkout is not clean')
    run('git', '-C', reshade, 'apply', '--index', patch)
    approved_diff = subprocess.check_output(['git', '-C', str(reshade), 'diff', '--cached', '--binary'])
    approved_status = subprocess.check_output(['git', '-C', str(reshade), 'status', '--porcelain'])
    # The upstream solution supplies SolutionDir used by dependency includes.
    # Building the vcxproj directly leaves those Windows include paths empty.
    run('msbuild', reshade / 'ReShade.sln', '/p:Configuration=Release', '/p:Platform=64-bit', '/m:2', '/verbosity:minimal')
    binary = reshade / 'bin/x64/Release/ReShade64.dll'
    clean = not subprocess.check_output(['git', '-C', str(reshade), 'status', '--porcelain']).strip()
    if subprocess.check_output(['git', '-C', str(reshade), 'status', '--porcelain']) != approved_status or subprocess.check_output(['git', '-C', str(reshade), 'diff', '--cached', '--binary']) != approved_diff:
        raise RuntimeError('Framework source differs from the approved reset patch')
    run('git', '-C', reshade, 'diff', '--quiet')
    framework = artifact / 'experimental-reshade'
    framework.mkdir()
    shutil.copyfile(binary, framework / binary.name)
    (framework / 'build-receipt.json').write_text(json.dumps({
        'commit': RESHADE_COMMIT, 'sourceClean': clean, 'binarySHA256': sha(binary),
        'baseCleanBeforePatch': True, 'approvedPatchOnly': True, 'patchSHA256': sha(patch),
        'configuration': 'Release x64 full addon support', 'WindowsQualified': False,
    }, indent=2) + '\n')
    vswhere = Path(os.environ['ProgramFiles(x86)']) / 'Microsoft Visual Studio/Installer/vswhere.exe'
    vs = Path(subprocess.check_output([str(vswhere), '-latest', '-products', '*', '-requires', 'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath'], text=True).strip())
    crt = latest_version(vs / 'VC/Tools/MSVC', ['include', 'lib/x64'])
    kits = Path(os.environ['ProgramFiles(x86)']) / 'Windows Kits/10'
    sdk_version = latest_version(kits / 'Include', ['ucrt', 'shared', 'um', 'winrt']).name
    sysroot = out / 'sysroot'
    junction(sysroot / 'crt/include', crt / 'include')
    junction(sysroot / 'crt/lib/x86_64', crt / 'lib/x64')
    for folder in ['ucrt', 'shared', 'um', 'winrt']:
        junction(sysroot / 'sdk/include' / folder, kits / 'Include' / sdk_version / folder)
    for folder in ['ucrt', 'um']:
        junction(sysroot / 'sdk/lib' / folder / 'x86_64', kits / 'Lib' / sdk_version / folder / 'x64')
    llvm = Path(os.environ['ProgramFiles']) / 'LLVM/bin'
    # Compile the isolated FSR SDK gate with the same Microsoft ABI. Download
    # only public MIT headers; vendor runtime DLLs stay external and are never
    # added to this artifact. Hosted CI cannot qualify active GPU execution.
    spec = importlib.util.spec_from_file_location('fsr_recipe', REPO / 'experiments/providers/build_fsr_probe.py')
    fsr_recipe = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(fsr_recipe)
    fsr_sdk, fsr_probe = out / 'fsr-sdk', out / 'fsr-owned-inputs'
    for name, expected in fsr_recipe.HEADERS.items():
        destination = fsr_sdk / name
        destination.parent.mkdir(parents=True, exist_ok=True)
        download('https://raw.githubusercontent.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/' +
                 fsr_recipe.COMMIT + '/Kits/FidelityFX/' + name, destination, expected)
    fsr_recipe.build(fsr_sdk, sysroot, fsr_probe, str(llvm / 'clang-cl.exe'), str(llvm / 'lld-link.exe'))
    run(sys.executable, REPO / 'experiments/providers/build_fsr_game_bridge.py',
        '--sdk', fsr_sdk, '--windows-sysroot', sysroot, '--output', out / 'fsr-game-bridge',
        '--clang-cl', llvm / 'clang-cl.exe', '--linker', llvm / 'lld-link.exe')
    fsr_bridge_destination = artifact / 'isolated-fsr-game-bridge'
    fsr_bridge_destination.mkdir()
    for name in ['mcd2-fsr-game-bridge.dll', 'build-receipt.json', 'FSR-SDK-HEADERS-LICENSE.txt']:
        shutil.copyfile(out / 'fsr-game-bridge' / name, fsr_bridge_destination / name)
    fsr_destination = artifact / 'isolated-fsr-probe'
    fsr_destination.mkdir()
    for name in ['fsr-owned-inputs.exe', 'build-receipt.json', 'FSR-SDK-HEADERS-LICENSE.txt']:
        shutil.copyfile(fsr_probe / name, fsr_destination / name)
    scripts = REPO / 'experiments/fg-streamline'
    probe = out / 'probe'
    run(sys.executable, scripts / 'build_probe.py', '--sdk', sdk, '--sysroot', sysroot,
        '--runtime-archive', sdk_zip, '--output', probe, '--reshade-headers', reshade / 'include',
        '--early-bootstrap', '--guide-recon', '--dxc', out / 'dxc/bin/x64/dxc.exe',
        '--clang-cl', llvm / 'clang-cl.exe', '--lld-link', llvm / 'lld-link.exe')
    latency = out / 'latency'
    run(sys.executable, scripts / 'build_game_candidate.py', '--probe-output', probe,
        '--reshade-headers', reshade / 'include', '--output', latency, '--mingw-cxx', mingw)
    amd_latency = out / 'amd-latency'
    run(sys.executable, REPO / 'build_amd_latency.py', '--windows-sysroot', sysroot,
        '--output', amd_latency, '--clang-cl', llvm / 'clang-cl.exe', '--linker', llvm / 'lld-link.exe', '--with-tests')
    run(amd_latency / 'amd-antilag-test.exe')
    ngx = out / 'ngx'
    run('git', 'clone', '--no-checkout', 'https://github.com/NVIDIA/DLSS.git', ngx)
    run('git', '-C', ngx, 'checkout', lock['buildDependencies']['NVIDIASDKCommit'])
    sr = out / 'sr'
    run(sys.executable, scripts / 'build_sr_candidate.py', '--output', sr,
        '--reshade-headers', reshade / 'include', '--ngx-headers', ngx / 'include',
        '--clang-cxx', llvm / 'clang++.exe', '--mingw-cxx', mingw)
    fsr_sustained = out / 'fsr-sustained'
    run(sys.executable, REPO / 'experiments/providers/build_fsr_sustained_probe.py',
        '--output', fsr_sustained, '--frames', '256', '--replace-output', '--world-to-meters', '100',
        '--quality-mode', '1', '--fail-after', '128',
        '--reshade-include', reshade / 'include', '--ngx-include', ngx / 'include',
        '--clang-cxx', llvm / 'clang++.exe', '--mingw-cxx', mingw)
    fsr_sustained_destination = artifact / 'isolated-fsr-sustained-probe'
    fsr_sustained_destination.mkdir()
    shutil.copyfile(fsr_sustained / 'native/mcd2-graphics.addon64', fsr_sustained_destination / 'mcd2-graphics.addon64')
    shutil.copyfile(fsr_sustained / 'build-receipt.json', fsr_sustained_destination / 'build-receipt.json')
    for folder in [probe, latency, sr]:
        destination = artifact / folder.name
        destination.mkdir()
        for name in ['build-receipt.json', 'FGBootstrap.ini', 'FG_UI_ALPHA.cso']:
            if (folder / name).exists():
                shutil.copyfile(folder / name, destination / name)
        receipt = json.loads((folder / 'build-receipt.json').read_text())
        names = receipt.get('ownBinariesSHA256', {}) if folder == probe else {folder.name: ''}
        if folder != probe:
            names = {'mcd2-display-latency.addon64' if folder == latency else 'mcd2-graphics.addon64': ''}
        for name in names:
            shutil.copyfile(folder / name, destination / name)
    copy_current_ui(REPO, artifact)
    amd_destination = artifact / 'amd-latency'
    amd_destination.mkdir()
    for name in ['mcd2-antilag2-bridge.dll', 'build-receipt.json']:
        shutil.copyfile(amd_latency / name, amd_destination / name)
    shutil.copyfile(REPO / 'third-party/AntiLag2/LICENSE.txt', amd_destination / 'LICENSE.txt')
    files = {path.relative_to(artifact).as_posix(): sha(path) for path in artifact.rglob('*') if path.is_file()}
    (artifact / 'windows-build-receipt.json').write_text(json.dumps({
        'sourceCommit': subprocess.check_output(['git', '-C', str(REPO), 'rev-parse', 'HEAD'], text=True).strip(),
        'files': files, 'runtimeQualified': False, 'vendorRuntimeBundled': False,
        'crtVersion': crt.name, 'windowsSDK': sdk_version,
    }, indent=2) + '\n')


if __name__ == '__main__':
    try:
        main()
    except subprocess.CalledProcessError:
        # Child recipes save compiler errors instead of printing them. Surface
        # build diagnostics in CI; no game/private runtime logs are collected.
        parser = argparse.ArgumentParser()
        parser.add_argument('--output', type=Path, required=True)
        args = parser.parse_args()
        for log in args.output.rglob('build-private.log'):
            print(log.read_text(errors='replace'))
        raise
