"""Build the identity-bound GDK save transport; no release assets are replaced."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

from build_toolchain import native_frame_guard, normalize_windows_header


def main():
    root = Path(__file__).resolve().parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--reshade-include', required=True, type=Path)
    parser.add_argument('--gdk-include', required=True, type=Path)
    parser.add_argument('--mingw-cxx', default='x86_64-w64-mingw32-g++')
    parser.add_argument('--output', type=Path, default=root / 'dist/gdk-save-bridge')
    args = parser.parse_args()
    if '#define RESHADE_API_VERSION 20' not in (args.reshade_include / 'reshade.hpp').read_text():
        raise ValueError('Expected ReShade API20 headers')
    output = args.output.resolve()
    include = output / 'include'
    include.mkdir(parents=True, exist_ok=True)
    for header in args.reshade_include.glob('reshade*.hpp'):
        (include / header.name).write_bytes(normalize_windows_header(header.read_bytes()))
    binary = output / 'mcd2-gdk-save-bridge.addon64'
    command = [args.mingw_cxx, '-std=c++20', '-O2', *native_frame_guard(args.mingw_cxx),
               '-shared', '-static', '-s', '-isystem', str(include), '-isystem', str(args.gdk_include.resolve()),
               str(root / 'src/gdk/save_bridge_addon.cpp'), '-o', str(binary)]
    with (output / 'build.log').open('w') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    sources = [*sorted((root / 'src/gdk').glob('*.*')), *sorted((root / 'src/providers').glob('*.hpp')), *sorted((root / 'src/providers').glob('*.h')),
               root / 'src/native/settings_bridge.hpp', root / 'src/latency/display_protocol.hpp',
               root / 'src/latency/process_exit.hpp', root / 'src/latency/process_lifetime.hpp',
               root / 'experiments/fg-streamline/fg_ui_protocol.hpp']
    receipt = {'runtimeQualified': False,
               'sourceSHA256': {path.relative_to(root).as_posix(): hashlib.sha256(path.read_bytes()).hexdigest()
                                for path in sources},
               'gdkHeadersSHA256': {path.relative_to(args.gdk_include).as_posix():hashlib.sha256(path.read_bytes()).hexdigest() for path in sorted(args.gdk_include.glob('*.h'))},
               'files': {binary.name: hashlib.sha256(binary.read_bytes()).hexdigest()}}
    (output / 'build-result.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(binary)


if __name__ == '__main__':
    main()
