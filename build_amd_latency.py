"""Build the source-based Anti-Lag 2 ABI bridge; no vendor DLL download."""
from pathlib import Path
import argparse, hashlib, json, subprocess

ROOT = Path(__file__).resolve().parent
SDK_COMMIT = '390aa4a8c8655d0ae6e90079db2c85e103a96da3'
PINNED = {
    'ffx_antilag2_dx12.h': '0554c804cfb02442974b90cf1887031bfd18d41fba2054a2cd23c8ee65e6265f',
    'LICENSE.txt': '020463d3b92d6d901c9b9d93a432359529be76883bb48f76eaeeecaea2f15c71',
}

def build(sysroot, output, clang_cl='clang-cl', linker='lld-link', with_tests=False):
    sdk = ROOT / 'third-party/AntiLag2'
    for name, expected in PINNED.items():
        if hashlib.sha256((sdk / name).read_bytes()).hexdigest() != expected:
            raise ValueError('Pinned Anti-Lag SDK source changed: ' + name)
    output.mkdir(parents=True, exist_ok=True)
    source = ROOT / 'src/latency/amd_antilag_bridge.cpp'
    obj = output / 'amd_antilag_bridge.obj'
    binary = output / 'mcd2-antilag2-bridge.dll'
    command = [clang_cl, '/nologo', '/std:c++20', '/O2', '/MT', '/EHsc', '/c', str(source),
               '/Fo' + str(obj), '/clang:-fms-compatibility-version=19.44',
               '/clang:-Wframe-larger-than=16384', '/clang:-Werror=frame-larger-than']
    for include in ['crt/include', 'sdk/include/ucrt', 'sdk/include/shared', 'sdk/include/um', 'sdk/include/winrt']:
        command += ['/imsvc' + str(sysroot / include)]
    link = [linker, '/dll', '/out:' + str(binary), '/implib:' + str(output / 'amd-antilag.lib'), str(obj), 'kernel32.lib']
    link += ['/export:mcd2_al2_' + name for name in ['abi', 'init', 'update', 'end_rendering', 'real_frame', 'shutdown']]
    for folder in ['crt/lib/x86_64', 'sdk/lib/ucrt/x86_64', 'sdk/lib/um/x86_64']:
        link += ['/libpath:' + str(sysroot / folder)]
    steps = [command, link]
    if with_tests:
        for name in ['amd_antilag_fake_driver', 'amd_antilag_bridge_test']:
            test_source = ROOT / 'tests' / (name + '.cpp')
            test_obj = output / (name + '.obj')
            steps.append([str(test_source) if part == str(source) else '/Fo' + str(test_obj)
                          if part.startswith('/Fo') else part for part in command])
            libs = [part for part in link if part.startswith('/libpath:')]
            if name == 'amd_antilag_fake_driver':
                steps.append([linker, '/dll', '/out:' + str(output / 'amdxc64.dll'), str(test_obj),
                              'kernel32.lib', *libs])
            else:
                steps.append([linker, '/out:' + str(output / 'amd-antilag-test.exe'), str(test_obj),
                              str(output / 'amd-antilag.lib'), 'kernel32.lib', *libs])
    with (output / 'build-private.log').open('w') as log:
        for step in steps:
            subprocess.run(step, stdout=log, stderr=subprocess.STDOUT, check=True)
    receipt = {'sdkCommit': SDK_COMMIT, 'sdkSHA256': PINNED,
               'sourceSHA256': hashlib.sha256(source.read_bytes()).hexdigest(),
               'recipeSHA256': hashlib.sha256((ROOT / 'build_amd_latency.py').read_bytes()).hexdigest(),
               'testSourceSHA256': {name: hashlib.sha256((ROOT / 'tests' / name).read_bytes()).hexdigest()
                                    for name in ['amd_antilag_fake_driver.cpp', 'amd_antilag_bridge_test.cpp']} if with_tests else {},
               'binarySHA256': hashlib.sha256(binary.read_bytes()).hexdigest(),
               'vendorRuntimeBundled': False, 'runtimeQualified': False}
    (output / 'build-receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    return binary

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--windows-sysroot', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--clang-cl', default='clang-cl')
    parser.add_argument('--linker', default='lld-link')
    parser.add_argument('--with-tests', action='store_true', help='Build an isolated synthetic driver fixture; never package it')
    args = parser.parse_args()
    build(args.windows_sysroot.resolve(), args.output.resolve(), args.clang_cl, args.linker, args.with_tests)
    print('Pinned Anti-Lag 2 bridge compiled; active AMD runtime qualification remains required.')

if __name__ == '__main__':
    main()
