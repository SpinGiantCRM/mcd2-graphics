"""Build an isolated FSR probe using externally acquired, pinned SDK headers."""
from pathlib import Path
import argparse
import hashlib
import json
import subprocess

COMMIT = '60f4ea81909200d8542eca14dccb2628b763a9a3'
HEADERS = {
    'api/include/ffx_api.h': 'e1a9d1b559eaf75f4cb401f1c43a66e2f3dc9621abfeb58b9c6fac56443177fd',
    'api/include/ffx_api_types.h': 'cc0adc0ffc804dd7cd03c39c15581b575e416383177ffe95060201d52cc0fc49',
    'api/include/dx12/ffx_api_dx12.h': 'c9afbc3c4c673723e9e5369a666f0ff54560543b5a93537f8ff453bf1c3bf530',
    'upscalers/include/ffx_upscale.h': '65be36d5653eb71c10379e0204f7bb7cd6a8aeec96e6f8cffc5f59ffeb1f89bf',
}

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def build(sdk, sysroot, output, compiler='clang-cl', linker='lld-link'):
    for name, expected in HEADERS.items():
        if digest(sdk / name) != expected:
            raise ValueError('Pinned FSR header changed: ' + name)
    source = Path(__file__).with_name('fsr_owned_inputs.cpp')
    output.mkdir(parents=True, exist_ok=True)
    obj, binary = output / 'fsr-owned-inputs.obj', output / 'fsr-owned-inputs.exe'
    compile_step = [compiler, '/nologo', '/std:c++20', '/O2', '/MT', '/EHsc', '/c', str(source),
                    '/Fo' + str(obj), '/I' + str(sdk), '/clang:-fms-compatibility-version=19.44',
                    '/clang:-Wframe-larger-than=16384', '/clang:-Werror=frame-larger-than']
    for folder in ['crt/include', 'sdk/include/ucrt', 'sdk/include/shared', 'sdk/include/um']:
        compile_step.append('/imsvc' + str(sysroot / folder))
    link_step = [linker, '/out:' + str(binary), str(obj), 'kernel32.lib', 'd3d12.lib', 'dxgi.lib', 'dxguid.lib']
    for folder in ['crt/lib/x86_64', 'sdk/lib/ucrt/x86_64', 'sdk/lib/um/x86_64']:
        link_step.append('/libpath:' + str(sysroot / folder))
    with (output / 'build-private.log').open('w') as log:
        for command in [compile_step, link_step]:
            subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    receipt = {'sdkCommit': COMMIT, 'SDKheadersSHA256': HEADERS, 'sourceSHA256': digest(source),
               'recipeSHA256': digest(Path(__file__)), 'binarySHA256': digest(binary),
               'runtimeBundled': False, 'gameIntegrationQualified': False}
    notice = Path(__file__).resolve().parents[2] / 'third-party/FSR-SDK-HEADERS-LICENSE.txt'
    (output / 'FSR-SDK-HEADERS-LICENSE.txt').write_bytes(notice.read_bytes())
    receipt['headersLicenseSHA256'] = digest(notice)
    (output / 'build-receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    return binary

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sdk', type=Path, required=True, help='Kits/FidelityFX from pinned SDK; not downloaded by this recipe')
    parser.add_argument('--windows-sysroot', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--clang-cl', default='clang-cl')
    parser.add_argument('--linker', default='lld-link')
    args = parser.parse_args()
    build(args.sdk.resolve(), args.windows_sysroot.resolve(), args.output.resolve(), args.clang_cl, args.linker)
    print('Isolated FSR owned-input probe compiled; game and release unchanged.')

if __name__ == '__main__':
    main()
