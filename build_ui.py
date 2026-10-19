"""Compile UI into an isolated directory; never install into a game directory."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parent
UI_SOURCES = ('ModActor.cs', 'ProviderSaveWords.cs', 'ProviderMenuClient.cs',
              'ProviderRuntimeWords.cs', 'ProviderRuntimeClient.cs', 'AdvancedSettings.cs', 'AdvancedInfoWidget.cs', 'AdvancedSettingsPanel.cs', 'NativeSettingsWidgets.cs', 'RuntimeDiagnostics.cs', 'RuntimeDiagnosticsOverlay.cs', 'UpscalingSharpness.cs')
SOURCE_INPUTS = tuple('src/ui/' + name for name in UI_SOURCES) + (
    'src/ui/mod-info.json', 'build_ui.py', 'dependencies.lock.json', 'manifest.json')
PAYLOAD_NAMES = tuple('MCD2Graphics_P.' + ext for ext in ('pak', 'ucas', 'utoc'))


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def source_sha(path):
    # Match canonical Git text blobs even on a Windows CRLF checkout.
    return hashlib.sha256(Path(path).read_bytes().replace(b'\r\n', b'\n')).hexdigest()


def response_file(arguments):
    # NeoRune response files use one complete argument per line, not shell quoting.
    if any('\n' in value or '\r' in value for value in arguments):
        raise ValueError('UI compiler arguments must not contain line breaks')
    return '\n'.join(arguments) + '\n'


def build_ui(dotnet, sdk, pack_tools, output, *, actor=None, extra_sources=()):
    dotnet, sdk, pack_tools, output = (Path(p).resolve() for p in (dotnet, sdk, pack_tools, output))
    if output.exists() and any(output.iterdir()):
        raise ValueError('UI output directory must be empty')
    pins = json.loads((ROOT / 'dependencies.lock.json').read_text())['buildDependencies']
    nuspec = ET.parse(sdk / 'NeoRune.Sdk.nuspec').getroot()
    version = next(node.text for node in nuspec.iter() if node.tag.rsplit('}', 1)[-1] == 'version')
    if version != pins['NeoRuneSDK'] or version != pins['NeoRuneTool']:
        raise ValueError('NeoRune package differs from the build dependency pin')
    installed = subprocess.check_output([str(dotnet), '--version'], text=True).strip()
    if installed != pins['dotnetSDK']:
        raise ValueError('Expected .NET SDK ' + pins['dotnetSDK'])
    refs = dotnet.parent / 'packs/Microsoft.NETCore.App.Ref' / pins['dotnetReferencePack'] / 'ref/net10.0'
    if not refs.is_dir() or not list(refs.glob('*.dll')):
        raise ValueError('Missing pinned .NET reference pack')
    helpers = sorted((sdk / 'src').glob('*.cs'))
    if not helpers or not all((sdk / 'ref' / name).is_file() for name in ('NeoRune.Abstractions.dll', 'NeoRune.Game.dll')):
        raise ValueError('Incomplete NeoRune SDK')
    metadata = json.loads((ROOT / 'src/ui/mod-info.json').read_text())
    version = json.loads((ROOT / 'manifest.json').read_text())['version']
    sources = [Path(actor).resolve() if actor else ROOT / 'src/ui/ModActor.cs']
    sources += [ROOT / 'src/ui' / name for name in UI_SOURCES[1:]]
    sources += [Path(path).resolve() for path in extra_sources]
    args = ['build', '--mod=MCD2Graphics', '--out=' + str(output), '--tools=' + str(pack_tools)]
    args += ['--title=' + metadata['title'], '--version=' + version,
             '--author=' + metadata['author'], '--author-url=' + metadata['authorURL'],
             '--description=' + metadata['description']]
    references = sorted(refs.glob('*.dll')) + [sdk / 'ref' / name for name in ('NeoRune.Abstractions.dll', 'NeoRune.Game.dll')]
    args += ['--ref=' + str(path) for path in references]
    args += ['--source=' + str(path) for path in helpers + sources]
    response = response_file(args)
    output.mkdir(parents=True, exist_ok=True)
    rsp = output / 'ui-build.rsp'
    rsp.write_text(response, encoding='utf-8')
    result = subprocess.run([str(dotnet), str(sdk / 'tools/neorune.dll'), '@' + str(rsp)], capture_output=True, text=True)
    (output / 'ui-build.log').write_text(result.stdout + result.stderr, encoding='utf-8')
    if result.returncode or re.search(r'\berror\s+\w+\d+:', result.stdout + result.stderr, re.I):
        raise RuntimeError('NeoRune compilation failed; inspect ui-build.log')
    # SDK 1.0 creates versioned ModInfo and packs its discovery header. Rebuilding
    # ModInfo with the legacy builder here would discard generated mod settings.
    for name in ('ModInfo.uasset', 'ModInfo.uexp'):
        if not (output / 'Assets' / name).is_file():
            raise RuntimeError('Compiler did not produce ModInfo')
    for name in PAYLOAD_NAMES:
        if not (output / 'Pak' / name).is_file() or not (output / 'Pak' / name).stat().st_size:
            raise RuntimeError('Compiler did not produce the complete UI package')
    sdk_receipt = json.loads((output / 'Pak/MCD2Graphics_P.neorune.json').read_text())
    if sdk_receipt.get('neoRune') != pins['NeoRuneTool'] or sdk_receipt.get('version') != version or sdk_receipt.get('mod') != 'MCD2Graphics':
        raise RuntimeError('Compiler metadata receipt differs from the requested build')
    receipt = {
        'schemaVersion': 3, 'candidateOnly': True, 'version': version,
        'sourceFileSHA256': {name: source_sha(ROOT / name) for name in SOURCE_INPUTS},
        'payloadSHA256': {name: sha(output / 'Pak' / name) for name in PAYLOAD_NAMES},
        'metadataAPI': 'Blueprint Loader 2.0 BP_ModInfo; SDK-generated versioned metadata; no popup API',
        'compiler': 'NeoRune ' + pins['NeoRuneTool'] + ' / .NET reference pack ' + pins['dotnetReferencePack'],
        'toolSHA256': {'dotnet': sha(dotnet), **{path.name: sha(path) for path in sorted((sdk / 'tools').glob('*.dll'))}},
        'packToolSHA256': {name: sha(pack_tools / name) for name in ('repak.exe', 'retoc.exe')},
        'sdkInputSHA256': {path.relative_to(sdk).as_posix(): sha(path) for path in helpers + references[-2:]},
        'referencePack': pins['dotnetReferencePack'],
        'dotnetSDK': installed,
        'metadataSHA256': {name: sha(output / 'Assets' / name) for name in ('ModInfo.uasset', 'ModInfo.uexp')},
        'WindowsQualified': False, 'LinuxRuntimeQualified': False, 'publishedPayloadChanged': False,
    }
    if actor or extra_sources:
        receipt['trialOnly'] = True
        receipt['trialSourceSHA256'] = {path.name: sha(path) for path in sources}
    (output / 'build-receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    return receipt


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('dotnet', 'neorune-sdk', 'pack-tools', 'output'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    build_ui(args.dotnet, args.neorune_sdk, args.pack_tools, args.output)
    print('UI built with generated Mods-tab metadata; runtime qualification required. Nothing installed.')


if __name__ == '__main__':
    main()
